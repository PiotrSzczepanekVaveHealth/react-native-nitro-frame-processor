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

  bool process(uint8_t* frame, int sampleCount, int scanlineCount);

private:
  struct LineResult;

  LineResult detectNeedleLine(
    const uint8_t* input,
    unsigned int sampleCount,
    unsigned int scanlineCount,
    unsigned int rotationStride,
    unsigned int* rotatedRows,
    unsigned int* rotatedColumns
  );
  void applyDepthMask(unsigned int sampleCount, unsigned int scanlineCount, int needleLengthPx);
  bool fuseNeedleMask(
    uint8_t* frame,
    unsigned int sampleCount,
    unsigned int scanlineCount,
    const LineResult& line,
    unsigned int rotatedRows,
    unsigned int rotatedColumns
  );

private:
  static constexpr float DEFAULT_THETA_RANGE_MIN_DEG = 25.0f;
  static constexpr float DEFAULT_THETA_RANGE_MAX_DEG = 35.0f;
  static constexpr float DEFAULT_THETA_STEP_DEG = 2.0f;
  static constexpr int DEFAULT_NEEDLE_LENGTH_PX = 100;

  bool isEnabled_ = false;
  float thetaRangeMinDeg_ = DEFAULT_THETA_RANGE_MIN_DEG;
  float thetaRangeMaxDeg_ = DEFAULT_THETA_RANGE_MAX_DEG;
  float thetaStepDeg_ = DEFAULT_THETA_STEP_DEG;
  int needleLengthPx_ = DEFAULT_NEEDLE_LENGTH_PX;
};

} // namespace margelo::nitro::nitroframeprocessor
