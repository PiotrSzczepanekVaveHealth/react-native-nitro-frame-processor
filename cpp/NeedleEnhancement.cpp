#include "NeedleEnhancement.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <vector>

namespace margelo::nitro::nitroframeprocessor {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kRoiDepthScale = 2.8f / 5.0f;
constexpr float kTrapezoidRatio = 2.8f / 3.56f;
constexpr float kNeedleEndpointThreshold = 0.5f;
constexpr int kNeedleWidthPx = 3;
constexpr int kMaskFuseWeightFactor = 2;
constexpr int kMeanFilterRadius = 4;
constexpr int kMaxBottomRowsToMask = 16;
constexpr int kRefinementOffset = 2;
constexpr bool kNeedleInsertionSideRight = true;

std::vector<uint8_t> roiBuffer;
std::vector<uint8_t> trapezoidRoiBuffer;
std::vector<uint8_t> processedBuffer;
std::vector<float> profileBuffer;
std::vector<float> smoothedProfileBuffer;
std::vector<float> rotationScratchBuffer;
std::vector<float> bestRotationBuffer;
std::vector<uint8_t> fusedMipBuffer;
std::vector<uint8_t> tempMorphologicBuffer;
std::vector<uint8_t> tempFuseBuffer;
std::vector<uint16_t> tempFuse16Buffer;
std::mutex bufferMutex;

struct DepthMaskParams {
  bool maskSkinLayer = false;
  int depthMaskThicknessPx = 8;
};

DepthMaskParams depthMaskParams;

int clampInt(int value, int lower, int upper) {
  return std::max(lower, std::min(value, upper));
}

unsigned int getEffectiveNeedleLength(unsigned int fallbackLength, int configuredNeedleLengthPx) {
  if (configuredNeedleLengthPx <= 0) {
    return fallbackLength;
  }

  return std::max(1U, std::min(static_cast<unsigned int>(configuredNeedleLengthPx), fallbackLength));
}

unsigned int clampUnsigned(unsigned int value, unsigned int lower, unsigned int upper) {
  return std::max(lower, std::min(value, upper));
}

void ensureSize(std::vector<uint8_t>& buffer, size_t size) {
  if (buffer.size() < size) {
    buffer.resize(size);
  }
}

void ensureSize(std::vector<uint16_t>& buffer, size_t size) {
  if (buffer.size() < size) {
    buffer.resize(size);
  }
}

void ensureSize(std::vector<float>& buffer, size_t size) {
  if (buffer.size() < size) {
    buffer.resize(size);
  }
}

void boundingBox(unsigned int inputRows, unsigned int inputColumns, float angleDegrees, unsigned int* outputRows, unsigned int* outputColumns) {
  if (angleDegrees == 0.0f) {
    *outputColumns = inputColumns;
    *outputRows = inputRows;
    return;
  }

  const float radians = angleDegrees * kPi / 180.0f;
  const float cosA = std::cos(radians);
  const float sinA = std::sin(radians);

  float maxColumn = -std::numeric_limits<float>::max();
  float maxRow = -std::numeric_limits<float>::max();
  float minColumn = std::numeric_limits<float>::max();
  float minRow = std::numeric_limits<float>::max();

  const unsigned int cornerColumns[] = {0, inputColumns - 1};
  const unsigned int cornerRows[] = {0, inputRows - 1};

  for (unsigned int cornerColumn : cornerColumns) {
    for (unsigned int cornerRow : cornerRows) {
      const float column = (static_cast<float>(cornerColumn) - static_cast<float>(inputColumns - 1) / 2.0f) * cosA -
        (static_cast<float>(cornerRow) - static_cast<float>(inputRows - 1) / 2.0f) * sinA;
      const float row = (static_cast<float>(cornerRow) - static_cast<float>(inputRows - 1) / 2.0f) * cosA +
        (static_cast<float>(cornerColumn) - static_cast<float>(inputColumns - 1) / 2.0f) * sinA;

      maxColumn = std::max(maxColumn, column);
      maxRow = std::max(maxRow, row);
      minColumn = std::min(minColumn, column);
      minRow = std::min(minRow, row);
    }
  }

  *outputColumns = static_cast<unsigned int>(std::ceil(maxColumn) - std::floor(minColumn) + 1.0f);
  *outputRows = static_cast<unsigned int>(std::ceil(maxRow) - std::floor(minRow) + 1.0f);
}

void rotateImageBilinear(
  const uint8_t* source,
  unsigned int inputRows,
  unsigned int inputColumns,
  float angleDegrees,
  float* destination,
  unsigned int stride,
  unsigned int outputRows,
  unsigned int outputColumns
) {
  const float radians = angleDegrees * kPi / 180.0f;
  const float cosA = std::cos(radians);
  const float sinA = std::sin(radians);

  const float inputCenterColumn = static_cast<float>(inputColumns - 1) * 0.5f;
  const float inputCenterRow = static_cast<float>(inputRows - 1) * 0.5f;
  const float outputCenterColumn = static_cast<float>(outputColumns - 1) * 0.5f;
  const float outputCenterRow = static_cast<float>(outputRows - 1) * 0.5f;

  for (unsigned int column = 0; column < outputColumns; column++) {
    const float columnRelative = static_cast<float>(column) - outputCenterColumn;
    for (unsigned int row = 0; row < outputRows; row++) {
      const float rowRelative = static_cast<float>(row) - outputCenterRow;
      const float sourceColumn = cosA * columnRelative + sinA * rowRelative + inputCenterColumn;
      const float sourceRow = -sinA * columnRelative + cosA * rowRelative + inputCenterRow;
      const size_t destinationIndex = static_cast<size_t>(column) * stride + row;

      if (sourceColumn < 0.0f || sourceColumn > static_cast<float>(inputColumns - 1) || sourceRow < 0.0f ||
          sourceRow > static_cast<float>(inputRows - 1)) {
        destination[destinationIndex] = 0.0f;
        continue;
      }

      const int column0 = static_cast<int>(std::floor(sourceColumn));
      const int row0 = static_cast<int>(std::floor(sourceRow));
      const int column1 = std::min(column0 + 1, static_cast<int>(inputColumns - 1));
      const int row1 = std::min(row0 + 1, static_cast<int>(inputRows - 1));
      const float dx = sourceColumn - static_cast<float>(column0);
      const float dy = sourceRow - static_cast<float>(row0);

      const float v00 = static_cast<float>(source[static_cast<size_t>(column0) * inputRows + row0]);
      const float v10 = static_cast<float>(source[static_cast<size_t>(column1) * inputRows + row0]);
      const float v01 = static_cast<float>(source[static_cast<size_t>(column0) * inputRows + row1]);
      const float v11 = static_cast<float>(source[static_cast<size_t>(column1) * inputRows + row1]);
      const float v0 = v00 * (1.0f - dx) + v10 * dx;
      const float v1 = v01 * (1.0f - dx) + v11 * dx;
      destination[destinationIndex] = v0 * (1.0f - dy) + v1 * dy;
    }
  }
}

void smoothProfileFast(const float* input, int count, float* output, float dcValue) {
  if (count <= 0) {
    return;
  }

  const int halfWindow = std::max(3, count / 20);
  float currentSum = 0.0f;
  int sampleCount = 0;

  for (int index = 0; index <= halfWindow && index < count; index++) {
    currentSum += input[index] - dcValue;
    sampleCount++;
  }
  output[0] = currentSum / static_cast<float>(sampleCount);

  for (int index = 1; index < count; index++) {
    const int oldIndex = index - halfWindow - 1;
    const int newIndex = index + halfWindow;

    if (newIndex < count) {
      currentSum += input[newIndex] - dcValue;
      sampleCount++;
    }
    if (oldIndex >= 0) {
      currentSum -= input[oldIndex] - dcValue;
      sampleCount--;
    }

    output[index] = currentSum / static_cast<float>(sampleCount);
  }
}

void findMaxPeakLastLocation(const float* values, int count, float* maxPeakValue, int* lastPeakLocation) {
  *maxPeakValue = 0.0f;
  *lastPeakLocation = -1;

  if (values == nullptr || count < 3) {
    return;
  }

  for (int index = 1; index < count - 1; index++) {
    if (values[index] > values[index - 1] && values[index] >= values[index + 1]) {
      if (values[index] > *maxPeakValue) {
        *maxPeakValue = values[index];
      }
      *lastPeakLocation = index;
    }
  }
}

int findTipXAlongBestLine(float* bestRotation, unsigned int rotatedColumns, unsigned int stride, int bestRho) {
  int tipX = -1;
  float lineSum = 0.0f;

  for (unsigned int column = 0; column < rotatedColumns; column++) {
    profileBuffer[column] = bestRotation[static_cast<size_t>(column) * stride + bestRho];
    lineSum += profileBuffer[column];
  }

  const float dcValue = lineSum / static_cast<float>(rotatedColumns);
  smoothProfileFast(profileBuffer.data(), static_cast<int>(rotatedColumns), smoothedProfileBuffer.data(), dcValue);

  float maxValue = -std::numeric_limits<float>::max();
  int maxIndex = -1;
  findMaxPeakLastLocation(smoothedProfileBuffer.data(), static_cast<int>(rotatedColumns), &maxValue, &maxIndex);

  if (maxValue > 0.0f) {
    const float inverseMax = 1.0f / maxValue;
    for (unsigned int index = 0; index < rotatedColumns; index++) {
      smoothedProfileBuffer[index] *= inverseMax;
    }
  }

  for (int index = maxIndex - 1; index > 1; index--) {
    if (smoothedProfileBuffer[index] > kNeedleEndpointThreshold && smoothedProfileBuffer[index - 1] < kNeedleEndpointThreshold) {
      tipX = index;
      break;
    }
  }

  return maxIndex < 0 || tipX < 0 ? 0 : tipX;
}

int intersect(int x1, int y1, int x2, int y2, int c) {
  const int dy = y2 - y1;
  const int dx = x2 - x1;
  if (dy == 0) {
    return x1;
  }

  const float numerator = static_cast<float>((c - y1) * dx) / static_cast<float>(dy);
  return static_cast<int>(static_cast<float>(x1) + numerator);
}

void morphologicOpenDisk2(uint8_t* input, uint8_t* output, int rows, int columns) {
  const size_t totalSize = static_cast<size_t>(rows) * columns;
  if (rows < 5 || columns < 5) {
    std::memcpy(output, input, totalSize);
    return;
  }

  static constexpr int8_t dx[13] = {0, -1, 0, 1, -2, -1, 0, 1, 2, -1, 0, 1, 0};
  static constexpr int8_t dy[13] = {-2, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 2};

  int offsets[13];
  for (int index = 0; index < 13; index++) {
    offsets[index] = dx[index] * rows + dy[index];
  }

  std::memset(output, 0, totalSize);
  std::memset(tempMorphologicBuffer.data(), 0, totalSize);

  for (int column = 2; column < columns - 2; column++) {
    for (int row = 2; row < rows - 2; row++) {
      const int pixelIndex = column * rows + row;
      uint8_t minValue = input[pixelIndex];
      for (int neighbor = 0; neighbor < 13; neighbor++) {
        minValue = std::min(minValue, input[pixelIndex + offsets[neighbor]]);
      }
      tempMorphologicBuffer[pixelIndex] = minValue;
    }
  }

  for (int column = 2; column < columns - 2; column++) {
    for (int row = 2; row < rows - 2; row++) {
      const int pixelIndex = column * rows + row;
      uint8_t maxValue = tempMorphologicBuffer[pixelIndex];
      for (int neighbor = 0; neighbor < 13; neighbor++) {
        maxValue = std::max(maxValue, tempMorphologicBuffer[pixelIndex + offsets[neighbor]]);
      }
      output[pixelIndex] = maxValue;
    }
  }
}
} // namespace

