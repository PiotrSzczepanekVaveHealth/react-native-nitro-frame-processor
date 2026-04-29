import type { HybridObject } from 'react-native-nitro-modules';

export interface NitroFrameProcessor extends HybridObject<{
  ios: 'c++';
  android: 'c++';
}> {
  setEnabled(value: boolean): void;
  setNumThreads(numThreads: number): void;
  setSetting(setting: number): void;
  setParameterFilePath(path: string): void;
  activateLicense(activationKey: string, deviceId: string): boolean;
  processFrame(input: ArrayBuffer): ArrayBuffer;
}
