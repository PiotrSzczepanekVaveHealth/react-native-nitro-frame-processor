#include "NeedleEnhancement.hpp"

#include <algorithm>
#include <array>
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
constexpr float kNeedleEndpointThreshold = 0.4f;
constexpr int kNeedleWidthPx = 1;
constexpr int kMaskFuseWeightFactor = 5;
constexpr int kMeanFilterRadius = 3;
constexpr int kMaxBottomRowsToMask = 40;
constexpr int kRefinementOffset = 10;
constexpr bool kNeedleInsertionSideRight = true;
constexpr int kNeedleLineHalfBand = 1;
constexpr float kNeedleMaxEntryOriginalYFraction = 0.40f;
constexpr int kNeedleMaxEntryOriginalYPx = 120;
constexpr int kTopLineCandidateCount = 3;
constexpr int kTopLineCandidateRowsPerAngle = 2;
constexpr int kTopLineMinSegmentPixels = 12;
constexpr float kMotionEmaAlpha = 0.25f;
constexpr int kMotionSearchRadiusPx = 55;
constexpr float kMotionMinCorrectionPx = 4.0f;
constexpr float kMotionMaxCorrectionPx = 60.0f;
constexpr float kMotionProfileSuspiciousJumpPx = 8.0f;
constexpr int kMotionAboveRows = 4;
constexpr int kMotionBelowRows = 40;
constexpr int kMotionBoxRadius = 1;
constexpr int kMotionScoreSmoothWindow = 5;
constexpr float kMotionRobustKSigma = 3.0f;
constexpr float kMotionPeakFraction = 0.45f;
constexpr float kMotionScoreKSigma = 3.0f;
constexpr float kMotionMinScoreConfidence = 1.0f;
constexpr int kMotionMinHotArea = 12;
constexpr int kMotionMinHotRows = 2;
constexpr int kMotionMinHotColumns = 2;
constexpr float kMotionMinLocalPeakSigma = 4.0f;
constexpr float kMotionMinLocalPeakAboveThresholdSigma = 0.75f;
constexpr int kMotionCentroidHalfWindowPx = 4;
constexpr int kMotionBlobMinArea = 16;
constexpr int kMotionBlobMinWidth = 2;
constexpr int kMotionBlobMinHeight = 3;
constexpr int kMotionBlobMaxWidth = 22;
constexpr int kMotionBlobMaxHeight = 36;
constexpr float kMotionBlobMinPeakSigma = 6.0f;
constexpr float kMotionBlobMinPeakAboveThresholdSigma = 1.25f;
constexpr float kMotionBlobMinMeanAboveThresholdSigma = 0.75f;
constexpr float kMotionBlobMinSumAboveThresholdSigma = 20.0f;
constexpr float kMotionTrackMaxJumpPx = 18.0f;
constexpr float kMotionTrackAcquireMaxDistancePx = 60.0f;
constexpr float kMotionTrackDistancePenalty = 1.5f;
constexpr float kMotionTrackAlpha = 0.45f;
constexpr float kMotionTrackVelocityAlpha = 0.30f;
constexpr int kMotionTrackMaxMissFrames = 3;
constexpr int kMotionTrackMinConfirmFrames = 1;
constexpr float kMotionTrackLineThetaJumpDeg = 4.0f;
constexpr int kMotionTrackLineRhoJumpPx = 12;

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
std::array<std::vector<float>, kTopLineCandidateCount> topLineRotationBuffers;
std::vector<float> motionBackgroundBuffer;
std::vector<float> motionDifferenceBuffer;
bool motionBackgroundValid = false;
unsigned int motionBackgroundRows = 0;
unsigned int motionBackgroundColumns = 0;
std::mutex bufferMutex;

struct DepthMaskParams {
  bool maskSkinLayer = false;
  int depthMaskThicknessPx = 8;
};

DepthMaskParams depthMaskParams;

struct LineCandidate {
  bool valid = false;
  float scoreSum = -std::numeric_limits<float>::max();
  float thetaDeg = 0.0f;
  int rho = 0;
  unsigned int rotatedRows = 0;
  unsigned int rotatedColumns = 0;
  int tipX = 0;
  int entryX = 0;
  float segmentMean = -std::numeric_limits<float>::max();
  int rotationBufferIndex = -1;
};

struct NeedleTipMotionState {
  bool hasTip = false;
  float tipX = 0.0f;
  bool hasLine = false;
  float thetaDeg = 0.0f;
  int rho = 0;
  bool hasMotionTrack = false;
  float motionTrackX = 0.0f;
  float motionTrackVelocity = 0.0f;
  int motionTrackCount = 0;
  int motionMissCount = 0;
};

NeedleTipMotionState needleTipMotionState;

struct MotionBlob {
  int area = 0;
  int width = 0;
  int height = 0;
  float centroidX = 0.0f;
  float peakSigma = 0.0f;
  float peakAboveThresholdSigma = 0.0f;
  float meanAboveThresholdSigma = 0.0f;
  float sumAboveThresholdSigma = 0.0f;
  float score = 0.0f;
  float selectScore = -std::numeric_limits<float>::max();
};

int clampInt(int value, int lower, int upper) {
  return std::max(lower, std::min(value, upper));
}