struct NeedleEnhancement::LineResult {
  int bestRho = 0;
  float thetaDeg = 0.0f;
  int entryX = 0;
  int entryY = 0;
  int tipX = 0;
  int tipY = 0;
  bool detected = false;
};

void NeedleEnhancement::setEnabled(bool value) {
  isEnabled_ = value;
}

bool NeedleEnhancement::isEnabled() const {
  return isEnabled_;
}

void NeedleEnhancement::setAngle(float degrees) {
  const float angle = std::clamp(degrees, 0.0f, 179.0f);
  thetaRangeMinDeg_ = angle;
  thetaRangeMaxDeg_ = angle;
  thetaStepDeg_ = DEFAULT_THETA_STEP_DEG;
}

void NeedleEnhancement::setAngleRange(float minDegrees, float maxDegrees, float stepDegrees) {
  const float lower = std::clamp(minDegrees, 0.0f, 179.0f);
  const float upper = std::clamp(maxDegrees, 0.0f, 179.0f);
  thetaRangeMinDeg_ = std::min(lower, upper);
  thetaRangeMaxDeg_ = std::max(lower, upper);
  thetaStepDeg_ = stepDegrees > 0.0f ? stepDegrees : DEFAULT_THETA_STEP_DEG;
}

void NeedleEnhancement::setNeedleLengthPx(int needleLengthPx) {
  needleLengthPx_ = std::max(0, needleLengthPx);
}

