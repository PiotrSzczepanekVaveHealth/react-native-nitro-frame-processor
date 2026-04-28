#import "CVIEBridge.h"
#import <cstring>
#import "CVIESDK/include/cvie.h"

extern "C" {

BOOL CVIEBridgeActivateLicense(const char* activationKey, const char* deviceId) {
  if (activationKey == nullptr || deviceId == nullptr) {
    return NO;
  }

  if (CVLMSetParameterValue(LM_INIT, deviceId) != CVIE_E_OK) {
    return NO;
  }

  const char** productIds = nullptr;
  if (CVLMGetPossibleModules(nullptr, &productIds) != CVIE_E_OK || productIds == nullptr) {
    return NO;
  }

  for (int index = 0; productIds[index] != nullptr; index++) {
    if (productIds[index][0] == '\0') {
      break;
    }

    if (CVLMSetKey(nullptr, index, activationKey) == CVIE_E_OK) {
      return YES;
    }
  }

  return NO;
}

BOOL CVIEBridgeCreate(void** handleOut) {
  if (handleOut == nullptr) {
    return NO;
  }

  HCVIE handle = nullptr;
  if (CVIECreate(&handle, 0) != CVIE_E_OK || handle == nullptr) {
    return NO;
  }

  *handleOut = handle;
  return YES;
}

void CVIEBridgeDestroy(void* handle) {
  if (handle == nullptr) {
    return;
  }

  HCVIE cvieHandle = handle;
  CVIEDestroy(&cvieHandle);
}

BOOL CVIEBridgeConfigure(
  void* handle,
  int threads,
  const char* parameterFilePath,
  int width,
  int height,
  int setting
) {
  if (handle == nullptr || parameterFilePath == nullptr) {
    return NO;
  }

  HCVIE cvieHandle = handle;
  if (CVIESetThreads(cvieHandle, threads) != CVIE_E_OK) {
    return NO;
  }
  if (CVIESetParameterFile(cvieHandle, parameterFilePath, nullptr) != CVIE_E_OK) {
    return NO;
  }
  if (CVIEEnhanceSetup(cvieHandle, width, height, CVIE_DATA_U8, setting, nullptr) != CVIE_E_OK) {
    return NO;
  }

  return YES;
}

BOOL CVIEBridgeEnhanceNextU8(
  void* handle,
  const uint8_t* inputBytes,
  uint8_t* outputBytes,
  int setting
) {
  if (handle == nullptr || inputBytes == nullptr || outputBytes == nullptr) {
    return NO;
  }

  HCVIE cvieHandle = handle;
  return CVIEEnhanceNext(cvieHandle, inputBytes, outputBytes, setting) == CVIE_E_OK;
}

} // extern "C"
