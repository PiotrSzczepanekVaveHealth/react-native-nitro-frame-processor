#include "FrameProcessor.h"

bool FrameProcessorActivateLicense(const char*, const char*) {
  return false;
}

bool FrameProcessorCreate(void** handleOut) {
  if (handleOut != nullptr) {
    *handleOut = nullptr;
  }
  return false;
}

void FrameProcessorDestroy(void*) {}

bool FrameProcessorConfigure(void*, int, const char*, int, int, int) {
  return false;
}

bool FrameProcessorEnhanceNextU8(void*, const uint8_t*, uint8_t*, int) {
  return false;
}
