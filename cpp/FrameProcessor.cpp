#include "FrameProcessor.h"

#include <mutex>

#include "cvie.h"

namespace {
struct FrameProcessorState {
  HCVIE handle = nullptr;
  std::mutex mutex;
};
} // namespace

bool FrameProcessorActivateLicense(const char* activationKey, const char* deviceId) {
  if (activationKey == nullptr || deviceId == nullptr) {
    return false;
  }

  if (CVLMSetParameterValue(LM_INIT, deviceId) != CVIE_E_OK) {
    return false;
  }

  const char** productIds = nullptr;
  if (CVLMGetPossibleModules(nullptr, &productIds) != CVIE_E_OK || productIds == nullptr) {
    return false;
  }

  for (int index = 0; productIds[index] != nullptr; index++) {
    if (productIds[index][0] == '\0') {
      break;
    }

    if (CVLMSetKey(nullptr, index, activationKey) == CVIE_E_OK) {
      return true;
    }
  }

  return false;
}

bool FrameProcessorCreate(void** handleOut) {
  if (handleOut == nullptr) {
    return false;
  }

  auto* state = new FrameProcessorState();
  if (CVIECreate(&state->handle, 0) != CVIE_E_OK || state->handle == nullptr) {
    delete state;
    return false;
  }

  *handleOut = state;
  return true;
}

void FrameProcessorDestroy(void* handle) {
  if (handle == nullptr) {
    return;
  }

  auto* state = static_cast<FrameProcessorState*>(handle);
  if (state->handle != nullptr) {
    CVIEDestroy(&state->handle);
  }
  delete state;
}

bool FrameProcessorConfigure(
  void* handle,
  int threads,
  const char* parameterFilePath,
  int width,
  int height,
  int setting
) {
  if (handle == nullptr || parameterFilePath == nullptr) {
    return false;
  }

  auto* state = static_cast<FrameProcessorState*>(handle);
  std::lock_guard<std::mutex> guard(state->mutex);

  if (CVIESetThreads(state->handle, threads) != CVIE_E_OK) {
    return false;
  }
  if (CVIESetParameterFile(state->handle, parameterFilePath, nullptr) != CVIE_E_OK) {
    return false;
  }
  if (CVIEEnhanceSetup(state->handle, width, height, CVIE_DATA_U8, setting, nullptr) != CVIE_E_OK) {
    return false;
  }

  return true;
}

bool FrameProcessorEnhanceNextU8(
  void* handle,
  const uint8_t* inputBytes,
  uint8_t* outputBytes,
  int setting
) {
  if (handle == nullptr || inputBytes == nullptr || outputBytes == nullptr) {
    return false;
  }

  auto* state = static_cast<FrameProcessorState*>(handle);
  std::lock_guard<std::mutex> guard(state->mutex);
  return CVIEEnhanceNext(state->handle, inputBytes, outputBytes, setting) == CVIE_E_OK;
}
