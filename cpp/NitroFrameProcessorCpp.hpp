#pragma once

#include "FrameProcessor.h"
#include "HybridNitroFrameProcessorSpec.hpp"

#include <string>

namespace margelo::nitro::nitroframeprocessor {

class NitroFrameProcessorCpp final : public HybridNitroFrameProcessorSpec {
public:
  NitroFrameProcessorCpp();
  ~NitroFrameProcessorCpp() override;

public:
  void setEnabled(bool value) override;
  void setNumThreads(double numThreads) override;
  void setSetting(double setting) override;
  void setParameterFilePath(const std::string& path) override;
  bool activateLicense(const std::string& activationKey, const std::string& deviceId) override;
  std::shared_ptr<ArrayBuffer> processFrame(const std::shared_ptr<ArrayBuffer>& input) override;

private:
  bool ensureConfigured(int width, int height);

private:
  bool isEnabled_ = true;
  int setting_ = 0;
  int numThreads_ = 1;
  std::string parameterFilePath_;
  bool isLicenseActivated_ = false;
  void* handle_ = nullptr;

  int previousWidth_ = -1;
  int previousHeight_ = -1;
  int previousSetting_ = -1;
  std::string previousParameterFilePath_;
};

} // namespace margelo::nitro::nitroframeprocessor
