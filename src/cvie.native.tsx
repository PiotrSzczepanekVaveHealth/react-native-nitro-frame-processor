import { NitroModules } from 'react-native-nitro-modules';
import type { NitroFrameProcessor } from './NitroFrameProcessor.nitro';

const nitroFrameProcessorHybridObject =
  NitroModules.createHybridObject<NitroFrameProcessor>('NitroFrameProcessor');

export function setEnabled(value: boolean): void {
  nitroFrameProcessorHybridObject.setEnabled(value);
}

export function setNumThreads(numThreads: number): void {
  nitroFrameProcessorHybridObject.setNumThreads(numThreads);
}

export function setSetting(setting: number): void {
  nitroFrameProcessorHybridObject.setSetting(setting);
}

export function setParameterFilePath(path: string): void {
  nitroFrameProcessorHybridObject.setParameterFilePath(path);
}

export function activateLicense(
  activationKey: string,
  deviceId: string
): boolean {
  return nitroFrameProcessorHybridObject.activateLicense(
    activationKey,
    deviceId
  );
}

export function processImage(
  width: number,
  height: number,
  input: ArrayBuffer
): ArrayBuffer {
  return nitroFrameProcessorHybridObject.processImage(width, height, input);
}
