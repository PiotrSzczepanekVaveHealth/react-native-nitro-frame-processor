#import "CVIEBridge.h"
#import "FrameProcessor.h"

extern "C" {

BOOL CVIEBridgeActivateLicense(const char* activationKey, const char* deviceId) {
  return FrameProcessorActivateLicense(activationKey, deviceId);
}

BOOL CVIEBridgeCreate(void** handleOut) {
  return FrameProcessorCreate(handleOut);
}

void CVIEBridgeDestroy(void* handle) {
  FrameProcessorDestroy(handle);
}

BOOL CVIEBridgeConfigure(
  void* handle,
  int threads,
  const char* parameterFilePath,
  int width,
  int height,
  int setting
) {
  return FrameProcessorConfigure(handle, threads, parameterFilePath, width, height, setting);
}

BOOL CVIEBridgeEnhanceNextU8(
  void* handle,
  const uint8_t* inputBytes,
  uint8_t* outputBytes,
  int setting
) {
  return FrameProcessorEnhanceNextU8(handle, inputBytes, outputBytes, setting);
}

} // extern "C"