bool NeedleEnhancement::process(uint8_t* frame, int sampleCount, int scanlineCount) {
  if (!isEnabled_) {
    return true;
  }
  if (frame == nullptr || sampleCount <= 0 || scanlineCount <= 0) {
    return false;
  }

  std::lock_guard<std::mutex> guard(bufferMutex);

  const auto originalSampleCount = static_cast<unsigned int>(sampleCount);
  const auto originalScanlineCount = static_cast<unsigned int>(scanlineCount);
  unsigned int roiSampleCount = static_cast<unsigned int>(static_cast<float>(originalSampleCount) * kRoiDepthScale);
  roiSampleCount = clampUnsigned(roiSampleCount, 1, originalSampleCount);

  const size_t roiSize = static_cast<size_t>(roiSampleCount) * originalScanlineCount;
  const auto rotationStride = static_cast<unsigned int>(
    std::ceil(std::sqrt(static_cast<double>(roiSampleCount) * roiSampleCount +
                       static_cast<double>(originalScanlineCount) * originalScanlineCount)) +
    2.0
  );
  const unsigned int rotationBufferSide = std::max({rotationStride, roiSampleCount, originalScanlineCount});
  const size_t rotationBufferSize = static_cast<size_t>(rotationBufferSide) * rotationBufferSide;

  ensureSize(roiBuffer, roiSize);
  ensureSize(trapezoidRoiBuffer, roiSize);
  ensureSize(processedBuffer, roiSize);
  ensureSize(profileBuffer, rotationBufferSide);
  ensureSize(smoothedProfileBuffer, rotationBufferSide);
  ensureSize(rotationScratchBuffer, rotationBufferSize);
  ensureSize(bestRotationBuffer, rotationBufferSize);
  ensureSize(fusedMipBuffer, roiSize);
  ensureSize(tempMorphologicBuffer, roiSize);
  ensureSize(tempFuseBuffer, roiSize);
  ensureSize(tempFuse16Buffer, roiSize);

  for (unsigned int scanline = 0; scanline < originalScanlineCount; scanline++) {
    const size_t originalOffset = static_cast<size_t>(scanline) * originalSampleCount;
    const size_t roiOffset = static_cast<size_t>(scanline) * roiSampleCount;
    std::memcpy(roiBuffer.data() + roiOffset, frame + originalOffset, roiSampleCount);
    std::memcpy(trapezoidRoiBuffer.data() + roiOffset, frame + originalOffset, roiSampleCount);

    for (unsigned int sample = 1; sample < roiSampleCount; sample++) {
      if (kNeedleInsertionSideRight) {
        const int minX = intersect(
          0,
          0,
          static_cast<int>(originalScanlineCount) - static_cast<int>(kTrapezoidRatio * static_cast<float>(originalScanlineCount)),
          static_cast<int>(roiSampleCount - 1),
          static_cast<int>(sample)
        );
        if (static_cast<int>(scanline) < minX) {
          trapezoidRoiBuffer[roiOffset + sample] = 0;
        }
      } else {
        const int maxX = intersect(
          static_cast<int>(originalScanlineCount - 1),
          0,
          static_cast<int>(kTrapezoidRatio * static_cast<float>(originalScanlineCount)),
          static_cast<int>(roiSampleCount - 1),
          static_cast<int>(sample)
        );
        if (static_cast<int>(scanline) > maxX) {
          trapezoidRoiBuffer[roiOffset + sample] = 0;
        }
      }
    }
  }

  unsigned int rotatedRows = 0;
  unsigned int rotatedColumns = 0;
  LineResult line = detectNeedleLine(
    trapezoidRoiBuffer.data(),
    roiSampleCount,
    originalScanlineCount,
    rotationBufferSide,
    &rotatedRows,
    &rotatedColumns
  );
  if (!line.detected) {
    return true;
  }

  if (!fuseNeedleMask(roiBuffer.data(), roiSampleCount, originalScanlineCount, line, rotatedRows, rotatedColumns)) {
    return false;
  }

  for (unsigned int scanline = 0; scanline < originalScanlineCount; scanline++) {
    std::memcpy(
      frame + static_cast<size_t>(scanline) * originalSampleCount,
      roiBuffer.data() + static_cast<size_t>(scanline) * roiSampleCount,
      roiSampleCount
    );
  }

  return true;
}

