#pragma once

#include <cstdint>

namespace margelo::nitro::nitroframeprocessor {

class NeedleEnhancement final {
public:
  void setEnabled(bool value);
  bool isEnabled() const;
  void setAngle(float degrees);
  void setAngleRange(float minDegrees, float maxDegrees, float stepDegrees);
  void setNeedleLengthPx(int needleLengthPx);
  void setDepthMask(bool maskSkinLayer, unsigned int depthMaskThicknessPx);
  void setPipParams(
    float thetaStepDeg,
    float thetaRangeMinDeg,
    float thetaRangeMaxDeg,
    float resizeFactor,
    bool normalize
  );

  bool process(uint8_t* frame, int sampleCount, int scanlineCount);

private:
  void resetTemporalState();

  static constexpr float DEFAULT_THETA_RANGE_MIN_DEG = 4.0f;
  static constexpr float DEFAULT_THETA_RANGE_MAX_DEG = 35.0f;
  static constexpr float DEFAULT_THETA_STEP_DEG = 2.0f;
  static constexpr float DEFAULT_RESIZE_FACTOR = 1.0f;
  static constexpr bool DEFAULT_NORMALIZE = false;
  static constexpr int DEFAULT_NEEDLE_LENGTH_PX = 100;
  static constexpr bool DEFAULT_MASK_SKIN_LAYER = false;
  static constexpr unsigned int DEFAULT_DEPTH_MASK_THICKNESS_PX = 8;

  bool isEnabled_ = false;
  int needleLengthPx_ = DEFAULT_NEEDLE_LENGTH_PX;
  bool maskSkinLayer_ = DEFAULT_MASK_SKIN_LAYER;
  unsigned int depthMaskThicknessPx_ = DEFAULT_DEPTH_MASK_THICKNESS_PX;
  float thetaStepDeg_ = DEFAULT_THETA_STEP_DEG;
  float thetaRangeMinDeg_ = DEFAULT_THETA_RANGE_MIN_DEG;
  float thetaRangeMaxDeg_ = DEFAULT_THETA_RANGE_MAX_DEG;
  float resizeFactor_ = DEFAULT_RESIZE_FACTOR;
  bool normalize_ = DEFAULT_NORMALIZE;
  int frameCounter_ = 0;
};

} // namespace margelo::nitro::nitroframeprocessor
