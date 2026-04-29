#include "NitroFrameProcessorCpp.hpp"

#include <algorithm>
#include <vector>

namespace margelo::nitro::nitroframeprocessor {

namespace {
size_t getHeaderLength(const uint8_t* message, size_t messageLength) {
  if (message == nullptr || messageLength <= 8) {
    return 0;
  }
  return static_cast<size_t>(message[8]);
}

size_t getTrailerLength(const uint8_t* message, size_t messageLength) {
  if (message == nullptr || messageLength <= 10) {
    return 0;
  }
  return static_cast<size_t>(message[9]) + (static_cast<size_t>(message[10]) * 256U);
}

size_t getDepth(const uint8_t* message, size_t messageLength) {
  if (message == nullptr || messageLength <= 13) {
    return 0;
  }
  const size_t low = static_cast<size_t>(message[13]);
  const size_t high = messageLength > 14 ? static_cast<size_t>(message[14]) : 0;
  return low + (high * 256U);
}

size_t getBeamCount(
  const uint8_t* message,
  size_t messageLength,
  size_t headerLength,
  size_t depth,
  size_t trailerLength
) {
  if (message == nullptr) {
    return 0;
  }
  if (headerLength == 33) {
    return messageLength > 24 ? static_cast<size_t>(message[24]) : 0;
  }
  if (depth == 0 || messageLength < headerLength + trailerLength) {
    return 0;
  }
  return (messageLength - headerLength - trailerLength) / depth;
}
}

NitroFrameProcessorCpp::NitroFrameProcessorCpp() : HybridObject(TAG) {}

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
  parameterFilePath_ = path;
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

  const size_t headerLength = getHeaderLength(inputData, inputSize);
  const size_t trailerLength = getTrailerLength(inputData, inputSize);
  const size_t depth = getDepth(inputData, inputSize);
  const size_t beamCount = getBeamCount(inputData, inputSize, headerLength, depth, trailerLength);

  const int frameWidth = static_cast<int>(depth);
  const int frameHeight = static_cast<int>(beamCount);
  if (frameWidth <= 0 || frameHeight <= 0) {
    return input;
  }

  const size_t rawLength = beamCount * depth;
  const size_t rawStart = headerLength;
  const size_t rawEnd = rawStart + rawLength;
  if (rawLength == 0 || rawEnd > inputSize) {
    return input;
  }

  if (!ensureConfigured(frameWidth, frameHeight)) {
    return input;
  }

  std::vector<uint8_t> processed(rawLength);
  if (!FrameProcessorEnhanceNextU8(handle_, inputData + rawStart, processed.data(), setting_)) {
    return input;
  }
  if (processed.size() != rawLength) {
    return input;
  }

  std::vector<uint8_t> out(inputData, inputData + inputSize);
  std::copy(processed.begin(), processed.end(), out.begin() + static_cast<long>(rawStart));
  return ArrayBuffer::copy(out);
}

bool NitroFrameProcessorCpp::ensureConfigured(int width, int height) {
  const bool needsRecreate = handle_ == nullptr || previousWidth_ != width || previousHeight_ != height ||
    previousSetting_ != setting_ || previousParameterFilePath_ != parameterFilePath_;
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
  previousParameterFilePath_ = parameterFilePath_;
  return true;
}

} // namespace margelo::nitro::nitroframeprocessor