void NeedleEnhancement::applyDepthMask(unsigned int sampleCount, unsigned int scanlineCount, int needleLengthPx) {
  if (depthMaskParams.maskSkinLayer) {
    const unsigned int maxRow = std::min(static_cast<unsigned int>(depthMaskParams.depthMaskThicknessPx), sampleCount);
    for (unsigned int scanline = 0; scanline < scanlineCount; scanline++) {
      std::memset(processedBuffer.data() + static_cast<size_t>(scanline) * sampleCount, 0, maxRow);
    }
  }

  int start = needleLengthPx > 0 ? needleLengthPx : static_cast<int>(sampleCount);
  const int capStart = std::max(0, static_cast<int>(sampleCount) - kMaxBottomRowsToMask);
  start = std::max(start, capStart);
  if (start >= static_cast<int>(sampleCount)) {
    return;
  }

  for (unsigned int scanline = 0; scanline < scanlineCount; scanline++) {
    std::memset(
      processedBuffer.data() + static_cast<size_t>(scanline) * sampleCount + start,
      0,
      static_cast<size_t>(sampleCount - static_cast<unsigned int>(start))
    );
  }
}

NeedleEnhancement::LineResult NeedleEnhancement::detectNeedleLine(
  const uint8_t* input,
  unsigned int sampleCount,
  unsigned int scanlineCount,
  unsigned int rotationStride,
  unsigned int* rotatedRows,
  unsigned int* rotatedColumns
) {
  LineResult result;
  *rotatedRows = 0;
  *rotatedColumns = 0;

  const size_t pixelCount = static_cast<size_t>(sampleCount) * scanlineCount;
  std::memcpy(processedBuffer.data(), input, pixelCount);
  applyDepthMask(sampleCount, scanlineCount, needleLengthPx_);

  float bestSum = -std::numeric_limits<float>::max();
  int bestRho = 0;
  float bestTheta = 0.0f;
  unsigned int bestRows = 0;
  unsigned int bestColumns = 0;

  float thetaMin = thetaRangeMinDeg_;
  float thetaMax = thetaRangeMaxDeg_;
  if (!kNeedleInsertionSideRight) {
    thetaMin = 180.0f - thetaRangeMaxDeg_;
    thetaMax = 180.0f - thetaRangeMinDeg_;
  }

  for (float theta = thetaMin; theta <= thetaMax + 0.5f; theta += thetaStepDeg_) {
    unsigned int rows = 0;
    unsigned int columns = 0;
    boundingBox(sampleCount, scanlineCount, theta, &rows, &columns);
    if (rows < 1 || columns < 1 || rows > rotationStride || columns > rotationStride) {
      continue;
    }

    rotateImageBilinear(processedBuffer.data(), sampleCount, scanlineCount, theta, rotationScratchBuffer.data(), rotationStride, rows, columns);
    std::fill(profileBuffer.begin(), profileBuffer.begin() + rows, 0.0f);

    const unsigned int effectiveNeedleLength = getEffectiveNeedleLength(columns, needleLengthPx_);
    const unsigned int firstNeedleColumn = columns > effectiveNeedleLength ? columns - effectiveNeedleLength : 0;
    for (unsigned int column = firstNeedleColumn; column < columns; column++) {
      const size_t columnOffset = static_cast<size_t>(column) * rotationStride;
      for (unsigned int row = 0; row < rows; row++) {
        profileBuffer[row] += rotationScratchBuffer[columnOffset + row];
      }
    }

    for (unsigned int row = 0; row < rows; row++) {
      if (profileBuffer[row] > bestSum) {
        bestSum = profileBuffer[row];
        bestRho = static_cast<int>(row);
        bestTheta = theta;
        bestRows = rows;
        bestColumns = columns;
        std::memcpy(bestRotationBuffer.data(), rotationScratchBuffer.data(), static_cast<size_t>(rotationStride) * columns * sizeof(float));
      }
    }
  }

  if (bestRows < 1 || bestColumns < 1 || bestRows > rotationStride || bestColumns > rotationStride) {
    return result;
  }

  *rotatedRows = bestRows;
  *rotatedColumns = bestColumns;

  result.bestRho = bestRho;
  result.thetaDeg = bestTheta;
  result.entryX = static_cast<int>(bestColumns - 1);
  result.entryY = bestRho;
  result.tipX = findTipXAlongBestLine(bestRotationBuffer.data(), bestColumns, rotationStride, bestRho);
  const int minTipX = std::max(0, result.entryX - static_cast<int>(getEffectiveNeedleLength(bestColumns, needleLengthPx_)) + 1);
  result.tipX = std::max(result.tipX, minTipX);
  result.tipY = bestRho;

  int startRow = std::max(0, result.tipY - kRefinementOffset);
  int endRow = std::min(static_cast<int>(bestRows - 1), result.tipY + kRefinementOffset);
  float sumProfile[2 * kRefinementOffset + 1] = {0.0f};
  int nonZeroPixels[2 * kRefinementOffset + 1] = {0};

  int x0 = std::min(result.tipX, result.entryX);
  int x1 = std::max(result.tipX, result.entryX);
  x0 = clampInt(x0, 0, static_cast<int>(bestColumns - 1));
  x1 = clampInt(x1, 0, static_cast<int>(bestColumns - 1));

  float bestMean = -1.0f;
  int refinedRho = result.bestRho;
  for (int column = x0; column <= x1; column++) {
    const size_t columnOffset = static_cast<size_t>(column) * rotationStride;
    for (int row = startRow; row <= endRow; row++) {
      const float value = bestRotationBuffer[columnOffset + row];
      if (value > 0.0f) {
        sumProfile[row - startRow] += value;
        nonZeroPixels[row - startRow]++;
      }
    }
  }

  for (int row = startRow; row <= endRow; row++) {
    const int index = row - startRow;
    if (nonZeroPixels[index] > 0) {
      const float mean = sumProfile[index] / static_cast<float>(nonZeroPixels[index]);
      if (mean > bestMean) {
        bestMean = mean;
        refinedRho = row;
      }
    }
  }

  if (refinedRho != result.bestRho) {
    result.bestRho = refinedRho;
    result.tipY = refinedRho;
    result.entryY = refinedRho;
    result.tipX = findTipXAlongBestLine(bestRotationBuffer.data(), bestColumns, rotationStride, refinedRho);
    result.tipX = std::max(result.tipX, minTipX);
  }

  result.detected = true;
  return result;
}

