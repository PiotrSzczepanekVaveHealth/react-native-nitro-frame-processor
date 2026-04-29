#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool FrameProcessorActivateLicense(const char* activationKey, const char* deviceId);
bool FrameProcessorCreate(void** handleOut);
void FrameProcessorDestroy(void* handle);
bool FrameProcessorConfigure(
  void* handle,
  int threads,
  const char* parameterFilePath,
  int width,
  int height,
  int setting
);
bool FrameProcessorEnhanceNextU8(
  void* handle,
  const uint8_t* inputBytes,
  uint8_t* outputBytes,
  int setting
);

#ifdef __cplusplus
}
#endif
