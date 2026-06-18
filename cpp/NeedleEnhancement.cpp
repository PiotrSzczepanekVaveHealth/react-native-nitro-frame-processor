#include "NeedleEnhancement.hpp"

#include <algorithm>
#include <cstring>
#include <mutex>

extern "C" {
#include "needle-reference/algorithm/needle_enhancement.h"
#include "needle-reference/cmd_central.h"
#include "needle-reference/probe/probe.h"
}

unsigned char frame_buffer[RAW_DATA_SIZE_MAX];
ProbeState probeState = {0};

namespace margelo::nitro::nitroframeprocessor {

namespace {
std::mutex needleEnhancementMutex;

float clampAngle(float degrees) {
  return std::clamp(degrees, 0.0f, 179.0f);
}
} // namespace

void NeedleEnhancement::setEnabled(bool value) {
  std::lock_guard<std::mutex> guard(needleEnhancementMutex);
  if (isEnabled_ != value) {
    resetNeedleEnhancementTemporalState(value ? 2U : 0U);
    frameCounter_ = 0;
  }
  isEnabled_ = value;
}

bool NeedleEnhancement::isEnabled() const {
  return isEnabled_;
}

void NeedleEnhancement::resetTemporalState() {
  resetNeedleEnhancementTemporalState(isEnabled_ ? 2U : 0U);
  frameCounter_ = 0;
}

void NeedleEnhancement::setAngle(float degrees) {
  std::lock_guard<std::mutex> guard(needleEnhancementMutex);
  const float angle = clampAngle(degrees);
  thetaRangeMinDeg_ = angle;
  thetaRangeMaxDeg_ = angle;
  thetaStepDeg_ = DEFAULT_THETA_STEP_DEG;
  resetTemporalState();
}

void NeedleEnhancement::setAngleRange(float minDegrees, float maxDegrees, float stepDegrees) {
  std::lock_guard<std::mutex> guard(needleEnhancementMutex);
  const float lower = clampAngle(minDegrees);
  const float upper = clampAngle(maxDegrees);
  thetaRangeMinDeg_ = std::min(lower, upper);
  thetaRangeMaxDeg_ = std::max(lower, upper);
  thetaStepDeg_ = stepDegrees > 0.0f ? stepDegrees : DEFAULT_THETA_STEP_DEG;
  resetTemporalState();
}

void NeedleEnhancement::setNeedleLengthPx(int needleLengthPx) {
  std::lock_guard<std::mutex> guard(needleEnhancementMutex);
  needleLengthPx_ = std::max(0, needleLengthPx);
  resetTemporalState();
}

void NeedleEnhancement::setDepthMask(bool maskSkinLayer, unsigned int depthMaskThicknessPx) {
  std::lock_guard<std::mutex> guard(needleEnhancementMutex);
  maskSkinLayer_ = maskSkinLayer;
  depthMaskThicknessPx_ = depthMaskThicknessPx;
  resetTemporalState();
}

void NeedleEnhancement::setPipParams(
  float thetaStepDeg,
  float thetaRangeMinDeg,
  float thetaRangeMaxDeg,
  float resizeFactor,
  bool normalize
) {
  std::lock_guard<std::mutex> guard(needleEnhancementMutex);
  const float lower = clampAngle(thetaRangeMinDeg);
  const float upper = clampAngle(thetaRangeMaxDeg);
  thetaStepDeg_ = thetaStepDeg > 0.0f ? thetaStepDeg : DEFAULT_THETA_STEP_DEG;
  thetaRangeMinDeg_ = std::min(lower, upper);
  thetaRangeMaxDeg_ = std::max(lower, upper);
  resizeFactor_ = resizeFactor > 0.0f ? resizeFactor : DEFAULT_RESIZE_FACTOR;
  normalize_ = normalize;
  resetTemporalState();
}

void NeedleEnhancement::setInsertionSideRight(bool rightSide) {
  std::lock_guard<std::mutex> guard(needleEnhancementMutex);
  insertionSideRight_ = rightSide;
  needle_insertion_side_right = rightSide ? 1 : 0;
  resetTemporalState();
}

bool NeedleEnhancement::process(uint8_t* frame, int sampleCount, int scanlineCount) {
  if (!isEnabled_) {
    return true;
  }
  if (frame == nullptr || sampleCount <= 0 || scanlineCount <= 0) {
    return false;
  }

  std::lock_guard<std::mutex> guard(needleEnhancementMutex);

  const auto pixelCount = static_cast<unsigned int>(sampleCount);
  const auto beamCount = static_cast<unsigned int>(scanlineCount);
  const size_t rawSize = static_cast<size_t>(pixelCount) * beamCount;
  if (
    pixelCount > MAX_SAMPLE_SIZE_NEEDLE ||
    beamCount > MAX_SCANLINES_NEEDLE ||
    rawSize > static_cast<size_t>(RAW_DATA_SIZE_MAX)
  ) {
    return false;
  }

  probeState.depth = pixelCount;
  std::memcpy(frame_buffer, frame, rawSize);

  DepthMaskParams depthParams = {
    maskSkinLayer_,
    depthMaskThicknessPx_
  };
  NeedlePipParams pipParams = {
    thetaStepDeg_,
    thetaRangeMinDeg_,
    thetaRangeMaxDeg_,
    resizeFactor_,
    normalize_
  };
  NeedleLineResult line = {};
  const int result = needle_enhance_process(
    0,
    frameCounter_++,
    pixelCount,
    beamCount,
    needleLengthPx_,
    &depthParams,
    &pipParams,
    &line
  );

  if (result > 0 && line.detected) {
    std::memcpy(frame, frame_buffer, rawSize);
  }

  return true;
}

} // namespace margelo::nitro::nitroframeprocessor