bool NeedleEnhancement::fuseNeedleMask(
  uint8_t* frame,
  unsigned int sampleCount,
  unsigned int scanlineCount,
  const LineResult& line,
  unsigned int rotatedRows,
  unsigned int rotatedColumns
) {
  if (rotatedRows == 0 || rotatedColumns == 0) {
    return false;
  }

  const size_t totalSize = static_cast<size_t>(sampleCount) * scanlineCount;
  std::memset(fusedMipBuffer.data(), 0, totalSize);

  const float radians = line.thetaDeg * kPi / 180.0f;
  const float cosA = std::cos(radians);
  const float sinA = std::sin(radians);

  const float inputCenterColumn = static_cast<float>(scanlineCount - 1) * 0.5f;
  const float inputCenterRow = static_cast<float>(sampleCount - 1) * 0.5f;
  const float outputCenterColumn = static_cast<float>(rotatedColumns - 1) * 0.5f;
  const float outputCenterRow = static_cast<float>(rotatedRows - 1) * 0.5f;

  const float entryColumnRelative = static_cast<float>(line.entryX) - outputCenterColumn;
  const float entryRowRelative = static_cast<float>(line.bestRho) - outputCenterRow;
  const float tipColumnRelative = static_cast<float>(line.tipX) - outputCenterColumn;
  const float tipRowRelative = static_cast<float>(line.bestRho) - outputCenterRow;

  const float entryColumn = cosA * entryColumnRelative + sinA * entryRowRelative + inputCenterColumn;
  const float entryRow = -sinA * entryColumnRelative + cosA * entryRowRelative + inputCenterRow;
  const float tipColumn = cosA * tipColumnRelative + sinA * tipRowRelative + inputCenterColumn;
  const float tipRow = -sinA * tipColumnRelative + cosA * tipRowRelative + inputCenterRow;

  const float dx = tipColumn - entryColumn;
  const float dy = tipRow - entryRow;
  const float segmentLengthSquared = dx * dx + dy * dy;
  if (segmentLengthSquared < 1.0e-20f) {
    return true;
  }

  const float radiusSquared = static_cast<float>(kNeedleWidthPx * kNeedleWidthPx);
  for (unsigned int scanline = 0; scanline < scanlineCount; scanline++) {
    for (unsigned int sample = 0; sample < sampleCount; sample++) {
      float t = ((static_cast<float>(scanline) - entryColumn) * dx + (static_cast<float>(sample) - entryRow) * dy) / segmentLengthSquared;
      t = std::clamp(t, 0.0f, 1.0f);
      const float closestColumn = entryColumn + t * dx;
      const float closestRow = entryRow + t * dy;
      const float deltaColumn = static_cast<float>(scanline) - closestColumn;
      const float deltaRow = static_cast<float>(sample) - closestRow;
      const size_t index = static_cast<size_t>(scanline) * sampleCount + sample;
      fusedMipBuffer[index] = (deltaColumn * deltaColumn + deltaRow * deltaRow <= radiusSquared) ? frame[index] : 0;
    }
  }

  constexpr int window = 2 * kMeanFilterRadius + 1;
  constexpr int normalization = window * window;

  for (unsigned int scanline = 0; scanline < scanlineCount; scanline++) {
    uint32_t sum = 0;
    const uint8_t* fusedColumn = fusedMipBuffer.data() + static_cast<size_t>(scanline) * sampleCount;
    uint16_t* tempColumn = tempFuse16Buffer.data() + static_cast<size_t>(scanline) * sampleCount;
    for (int offset = -kMeanFilterRadius; offset <= kMeanFilterRadius; offset++) {
      const int sample = clampInt(offset, 0, static_cast<int>(sampleCount - 1));
      sum += fusedColumn[sample];
    }
    tempColumn[0] = static_cast<uint16_t>(sum);

    for (unsigned int sample = 1; sample < sampleCount; sample++) {
      const int sampleToAdd = clampInt(static_cast<int>(sample) + kMeanFilterRadius, 0, static_cast<int>(sampleCount - 1));
      const int sampleToSubtract = clampInt(static_cast<int>(sample) - kMeanFilterRadius - 1, 0, static_cast<int>(sampleCount - 1));
      sum += fusedColumn[sampleToAdd];
      sum -= fusedColumn[sampleToSubtract];
      tempColumn[sample] = static_cast<uint16_t>(sum);
    }
  }

  for (unsigned int sample = 0; sample < sampleCount; sample++) {
    uint32_t sum = 0;
    for (int offset = -kMeanFilterRadius; offset <= kMeanFilterRadius; offset++) {
      const int scanline = clampInt(offset, 0, static_cast<int>(scanlineCount - 1));
      sum += tempFuse16Buffer[static_cast<size_t>(scanline) * sampleCount + sample];
    }
    tempFuseBuffer[sample] = static_cast<uint8_t>((sum + normalization / 2) / normalization);

    for (unsigned int scanline = 1; scanline < scanlineCount; scanline++) {
      const int scanlineToAdd = clampInt(static_cast<int>(scanline) + kMeanFilterRadius, 0, static_cast<int>(scanlineCount - 1));
      const int scanlineToSubtract = clampInt(static_cast<int>(scanline) - kMeanFilterRadius - 1, 0, static_cast<int>(scanlineCount - 1));
      sum += tempFuse16Buffer[static_cast<size_t>(scanlineToAdd) * sampleCount + sample];
      sum -= tempFuse16Buffer[static_cast<size_t>(scanlineToSubtract) * sampleCount + sample];
      tempFuseBuffer[static_cast<size_t>(scanline) * sampleCount + sample] =
        static_cast<uint8_t>((sum + normalization / 2) / normalization);
    }
  }

  morphologicOpenDisk2(tempFuseBuffer.data(), fusedMipBuffer.data(), static_cast<int>(sampleCount), static_cast<int>(scanlineCount));

  for (size_t index = 0; index < totalSize; index++) {
    const unsigned int value = static_cast<unsigned int>(frame[index]) + kMaskFuseWeightFactor * static_cast<unsigned int>(fusedMipBuffer[index]);
    frame[index] = static_cast<uint8_t>(std::min(value, 255U));
  }

  return true;
}

} // namespace margelo::nitro::nitroframeprocessor
