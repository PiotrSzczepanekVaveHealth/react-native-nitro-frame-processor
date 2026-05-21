#include "NitroFrameProcessorCpp.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <vector>

namespace margelo::nitro::nitroframeprocessor {

namespace {
int getDefaultNumThreads() {
  const unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
  return hardwareConcurrency > 0 ? static_cast<int>(hardwareConcurrency) : 4;
}

std::string resolveParameterFilePath(const std::string& path) {
  if (path.empty()) {
    return path;
  }

  std::error_code ec;
  const std::filesystem::path inputPath(path);
  if (inputPath.is_absolute()) {
    return path;
  }

  const char* licenseDir = std::getenv("COV_LICENSE_LOCATION");
  if (licenseDir == nullptr || licenseDir[0] == '\0') {
    return path;
  }

  std::filesystem::path resolvedPath = std::filesystem::path(licenseDir) / inputPath.filename();
  if (std::filesystem::exists(resolvedPath, ec) && !ec) {
    return resolvedPath.string();
  }

  resolvedPath = std::filesystem::path(licenseDir) / inputPath;
  return resolvedPath.string();
}

struct ParsedFrameLayout {
  size_t headerLength = 0;
  size_t trailerLength = 0;
  size_t depth = 0;
  size_t beamCount = 0;
  size_t rawStart = 0;
  size_t rawLength = 0;
};

bool parseFrameLayout(const uint8_t* message, size_t messageLength, ParsedFrameLayout* outLayout) {
  if (message == nullptr || outLayout == nullptr || messageLength == 0) {
    return false;
  }

  const size_t headerLength = messageLength > 8 ? static_cast<size_t>(message[8]) : 0;
  const size_t trailerLength = messageLength > 10
    ? static_cast<size_t>(message[9]) + (static_cast<size_t>(message[10]) * 256U)
    : 0;
  const size_t depth = messageLength > 13
    ? static_cast<size_t>(message[13]) + ((messageLength > 14 ? static_cast<size_t>(message[14]) : 0) * 256U)
    : 0;
  if (depth == 0) {
    return false;
  }

  size_t beamCount = 0;
  if (headerLength == 33) {
    beamCount = messageLength > 24 ? static_cast<size_t>(message[24]) : 0;
  } else {
    if (headerLength + trailerLength > messageLength) {
      return false;
    }
    beamCount = (messageLength - headerLength - trailerLength) / depth;
  }
  if (beamCount == 0) {
    return false;
  }

  const size_t rawLength = beamCount * depth;
  const size_t rawStart = headerLength;
  if (rawStart + rawLength > messageLength) {
    return false;
  }

  outLayout->headerLength = headerLength;
  outLayout->trailerLength = trailerLength;
  outLayout->depth = depth;
  outLayout->beamCount = beamCount;
  outLayout->rawStart = rawStart;
  outLayout->rawLength = rawLength;
  return true;
}
}

NitroFrameProcessorCpp::NitroFrameProcessorCpp()
  : HybridObject(TAG), numThreads_(getDefaultNumThreads()) {}

NitroFrameProcessorCpp::~NitroFrameProcessorCpp() {
  if (handle_ != nullptr) {
    FrameProcessorDestroy(handle_);
    handle_ = nullptr;
  }
}

void NitroFrameProcessorCpp::setEnabled(bool value) {
  isEnabled_ = value;
}

void NitroFrameProcessorCpp::setNumThreads(double numThreads) {
  numThreads_ = std::max(1, static_cast<int>(numThreads));
}

void NitroFrameProcessorCpp::setSetting(double setting) {
  setting_ = static_cast<int>(setting);
}

void NitroFrameProcessorCpp::setParameterFilePath(const std::string& path) {
  parameterFilePath_ = resolveParameterFilePath(path);
}

bool NitroFrameProcessorCpp::activateLicense(
  const std::string& activationKey,
  const std::string& deviceId
) {
  isLicenseActivated_ = FrameProcessorActivateLicense(activationKey.c_str(), deviceId.c_str());
  return isLicenseActivated_;
}

std::shared_ptr<ArrayBuffer> NitroFrameProcessorCpp::processFrame(const std::shared_ptr<ArrayBuffer>& input) {
  if (!isEnabled_ || input == nullptr || input->data() == nullptr) {
    return input;
  }
  if (parameterFilePath_.empty()) {
    return input;
  }

  const auto* inputData = input->data();
  const size_t inputSize = input->size();
  if (inputSize == 0) {
    return input;
  }

  ParsedFrameLayout layout;
  if (!parseFrameLayout(inputData, inputSize, &layout)) {
    return input;
  }

  const int frameWidth = static_cast<int>(layout.depth);
  const int frameHeight = static_cast<int>(layout.beamCount);
  if (frameWidth <= 0 || frameHeight <= 0) {
    return input;
  }

  if (!ensureConfigured(frameWidth, frameHeight)) {
    return input;
  }

  auto output = ArrayBuffer::copy(input);
  auto* outputData = output != nullptr ? output->data() : nullptr;
  if (outputData == nullptr) {
    return input;
  }

  uint8_t* rawOutput = outputData + layout.rawStart;
  if (!FrameProcessorEnhanceNextU8(handle_, rawOutput, rawOutput, setting_)) {
    return input;
  }

  return output;
}

bool NitroFrameProcessorCpp::ensureConfigured(int width, int height) {
  const bool needsRecreate = handle_ == nullptr || previousWidth_ != width || previousHeight_ != height ||
    previousSetting_ != setting_ || previousNumThreads_ != numThreads_ ||
    previousParameterFilePath_ != parameterFilePath_;
  if (!needsRecreate) {
    return true;
  }

  if (handle_ != nullptr) {
    FrameProcessorDestroy(handle_);
    handle_ = nullptr;
  }
  if (!isLicenseActivated_) {
    return false;
  }
  if (!FrameProcessorCreate(&handle_) || handle_ == nullptr) {
    return false;
  }
  if (!FrameProcessorConfigure(handle_, numThreads_, parameterFilePath_.c_str(), width, height, setting_)) {
    FrameProcessorDestroy(handle_);
    handle_ = nullptr;
    return false;
  }

  previousWidth_ = width;
  previousHeight_ = height;
  previousSetting_ = setting_;
  previousNumThreads_ = numThreads_;
  previousParameterFilePath_ = parameterFilePath_;
  return true;
}

} // namespace margelo::nitro::nitroframeprocessor
