#include "FrameProcessor.h"

#include <cstdio>
#include <filesystem>
#include <mutex>

#include "cvie.h"
#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace {
void fpLog(const char* message) {
#if defined(__ANDROID__)
  __android_log_write(ANDROID_LOG_DEBUG, "NitroFrameProcessor", message);
#else
  std::fprintf(stderr, "[NitroFrameProcessor] %s\n", message);
#endif
}

void fpLogLastLicError(const char* context) {
  char errorBuffer[MAX_ERROR_MESSAGE_LENGTH] = {0};
  CVEMGetLastError(RESERVED_LICERROR_HANDLE, errorBuffer, MAX_ERROR_MESSAGE_LENGTH);
  char logBuffer[512];
  std::snprintf(logBuffer, sizeof(logBuffer), "%s | CVLM error: %s", context, errorBuffer);
  fpLog(logBuffer);
}

struct FrameProcessorState {
  HCVIE handle = nullptr;
  std::mutex mutex;
};
} // namespace

bool FrameProcessorActivateLicense(const char* activationKey, const char* deviceId) {
  fpLog("FrameProcessorActivateLicense called");
  if (activationKey == nullptr || deviceId == nullptr) {
    fpLog("FrameProcessorActivateLicense failed: null activationKey/deviceId");
    return false;
  }

  if (CVLMSetParameterValue(LM_INIT, deviceId) != CVIE_E_OK) {
    fpLog("FrameProcessorActivateLicense failed: CVLMSetParameterValue");
    fpLogLastLicError("FrameProcessorActivateLicense CVLMSetParameterValue");
    return false;
  }

  const char** productIds = nullptr;
  if (CVLMGetPossibleModules(nullptr, &productIds) != CVIE_E_OK || productIds == nullptr) {
    fpLog("FrameProcessorActivateLicense failed: CVLMGetPossibleModules");
    fpLogLastLicError("FrameProcessorActivateLicense CVLMGetPossibleModules");
    return false;
  }

  for (int index = 0; productIds[index] != nullptr; index++) {
    if (productIds[index][0] == '\0') {
      break;
    }

    if (CVLMSetKey(nullptr, index, activationKey) == CVIE_E_OK) {
      fpLog("FrameProcessorActivateLicense success");
      return true;
    }
  }

  fpLog("FrameProcessorActivateLicense failed: no product accepted key");
  fpLogLastLicError("FrameProcessorActivateLicense no product accepted key");
  return false;
}

bool FrameProcessorCreate(void** handleOut) {
  fpLog("FrameProcessorCreate called");
  if (handleOut == nullptr) {
    fpLog("FrameProcessorCreate failed: handleOut is null");
    return false;
  }

  auto* state = new FrameProcessorState();
  if (CVIECreate(&state->handle, 0) != CVIE_E_OK || state->handle == nullptr) {
    fpLog("FrameProcessorCreate failed: CVIECreate");
    delete state;
    return false;
  }

  *handleOut = state;
  fpLog("FrameProcessorCreate success");
  return true;
}

void FrameProcessorDestroy(void* handle) {
  if (handle == nullptr) {
    return;
  }

  fpLog("FrameProcessorDestroy called");
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
  fpLog("FrameProcessorConfigure called");
  if (handle == nullptr || parameterFilePath == nullptr) {
    fpLog("FrameProcessorConfigure failed: handle/parameterFilePath null");
    return false;
  }
  {
    char pathLog[512];
    std::snprintf(pathLog, sizeof(pathLog), "FrameProcessorConfigure parameterFilePath=%s", parameterFilePath);
    fpLog(pathLog);
  }
  {
    std::error_code ec;
    const bool exists = std::filesystem::exists(parameterFilePath, ec) && !ec;
    char existsLog[512];
    std::snprintf(
      existsLog,
      sizeof(existsLog),
      "FrameProcessorConfigure parameterFileExists=%s",
      exists ? "true" : "false"
    );
    fpLog(existsLog);
  }

  auto* state = static_cast<FrameProcessorState*>(handle);
  std::lock_guard<std::mutex> guard(state->mutex);

  if (CVIESetThreads(state->handle, threads) != CVIE_E_OK) {
    fpLog("FrameProcessorConfigure failed: CVIESetThreads");
    return false;
  }
  if (CVIESetParameterFile(state->handle, parameterFilePath, nullptr) != CVIE_E_OK) {
    fpLog("FrameProcessorConfigure failed: CVIESetParameterFile");
    return false;
  }
  if (CVIEEnhanceSetup(state->handle, width, height, CVIE_DATA_U8, setting, nullptr) != CVIE_E_OK) {
    fpLog("FrameProcessorConfigure failed: CVIEEnhanceSetup");
    return false;
  }

  fpLog("FrameProcessorConfigure success");
  return true;
}

bool FrameProcessorEnhanceNextU8(
  void* handle,
  const uint8_t* inputBytes,
  uint8_t* outputBytes,
  int setting
) {
  fpLog("FrameProcessorEnhanceNextU8 called");
  if (handle == nullptr || inputBytes == nullptr || outputBytes == nullptr) {
    fpLog("FrameProcessorEnhanceNextU8 failed: null argument");
    return false;
  }

  auto* state = static_cast<FrameProcessorState*>(handle);
  std::lock_guard<std::mutex> guard(state->mutex);
  const bool ok = CVIEEnhanceNext(state->handle, inputBytes, outputBytes, setting) == CVIE_E_OK;
  fpLog(ok ? "FrameProcessorEnhanceNextU8 success" : "FrameProcessorEnhanceNextU8 failed: CVIEEnhanceNext");
  return ok;
}
