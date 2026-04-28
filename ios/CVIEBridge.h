#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

#ifdef __cplusplus
extern "C" {
#endif

FOUNDATION_EXPORT BOOL CVIEBridgeActivateLicense(const char* activationKey, const char* deviceId);
FOUNDATION_EXPORT BOOL CVIEBridgeCreate(void** handleOut);
FOUNDATION_EXPORT void CVIEBridgeDestroy(void* _Nullable handle);
FOUNDATION_EXPORT BOOL CVIEBridgeConfigure(
  void* handle,
  int threads,
  const char* parameterFilePath,
  int width,
  int height,
  int setting
);
FOUNDATION_EXPORT BOOL CVIEBridgeEnhanceNextU8(
  void* handle,
  const uint8_t* inputBytes,
  uint8_t* outputBytes,
  int setting
);

#ifdef __cplusplus
}
#endif

NS_ASSUME_NONNULL_END