float clampFloat(float value, float lower, float upper) {
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

void rotatedPointToOriginal(
  unsigned int inputRows,
  unsigned int inputColumns,
  unsigned int rotatedRows,
  unsigned int rotatedColumns,
  float thetaDeg,
  float rotatedColumn,
  float rotatedRow,
  float* originalColumn,
  float* originalRow
) {
  const float radians = thetaDeg * kPi / 180.0f;
  const float cosA = std::cos(radians);
  const float sinA = std::sin(radians);
  const float inputCenterColumn = static_cast<float>(inputColumns - 1) * 0.5f;
  const float inputCenterRow = static_cast<float>(inputRows - 1) * 0.5f;
  const float rotatedCenterColumn = static_cast<float>(rotatedColumns - 1) * 0.5f;
  const float rotatedCenterRow = static_cast<float>(rotatedRows - 1) * 0.5f;
  const float columnRelative = rotatedColumn - rotatedCenterColumn;
  const float rowRelative = rotatedRow - rotatedCenterRow;

  *originalColumn = cosA * columnRelative + sinA * rowRelative + inputCenterColumn;
  *originalRow = -sinA * columnRelative + cosA * rowRelative + inputCenterRow;
}

void rotatedOriginalFrameCorners(
  unsigned int inputRows,
  unsigned int inputColumns,
  unsigned int rotatedRows,
  unsigned int rotatedColumns,
  float thetaDeg,
  float cornerColumns[4],
  float cornerRows[4]
) {
  const float radians = thetaDeg * kPi / 180.0f;
  const float cosA = std::cos(radians);
  const float sinA = std::sin(radians);
  const float inputCenterColumn = static_cast<float>(inputColumns - 1) * 0.5f;
  const float inputCenterRow = static_cast<float>(inputRows - 1) * 0.5f;
  const float rotatedCenterColumn = static_cast<float>(rotatedColumns - 1) * 0.5f;
  const float rotatedCenterRow = static_cast<float>(rotatedRows - 1) * 0.5f;
  const float originalColumns[4] = {0.0f, static_cast<float>(inputColumns - 1), static_cast<float>(inputColumns - 1), 0.0f};
  const float originalRows[4] = {0.0f, 0.0f, static_cast<float>(inputRows - 1), static_cast<float>(inputRows - 1)};

  for (int index = 0; index < 4; index++) {
    const float columnRelative = originalColumns[index] - inputCenterColumn;
    const float rowRelative = originalRows[index] - inputCenterRow;
    cornerColumns[index] = cosA * columnRelative - sinA * rowRelative + rotatedCenterColumn;
    cornerRows[index] = sinA * columnRelative + cosA * rowRelative + rotatedCenterRow;
  }
}

bool rotatedRowColumnBoundsFromCorners(
  const float cornerColumns[4],
  const float cornerRows[4],
  unsigned int rotatedColumns,
  int rho,
  int* leftColumn,
  int* rightColumn
) {
  float intersections[8];
  int intersectionCount = 0;
  const float rowLine = static_cast<float>(rho);

  for (int index = 0; index < 4; index++) {
    const int nextIndex = (index + 1) & 3;
    const float column1 = cornerColumns[index];
    const float row1 = cornerRows[index];
    const float column2 = cornerColumns[nextIndex];
    const float row2 = cornerRows[nextIndex];
    const float minRow = std::min(row1, row2);
    const float maxRow = std::max(row1, row2);

    if (rowLine < minRow - 0.001f || rowLine > maxRow + 0.001f) {
      continue;
    }

    if (std::fabs(row2 - row1) < 1.0e-6f) {
      if (std::fabs(rowLine - row1) < 0.001f && intersectionCount <= 6) {
        intersections[intersectionCount++] = column1;
        intersections[intersectionCount++] = column2;
      }
    } else {
      const float t = (rowLine - row1) / (row2 - row1);
      if (t >= -0.001f && t <= 1.001f && intersectionCount < 8) {
        intersections[intersectionCount++] = column1 + t * (column2 - column1);
      }
    }
  }

  if (intersectionCount < 2) {
    return false;
  }

  float minColumn = intersections[0];
  float maxColumn = intersections[0];
  for (int index = 1; index < intersectionCount; index++) {
    minColumn = std::min(minColumn, intersections[index]);
    maxColumn = std::max(maxColumn, intersections[index]);
  }

  const int left = clampInt(static_cast<int>(std::ceil(minColumn - 0.001f)), 0, static_cast<int>(rotatedColumns - 1));
  const int right = clampInt(static_cast<int>(std::floor(maxColumn + 0.001f)), 0, static_cast<int>(rotatedColumns - 1));
  if (right < left) {
    return false;
  }

  *leftColumn = left;
  *rightColumn = right;
  return true;
}

bool rotatedRowOriginalColumnBounds(
  unsigned int inputRows,
  unsigned int inputColumns,
  unsigned int rotatedRows,
  unsigned int rotatedColumns,
  float thetaDeg,
  int rho,
  int* leftColumn,
  int* rightColumn
) {
  float cornerColumns[4];
  float cornerRows[4];
  rotatedOriginalFrameCorners(inputRows, inputColumns, rotatedRows, rotatedColumns, thetaDeg, cornerColumns, cornerRows);
  return rotatedRowColumnBoundsFromCorners(cornerColumns, cornerRows, rotatedColumns, rho, leftColumn, rightColumn);
}

bool entryIsTooDeep(
  unsigned int inputRows,
  unsigned int inputColumns,
  unsigned int rotatedRows,
  unsigned int rotatedColumns,
  float thetaDeg,
  int entryColumn,
  int entryRow
) {
  float originalColumn = 0.0f;
  float originalRow = 0.0f;
  rotatedPointToOriginal(
    inputRows,
    inputColumns,
    rotatedRows,
    rotatedColumns,
    thetaDeg,
    static_cast<float>(entryColumn),
    static_cast<float>(entryRow),
    &originalColumn,
    &originalRow
  );
  (void)originalColumn;

  const float entryLimit = std::min(
    kNeedleMaxEntryOriginalYFraction * static_cast<float>(inputRows),
    static_cast<float>(kNeedleMaxEntryOriginalYPx)
  );
  return originalRow > entryLimit;
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

float lineBandSum(float* rotation, unsigned int rotatedRows, unsigned int rotatedColumns, unsigned int stride, int rho, int column0, int column1) {
  if (rho < 0 || rho >= static_cast<int>(rotatedRows)) {
    return 0.0f;
  }

  column0 = clampInt(column0, 0, static_cast<int>(rotatedColumns - 1));
  column1 = clampInt(column1, 0, static_cast<int>(rotatedColumns - 1));
  if (column1 < column0) {
    return 0.0f;
  }

  float lineSum = 0.0f;
  for (int column = column0; column <= column1; column++) {
    float bandSum = 0.0f;
    int bandCount = 0;
    for (int deltaRow = -kNeedleLineHalfBand; deltaRow <= kNeedleLineHalfBand; deltaRow++) {
      const int row = rho + deltaRow;
      if (row >= 0 && row < static_cast<int>(rotatedRows)) {
        bandSum += rotation[static_cast<size_t>(column) * stride + row];
        bandCount++;
      }
    }
    if (bandCount > 0) {
      lineSum += bandSum / static_cast<float>(bandCount);
    }
  }

  return lineSum;
}

float lineSegmentBandMean(
  float* rotation,
  unsigned int rotatedRows,
  unsigned int rotatedColumns,
  unsigned int stride,
  int rho,
  int tipColumn,
  int entryColumn,
  int* segmentLength
) {
  int column0 = clampInt(std::min(tipColumn, entryColumn), 0, static_cast<int>(rotatedColumns - 1));
  int column1 = clampInt(std::max(tipColumn, entryColumn), 0, static_cast<int>(rotatedColumns - 1));

  if (column1 < column0) {
    *segmentLength = 0;
    return -std::numeric_limits<float>::max();
  }

  *segmentLength = column1 - column0 + 1;

  float sum = 0.0f;
  int count = 0;
  for (int column = column0; column <= column1; column++) {
    for (int deltaRow = -kNeedleLineHalfBand; deltaRow <= kNeedleLineHalfBand; deltaRow++) {
      const int row = rho + deltaRow;
      if (row >= 0 && row < static_cast<int>(rotatedRows)) {
        const float value = rotation[static_cast<size_t>(column) * stride + row];
        if (value > 0.0f) {
          sum += value;
          count++;
        }
      }
    }
  }

  if (count <= 0) {
    return -std::numeric_limits<float>::max();
  }

  return sum / static_cast<float>(count);
}

int findTipXAlongBestLine(float* bestRotation, unsigned int rotatedRows, unsigned int rotatedColumns, unsigned int stride, int bestRho) {
  int tipX = -1;
  float lineSum = 0.0f;
  bestRho = clampInt(bestRho, 0, static_cast<int>(rotatedRows - 1));

  for (unsigned int column = 0; column < rotatedColumns; column++) {
    float bandSum = 0.0f;
    int bandCount = 0;
    for (int deltaRow = -kNeedleLineHalfBand; deltaRow <= kNeedleLineHalfBand; deltaRow++) {
      const int row = bestRho + deltaRow;
      if (row >= 0 && row < static_cast<int>(rotatedRows)) {
        bandSum += bestRotation[static_cast<size_t>(column) * stride + row];
        bandCount++;
      }
    }
    profileBuffer[column] = bandCount > 0 ? bandSum / static_cast<float>(bandCount) : 0.0f;
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

void resetTipMotionState() {
  needleTipMotionState = NeedleTipMotionState{};
}

void resetMotionBackgroundState() {
  motionBackgroundValid = false;
  motionBackgroundRows = 0;
  motionBackgroundColumns = 0;
  resetTipMotionState();
}

float angleDifferenceDeg(float a, float b) {
  float delta = std::fmod((a - b) + 180.0f, 360.0f);
  if (delta < 0.0f) {
    delta += 360.0f;
  }
  return std::fabs(delta - 180.0f);
}

void updateMotionDifferenceImage(const uint8_t* currentImage, unsigned int rows, unsigned int columns) {
  const size_t pixelCount = static_cast<size_t>(rows) * columns;

  if (!motionBackgroundValid || motionBackgroundRows != rows || motionBackgroundColumns != columns) {
    for (size_t index = 0; index < pixelCount; index++) {
      motionBackgroundBuffer[index] = static_cast<float>(currentImage[index]);
      motionDifferenceBuffer[index] = 0.0f;
    }
    motionBackgroundValid = true;
    motionBackgroundRows = rows;
    motionBackgroundColumns = columns;
    resetTipMotionState();
    return;
  }

  for (size_t index = 0; index < pixelCount; index++) {
    const float current = static_cast<float>(currentImage[index]);
    motionBackgroundBuffer[index] = kMotionEmaAlpha * current + (1.0f - kMotionEmaAlpha) * motionBackgroundBuffer[index];
    motionDifferenceBuffer[index] = std::fabs(current - motionBackgroundBuffer[index]);
  }
}

float medianFloat(std::vector<float>& values, int count) {
  if (count <= 0) {
    return 0.0f;
  }

  std::sort(values.begin(), values.begin() + count);
  if ((count & 1) != 0) {
    return values[static_cast<size_t>(count / 2)];
  }

  return 0.5f * (values[static_cast<size_t>(count / 2 - 1)] + values[static_cast<size_t>(count / 2)]);
}

void markMotionTrackMiss() {
  if (!needleTipMotionState.hasMotionTrack) {
    return;
  }

  needleTipMotionState.motionMissCount++;
  if (needleTipMotionState.motionMissCount > kMotionTrackMaxMissFrames) {
    needleTipMotionState.hasMotionTrack = false;
    needleTipMotionState.motionTrackX = 0.0f;
    needleTipMotionState.motionTrackVelocity = 0.0f;
    needleTipMotionState.motionTrackCount = 0;
  }
}

bool motionHotspotTipFromDiff(
  const float* differenceImage,
  int inputRows,
  int inputColumns,
  int rotatedRows,
  int rotatedColumns,
  float thetaDeg,
  int rho,
  int profileTip,
  int entryColumn,
  float* tipOut
) {
  rho = clampInt(rho, 0, rotatedRows - 1);
  profileTip = clampInt(profileTip, 0, rotatedColumns - 1);
  entryColumn = clampInt(entryColumn, 0, rotatedColumns - 1);

  float predictionColumn = static_cast<float>(profileTip);
  if (needleTipMotionState.hasMotionTrack) {
    predictionColumn = needleTipMotionState.motionTrackX + needleTipMotionState.motionTrackVelocity;
  } else if (needleTipMotionState.hasTip) {
    predictionColumn = needleTipMotionState.tipX;
  }
  predictionColumn = clampFloat(predictionColumn, 0.0f, static_cast<float>(rotatedColumns - 1));

  int columnA = clampInt(profileTip - kMotionSearchRadiusPx, 0, rotatedColumns - 1);
  int columnB = clampInt(profileTip + kMotionSearchRadiusPx, 0, rotatedColumns - 1);
  if (kNeedleInsertionSideRight) {
    columnB = clampInt(columnB, 0, entryColumn);
  } else {
    columnA = clampInt(columnA, entryColumn, rotatedColumns - 1);
  }

  const int rowA = clampInt(rho - kMotionAboveRows, 0, rotatedRows - 1);
  const int rowB = clampInt(rho + kMotionBelowRows, 0, rotatedRows - 1);
  if (columnB < columnA || rowB < rowA) {
    return false;
  }

  const int columnCount = columnB - columnA + 1;
  const int rowCount = rowB - rowA + 1;
  const int pixelCount = columnCount * rowCount;
  if (pixelCount <= 0) {
    return false;
  }

  std::vector<float> roiRaw(static_cast<size_t>(pixelCount));
  std::vector<float> roi(static_cast<size_t>(pixelCount));
  std::vector<float> hot(static_cast<size_t>(pixelCount));
  std::vector<float> roiValues(static_cast<size_t>(pixelCount));
  std::vector<float> devValues(static_cast<size_t>(pixelCount));
  std::vector<uint8_t> binary(static_cast<size_t>(pixelCount));
  std::vector<uint8_t> visited(static_cast<size_t>(pixelCount));
  std::vector<int> queue(static_cast<size_t>(pixelCount));
  std::vector<float> score(static_cast<size_t>(columnCount));
  std::vector<float> scoreSmooth(static_cast<size_t>(columnCount));

  const float radians = thetaDeg * kPi / 180.0f;
  const float cosA = std::cos(radians);
  const float sinA = std::sin(radians);
  const float inputCenterColumn = static_cast<float>(inputColumns - 1) * 0.5f;
  const float inputCenterRow = static_cast<float>(inputRows - 1) * 0.5f;
  const float rotatedCenterColumn = static_cast<float>(rotatedColumns - 1) * 0.5f;
  const float rotatedCenterRow = static_cast<float>(rotatedRows - 1) * 0.5f;

  for (int roiRow = 0; roiRow < rowCount; roiRow++) {
    float columnRelative = static_cast<float>(columnA) - rotatedCenterColumn;
    const float rowRelative = static_cast<float>(rowA + roiRow) - rotatedCenterRow;
    float originalColumn = cosA * columnRelative + sinA * rowRelative + inputCenterColumn;
    float originalRow = -sinA * columnRelative + cosA * rowRelative + inputCenterRow;

    for (int roiColumn = 0; roiColumn < columnCount; roiColumn++) {
      const int index = roiRow * columnCount + roiColumn;
      const int roundedColumn = static_cast<int>(std::floor(originalColumn + 0.5f));
      const int roundedRow = static_cast<int>(std::floor(originalRow + 0.5f));
      roiRaw[static_cast<size_t>(index)] =
        roundedColumn >= 0 && roundedColumn < inputColumns && roundedRow >= 0 && roundedRow < inputRows
        ? differenceImage[static_cast<size_t>(roundedColumn) * inputRows + roundedRow]
        : 0.0f;

      columnRelative += 1.0f;
      originalColumn += cosA;
      originalRow -= sinA;
    }
  }

  for (int roiColumn = 0; roiColumn < columnCount; roiColumn++) {
    score[static_cast<size_t>(roiColumn)] = 0.0f;
    for (int roiRow = 0; roiRow < rowCount; roiRow++) {
      const int index = roiRow * columnCount + roiColumn;
      float boxSum = 0.0f;
      int boxCount = 0;
      for (int deltaColumn = -kMotionBoxRadius; deltaColumn <= kMotionBoxRadius; deltaColumn++) {
        const int boxColumn = clampInt(roiColumn + deltaColumn, 0, columnCount - 1);
        for (int deltaRow = -kMotionBoxRadius; deltaRow <= kMotionBoxRadius; deltaRow++) {
          const int boxRow = clampInt(roiRow + deltaRow, 0, rowCount - 1);
          boxSum += roiRaw[static_cast<size_t>(boxRow * columnCount + boxColumn)];
          boxCount++;
        }
      }

      roi[static_cast<size_t>(index)] = boxCount > 0 ? boxSum / static_cast<float>(boxCount) : roiRaw[static_cast<size_t>(index)];
      roiValues[static_cast<size_t>(index)] = roi[static_cast<size_t>(index)];
      hot[static_cast<size_t>(index)] = 0.0f;
      binary[static_cast<size_t>(index)] = 0;
      visited[static_cast<size_t>(index)] = 0;
    }
  }

  const float background = medianFloat(roiValues, pixelCount);
  for (int index = 0; index < pixelCount; index++) {
    devValues[static_cast<size_t>(index)] = std::fabs(roi[static_cast<size_t>(index)] - background);
  }

  float robustSigma = 1.4826f * medianFloat(devValues, pixelCount);
  if (robustSigma <= 1.0e-6f) {
    float sum = 0.0f;
    float sum2 = 0.0f;
    for (float value : roi) {
      sum += value;
      sum2 += value * value;
    }
    const float mean = sum / static_cast<float>(pixelCount);
    robustSigma = std::sqrt(std::max(0.0f, sum2 / static_cast<float>(pixelCount) - mean * mean));
  }
  robustSigma = std::max(robustSigma, 1.0e-3f);

  const float peakValue = *std::max_element(roi.begin(), roi.end());
  const float amplitude = peakValue - background;
  if (amplitude <= 1.0e-6f) {
    return false;
  }

  const float threshold = std::max(background + kMotionRobustKSigma * robustSigma, background + kMotionPeakFraction * amplitude);
  for (int roiColumn = 0; roiColumn < columnCount; roiColumn++) {
    for (int roiRow = 0; roiRow < rowCount; roiRow++) {
      const int index = roiRow * columnCount + roiColumn;
      if (roi[static_cast<size_t>(index)] > threshold) {
        binary[static_cast<size_t>(index)] = 1;
        hot[static_cast<size_t>(index)] = roi[static_cast<size_t>(index)] - threshold;
        score[static_cast<size_t>(roiColumn)] += hot[static_cast<size_t>(index)];
      }
    }
  }

  const int halfSmooth = kMotionScoreSmoothWindow / 2;
  for (int roiColumn = 0; roiColumn < columnCount; roiColumn++) {
    const int column0 = clampInt(roiColumn - halfSmooth, 0, columnCount - 1);
    const int column1 = clampInt(roiColumn + halfSmooth, 0, columnCount - 1);
    float sum = 0.0f;
    int count = 0;
    for (int column = column0; column <= column1; column++) {
      sum += score[static_cast<size_t>(column)];
      count++;
    }
    scoreSmooth[static_cast<size_t>(roiColumn)] = count > 0 ? sum / static_cast<float>(count) : score[static_cast<size_t>(roiColumn)];
  }

  std::vector<float> scoreTmp = scoreSmooth;
  std::vector<float> scoreDev(static_cast<size_t>(columnCount));
  const float scoreMedian = medianFloat(scoreTmp, columnCount);
  for (int column = 0; column < columnCount; column++) {
    scoreDev[static_cast<size_t>(column)] = std::fabs(scoreSmooth[static_cast<size_t>(column)] - scoreMedian);
  }
  const float scoreSigma = std::max(1.4826f * medianFloat(scoreDev, columnCount), 1.0e-3f);
  const float scoreThreshold = scoreMedian + kMotionScoreKSigma * scoreSigma;

  std::vector<MotionBlob> blobs;
  blobs.reserve(16);
  for (int startIndex = 0; startIndex < pixelCount; startIndex++) {
    if (!binary[static_cast<size_t>(startIndex)] || visited[static_cast<size_t>(startIndex)]) {
      continue;
    }

    int queueHead = 0;
    int queueTail = 0;
    queue[static_cast<size_t>(queueTail++)] = startIndex;
    visited[static_cast<size_t>(startIndex)] = 1;

    int area = 0;
    int minRow = rowCount;
    int maxRow = -1;
    int minColumn = columnCount;
    int maxColumn = -1;
    float peak = -std::numeric_limits<float>::max();
    float hotSum = 0.0f;
    float hotWeightedColumn = 0.0f;

    while (queueHead < queueTail) {
      const int index = queue[static_cast<size_t>(queueHead++)];
      const int row = index / columnCount;
      const int column = index - row * columnCount;
      area++;
      minRow = std::min(minRow, row);
      maxRow = std::max(maxRow, row);
      minColumn = std::min(minColumn, column);
      maxColumn = std::max(maxColumn, column);
      peak = std::max(peak, roi[static_cast<size_t>(index)]);
      hotSum += hot[static_cast<size_t>(index)];
      hotWeightedColumn += static_cast<float>(columnA + column) * hot[static_cast<size_t>(index)];

      for (int deltaRow = -1; deltaRow <= 1; deltaRow++) {
        for (int deltaColumn = -1; deltaColumn <= 1; deltaColumn++) {
          if (deltaRow == 0 && deltaColumn == 0) {
            continue;
          }
          const int neighborRow = row + deltaRow;
          const int neighborColumn = column + deltaColumn;
          if (neighborRow < 0 || neighborRow >= rowCount || neighborColumn < 0 || neighborColumn >= columnCount) {
            continue;
          }
          const int neighborIndex = neighborRow * columnCount + neighborColumn;
          if (binary[static_cast<size_t>(neighborIndex)] && !visited[static_cast<size_t>(neighborIndex)]) {
            visited[static_cast<size_t>(neighborIndex)] = 1;
            queue[static_cast<size_t>(queueTail++)] = neighborIndex;
          }
        }
      }
    }

    if (area < kMotionBlobMinArea) {
      continue;
    }

    const int width = maxColumn - minColumn + 1;
    const int height = maxRow - minRow + 1;
    if (width < kMotionBlobMinWidth || height < kMotionBlobMinHeight || width > kMotionBlobMaxWidth || height > kMotionBlobMaxHeight) {
      continue;
    }

    const float peakSigma = (peak - background) / robustSigma;
    const float peakAboveThresholdSigma = (peak - threshold) / robustSigma;
    const float meanAboveThresholdSigma = (hotSum / static_cast<float>(area)) / robustSigma;
    const float sumAboveThresholdSigma = hotSum / robustSigma;
    if (
      peakSigma < kMotionBlobMinPeakSigma ||
      peakAboveThresholdSigma < kMotionBlobMinPeakAboveThresholdSigma ||
      meanAboveThresholdSigma < kMotionBlobMinMeanAboveThresholdSigma ||
      sumAboveThresholdSigma < kMotionBlobMinSumAboveThresholdSigma
    ) {
      continue;
    }

    MotionBlob blob;
    blob.area = area;
    blob.width = width;
    blob.height = height;
    blob.centroidX = hotSum > 0.0f ? hotWeightedColumn / hotSum : static_cast<float>(columnA + (minColumn + maxColumn) / 2);
    blob.peakSigma = peakSigma;
    blob.peakAboveThresholdSigma = peakAboveThresholdSigma;
    blob.meanAboveThresholdSigma = meanAboveThresholdSigma;
    blob.sumAboveThresholdSigma = sumAboveThresholdSigma;
    const float compactnessPenalty = 0.15f * std::max(
      static_cast<float>(width) / std::max(static_cast<float>(height), 1.0f),
      static_cast<float>(height) / std::max(static_cast<float>(width), 1.0f)
    );
    blob.score = sumAboveThresholdSigma + 2.0f * peakAboveThresholdSigma + 0.25f * static_cast<float>(area) - compactnessPenalty;
    blobs.push_back(blob);
  }

  int bestBlob = -1;
  float bestSelectScore = -std::numeric_limits<float>::max();
  for (size_t blobIndex = 0; blobIndex < blobs.size(); blobIndex++) {
    const float distanceToPrediction = std::fabs(blobs[blobIndex].centroidX - predictionColumn);
    const float distanceToProfile = std::fabs(blobs[blobIndex].centroidX - static_cast<float>(profileTip));

    if (needleTipMotionState.hasMotionTrack) {
      if (distanceToPrediction > kMotionTrackMaxJumpPx) {
        continue;
      }
    } else if (distanceToProfile > kMotionTrackAcquireMaxDistancePx) {
      continue;
    }

    const float selectScore = blobs[blobIndex].score - kMotionTrackDistancePenalty * distanceToPrediction;
    blobs[blobIndex].selectScore = selectScore;
    if (selectScore > bestSelectScore) {
      bestSelectScore = selectScore;
      bestBlob = static_cast<int>(blobIndex);
    }
  }

  if (bestBlob < 0) {
    markMotionTrackMiss();
    return false;
  }

  const float rawMotionColumn = blobs[static_cast<size_t>(bestBlob)].centroidX;
  float motionTip = rawMotionColumn;
  float newVelocity = 0.0f;
  const int newTrackCount = needleTipMotionState.motionTrackCount + 1;
  if (needleTipMotionState.hasMotionTrack) {
    const float rawStep = clampFloat(rawMotionColumn - needleTipMotionState.motionTrackX, -kMotionTrackMaxJumpPx, kMotionTrackMaxJumpPx);
    motionTip = needleTipMotionState.motionTrackX + kMotionTrackAlpha * rawStep;
    newVelocity = (1.0f - kMotionTrackVelocityAlpha) * needleTipMotionState.motionTrackVelocity +
      kMotionTrackVelocityAlpha * (motionTip - needleTipMotionState.motionTrackX);
  }

  const int bestColumn = clampInt(static_cast<int>(std::floor(motionTip + 0.5f)) - columnA, 0, columnCount - 1);
  const float scoreAtMotion = scoreSmooth[static_cast<size_t>(bestColumn)];
  const float confidence = std::max(0.0f, (scoreAtMotion - scoreThreshold) / scoreSigma);

  const int localColumn0 = clampInt(bestColumn - kMotionCentroidHalfWindowPx, 0, columnCount - 1);
  const int localColumn1 = clampInt(bestColumn + kMotionCentroidHalfWindowPx, 0, columnCount - 1);
  int localHotArea = 0;
  int localHotRows = 0;
  int localHotColumns = 0;
  float localPeak = -std::numeric_limits<float>::max();

  for (int row = 0; row < rowCount; row++) {
    bool rowHasHot = false;
    for (int column = localColumn0; column <= localColumn1; column++) {
      const int index = row * columnCount + column;
      localPeak = std::max(localPeak, roi[static_cast<size_t>(index)]);
      if (binary[static_cast<size_t>(index)]) {
        localHotArea++;
        rowHasHot = true;
      }
    }
    if (rowHasHot) {
      localHotRows++;
    }
  }

  for (int column = localColumn0; column <= localColumn1; column++) {
    bool columnHasHot = false;
    for (int row = 0; row < rowCount; row++) {
      if (binary[static_cast<size_t>(row * columnCount + column)]) {
        columnHasHot = true;
        break;
      }
    }
    if (columnHasHot) {
      localHotColumns++;
    }
  }

  const float localPeakSigma = (localPeak - background) / robustSigma;
  const float localPeakAboveThresholdSigma = (localPeak - threshold) / robustSigma;
  const float delta = motionTip - static_cast<float>(profileTip);
  const bool hasHotspot = scoreAtMotion > scoreThreshold &&
    confidence >= kMotionMinScoreConfidence &&
    localHotArea >= kMotionMinHotArea &&
    localHotRows >= kMotionMinHotRows &&
    localHotColumns >= kMotionMinHotColumns &&
    localPeakSigma >= kMotionMinLocalPeakSigma &&
    localPeakAboveThresholdSigma >= kMotionMinLocalPeakAboveThresholdSigma;

  if (
    !hasHotspot ||
    std::fabs(delta) < kMotionMinCorrectionPx ||
    std::fabs(delta) > kMotionMaxCorrectionPx ||
    newTrackCount < kMotionTrackMinConfirmFrames
  ) {
    markMotionTrackMiss();
    return false;
  }

  needleTipMotionState.hasMotionTrack = true;
  needleTipMotionState.motionTrackX = motionTip;
  needleTipMotionState.motionTrackVelocity = newVelocity;
  needleTipMotionState.motionTrackCount = newTrackCount;
  needleTipMotionState.motionMissCount = 0;

  *tipOut = motionTip;
  return true;
}

void initializeLineCandidates(std::array<LineCandidate, kTopLineCandidateCount>& candidates) {
  for (auto& candidate : candidates) {
    candidate = LineCandidate{};
  }
}

void insertTopLineCandidate(
  std::array<LineCandidate, kTopLineCandidateCount>& candidates,
  LineCandidate candidate,
  const float* rotation,
  unsigned int stride
) {
  int worstIndex = 0;
  float worstScore = candidates[0].scoreSum;
  for (int index = 1; index < kTopLineCandidateCount; index++) {
    if (candidates[index].scoreSum < worstScore) {
      worstScore = candidates[index].scoreSum;
      worstIndex = index;
    }
  }

  if (candidate.scoreSum <= worstScore) {
    return;
  }

  int rotationBufferIndex = candidates[worstIndex].rotationBufferIndex;
  if (rotationBufferIndex < 0 || rotationBufferIndex >= kTopLineCandidateCount) {
    rotationBufferIndex = worstIndex;
  }

  const size_t copyCount = static_cast<size_t>(candidate.rotatedColumns) * stride;
  std::memcpy(topLineRotationBuffers[static_cast<size_t>(rotationBufferIndex)].data(), rotation, copyCount * sizeof(float));
  candidate.rotationBufferIndex = rotationBufferIndex;
  candidate.valid = true;
  candidates[worstIndex] = candidate;

  std::sort(candidates.begin(), candidates.end(), [](const LineCandidate& lhs, const LineCandidate& rhs) {
    return lhs.scoreSum > rhs.scoreSum;
  });
}

bool evaluateLineCandidateOnCurrentRotation(
  float* rotation,
  unsigned int inputRows,
  unsigned int inputColumns,
  unsigned int rotatedRows,
  unsigned int rotatedColumns,
  unsigned int stride,
  float thetaDeg,
  const float cornerColumns[4],
  const float cornerRows[4],
  int rho,
  float scoreSum,
  LineCandidate* candidate
) {
  int leftBound = 0;
  int rightBound = 0;
  if (!rotatedRowColumnBoundsFromCorners(cornerColumns, cornerRows, rotatedColumns, rho, &leftBound, &rightBound)) {
    return false;
  }

  const int entryColumn = kNeedleInsertionSideRight ? rightBound : leftBound;
  const int tipColumn = findTipXAlongBestLine(rotation, rotatedRows, rotatedColumns, stride, rho);
  int segmentLength = 0;
  const float segmentMean = lineSegmentBandMean(rotation, rotatedRows, rotatedColumns, stride, rho, tipColumn, entryColumn, &segmentLength);
  if (segmentLength < kTopLineMinSegmentPixels) {
    return false;
  }

  if (entryIsTooDeep(inputRows, inputColumns, rotatedRows, rotatedColumns, thetaDeg, entryColumn, rho)) {
    return false;
  }

  candidate->valid = true;
  candidate->scoreSum = scoreSum;
  candidate->thetaDeg = thetaDeg;
  candidate->rho = rho;
  candidate->rotatedRows = rotatedRows;
  candidate->rotatedColumns = rotatedColumns;
  candidate->tipX = tipColumn;
  candidate->entryX = entryColumn;
  candidate->segmentMean = segmentMean;
  return true;
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
  if (isEnabled_ != value) {
    std::lock_guard<std::mutex> guard(bufferMutex);
    resetMotionBackgroundState();
  }
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
  ensureSize(motionBackgroundBuffer, roiSize);
  ensureSize(motionDifferenceBuffer, roiSize);
  for (auto& rotationBuffer : topLineRotationBuffers) {
    ensureSize(rotationBuffer, rotationBufferSize);
  }

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
  updateMotionDifferenceImage(processedBuffer.data(), sampleCount, scanlineCount);

  float bestSum = -std::numeric_limits<float>::max();
  int bestRho = 0;
  float bestTheta = 0.0f;
  unsigned int bestRows = 0;
  unsigned int bestColumns = 0;
  std::array<LineCandidate, kTopLineCandidateCount> topCandidates;
  initializeLineCandidates(topCandidates);

  float thetaMin = thetaRangeMinDeg_;
  float thetaMax = thetaRangeMaxDeg_;
  if (!kNeedleInsertionSideRight) {
    thetaMin = 180.0f - thetaRangeMaxDeg_;
    thetaMax = 180.0f - thetaRangeMinDeg_;
  }
  const unsigned int needleLengthRotated = getEffectiveNeedleLength(rotationStride, needleLengthPx_);

  for (float theta = thetaMin; theta <= thetaMax + 0.5f; theta += thetaStepDeg_) {
    unsigned int rows = 0;
    unsigned int columns = 0;
    boundingBox(sampleCount, scanlineCount, theta, &rows, &columns);
    if (rows < 1 || columns < 1 || rows > rotationStride || columns > rotationStride) {
      continue;
    }

    rotateImageBilinear(processedBuffer.data(), sampleCount, scanlineCount, theta, rotationScratchBuffer.data(), rotationStride, rows, columns);
    std::fill(profileBuffer.begin(), profileBuffer.begin() + rows, 0.0f);
    float cornerColumns[4];
    float cornerRows[4];
    rotatedOriginalFrameCorners(sampleCount, scanlineCount, rows, columns, theta, cornerColumns, cornerRows);

    for (unsigned int row = 0; row < rows; row++) {
      int leftBound = 0;
      int rightBound = 0;
      if (!rotatedRowColumnBoundsFromCorners(cornerColumns, cornerRows, columns, static_cast<int>(row), &leftBound, &rightBound)) {
        continue;
      }

      int leftColumn = leftBound;
      int rightColumn = rightBound;
      if (kNeedleInsertionSideRight) {
        rightColumn = rightBound;
        leftColumn = std::max(leftBound, rightColumn - static_cast<int>(needleLengthRotated) + 1);
      } else {
        leftColumn = leftBound;
        rightColumn = std::min(rightBound, leftColumn + static_cast<int>(needleLengthRotated) - 1);
      }

      profileBuffer[row] = lineBandSum(
        rotationScratchBuffer.data(),
        rows,
        columns,
        rotationStride,
        static_cast<int>(row),
        leftColumn,
        rightColumn
      );
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

    std::array<int, kTopLineCandidateRowsPerAngle> selectedRows;
    selectedRows.fill(-1);
    const int rowsToEvaluate = std::min(kTopLineCandidateRowsPerAngle, static_cast<int>(rows));
    for (int candidateIndex = 0; candidateIndex < rowsToEvaluate; candidateIndex++) {
      float localBest = -std::numeric_limits<float>::max();
      int localBestRow = -1;

      for (unsigned int row = 0; row < rows; row++) {
        bool alreadySelected = false;
        for (int selectedIndex = 0; selectedIndex < candidateIndex; selectedIndex++) {
          if (selectedRows[static_cast<size_t>(selectedIndex)] == static_cast<int>(row)) {
            alreadySelected = true;
            break;
          }
        }

        if (!alreadySelected && profileBuffer[row] > localBest) {
          localBest = profileBuffer[row];
          localBestRow = static_cast<int>(row);
        }
      }

      if (localBestRow >= 0 && localBest > 0.0f) {
        selectedRows[static_cast<size_t>(candidateIndex)] = localBestRow;
        LineCandidate candidate;
        if (
          evaluateLineCandidateOnCurrentRotation(
            rotationScratchBuffer.data(),
            sampleCount,
            scanlineCount,
            rows,
            columns,
            rotationStride,
            theta,
            cornerColumns,
            cornerRows,
            localBestRow,
            localBest,
            &candidate
          )
        ) {
          insertTopLineCandidate(topCandidates, candidate, rotationScratchBuffer.data(), rotationStride);
        }
      }
    }
  }

  LineCandidate selectedCandidate;
  float bestSegmentMean = -std::numeric_limits<float>::max();
  for (const auto& candidate : topCandidates) {
    if (candidate.valid && candidate.segmentMean > bestSegmentMean) {
      bestSegmentMean = candidate.segmentMean;
      selectedCandidate = candidate;
    }
  }

  float* bestRotation = nullptr;
  if (
    selectedCandidate.valid &&
    selectedCandidate.rotationBufferIndex >= 0 &&
    selectedCandidate.rotationBufferIndex < kTopLineCandidateCount
  ) {
    bestRotation = topLineRotationBuffers[static_cast<size_t>(selectedCandidate.rotationBufferIndex)].data();
  } else if (bestRows >= 1 && bestColumns >= 1 && bestRows <= rotationStride && bestColumns <= rotationStride) {
    selectedCandidate.valid = true;
    selectedCandidate.scoreSum = bestSum;
    selectedCandidate.thetaDeg = bestTheta;
    selectedCandidate.rho = bestRho;
    selectedCandidate.rotatedRows = bestRows;
    selectedCandidate.rotatedColumns = bestColumns;
    bestRotation = bestRotationBuffer.data();

    int leftBound = 0;
    int rightBound = 0;
    if (rotatedRowOriginalColumnBounds(sampleCount, scanlineCount, bestRows, bestColumns, bestTheta, bestRho, &leftBound, &rightBound)) {
      selectedCandidate.entryX = kNeedleInsertionSideRight ? rightBound : leftBound;
    } else {
      selectedCandidate.entryX = kNeedleInsertionSideRight ? static_cast<int>(bestColumns - 1) : 0;
    }
    selectedCandidate.tipX = findTipXAlongBestLine(bestRotation, bestRows, bestColumns, rotationStride, bestRho);
  }

  if (
    !selectedCandidate.valid ||
    bestRotation == nullptr ||
    selectedCandidate.rotatedRows < 1 ||
    selectedCandidate.rotatedColumns < 1 ||
    selectedCandidate.rotatedRows > rotationStride ||
    selectedCandidate.rotatedColumns > rotationStride
  ) {
    return result;
  }

  *rotatedRows = selectedCandidate.rotatedRows;
  *rotatedColumns = selectedCandidate.rotatedColumns;

  result.bestRho = selectedCandidate.rho;
  result.thetaDeg = selectedCandidate.thetaDeg;
  result.entryX = selectedCandidate.entryX;
  result.entryY = selectedCandidate.rho;
  result.tipX = selectedCandidate.tipX;
  if (result.tipX <= 0) {
    result.tipX = findTipXAlongBestLine(
      bestRotation,
      selectedCandidate.rotatedRows,
      selectedCandidate.rotatedColumns,
      rotationStride,
      result.bestRho
    );
  }
  result.tipY = selectedCandidate.rho;

  int startRow = std::max(0, result.tipY - kRefinementOffset);
  int endRow = std::min(static_cast<int>(selectedCandidate.rotatedRows - 1), result.tipY + kRefinementOffset);
  float sumProfile[2 * kRefinementOffset + 1] = {0.0f};
  int nonZeroPixels[2 * kRefinementOffset + 1] = {0};

  int x0 = std::min(result.tipX, result.entryX);
  int x1 = std::max(result.tipX, result.entryX);
  x0 = clampInt(x0, 0, static_cast<int>(selectedCandidate.rotatedColumns - 1));
  x1 = clampInt(x1, 0, static_cast<int>(selectedCandidate.rotatedColumns - 1));

  float bestMean = -1.0f;
  int refinedRho = result.bestRho;
  for (int column = x0; column <= x1; column++) {
    const size_t columnOffset = static_cast<size_t>(column) * rotationStride;
    for (int row = startRow; row <= endRow; row++) {
      for (int deltaRow = -kNeedleLineHalfBand; deltaRow <= kNeedleLineHalfBand; deltaRow++) {
        const int bandRow = row + deltaRow;
        if (bandRow >= 0 && bandRow < static_cast<int>(selectedCandidate.rotatedRows)) {
          const float value = bestRotation[columnOffset + bandRow];
          if (value > 0.0f) {
            sumProfile[row - startRow] += value;
            nonZeroPixels[row - startRow]++;
          }
        }
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
    result.tipX = findTipXAlongBestLine(
      bestRotation,
      selectedCandidate.rotatedRows,
      selectedCandidate.rotatedColumns,
      rotationStride,
      refinedRho
    );
  }

  if (
    entryIsTooDeep(
      sampleCount,
      scanlineCount,
      selectedCandidate.rotatedRows,
      selectedCandidate.rotatedColumns,
      result.thetaDeg,
      result.entryX,
      result.entryY
    )
  ) {
    resetTipMotionState();
    return result;
  }

  if (needleTipMotionState.hasLine) {
    const float thetaJump = angleDifferenceDeg(result.thetaDeg, needleTipMotionState.thetaDeg);
    const int rhoJump = std::abs(result.bestRho - needleTipMotionState.rho);
    if (thetaJump > kMotionTrackLineThetaJumpDeg || rhoJump > kMotionTrackLineRhoJumpPx) {
      resetTipMotionState();
    }
  }

  float motionTip = static_cast<float>(result.tipX);
  bool tryMotionCorrection = false;
  if (needleTipMotionState.hasMotionTrack) {
    tryMotionCorrection = true;
  } else if (needleTipMotionState.hasTip) {
    const float profileTipJump = std::fabs(static_cast<float>(result.tipX) - needleTipMotionState.tipX);
    tryMotionCorrection = profileTipJump >= kMotionProfileSuspiciousJumpPx;
  }

  if (
    tryMotionCorrection &&
    motionHotspotTipFromDiff(
      motionDifferenceBuffer.data(),
      static_cast<int>(sampleCount),
      static_cast<int>(scanlineCount),
      static_cast<int>(selectedCandidate.rotatedRows),
      static_cast<int>(selectedCandidate.rotatedColumns),
      result.thetaDeg,
      result.bestRho,
      result.tipX,
      result.entryX,
      &motionTip
    )
  ) {
    result.tipX = clampInt(
      static_cast<int>(std::floor(motionTip + 0.5f)),
      0,
      static_cast<int>(selectedCandidate.rotatedColumns - 1)
    );
  }

  needleTipMotionState.hasTip = true;
  needleTipMotionState.tipX = static_cast<float>(result.tipX);
  needleTipMotionState.hasLine = true;
  needleTipMotionState.thetaDeg = result.thetaDeg;
  needleTipMotionState.rho = result.bestRho;

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
