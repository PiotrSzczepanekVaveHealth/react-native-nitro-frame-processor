import { NitroModules } from 'react-native-nitro-modules';
import type { NitroFrameProcessor } from './NitroFrameProcessor.nitro';

let nitroFrameProcessorHybridObject: NitroFrameProcessor | null | undefined;

function getNitroFrameProcessorHybridObject(): NitroFrameProcessor | null {
  if (nitroFrameProcessorHybridObject !== undefined) {
    return nitroFrameProcessorHybridObject;
  }

  try {
    nitroFrameProcessorHybridObject =
      NitroModules.createHybridObject<NitroFrameProcessor>(
        'NitroFrameProcessor'
      );
  } catch {
    nitroFrameProcessorHybridObject = null;
  }

  return nitroFrameProcessorHybridObject;
}

export function setEnabled(value: boolean): void {
  getNitroFrameProcessorHybridObject()?.setEnabled(value);
}

export function setNumThreads(numThreads: number): void {
  getNitroFrameProcessorHybridObject()?.setNumThreads(numThreads);
}

export function setSetting(setting: number): void {
  getNitroFrameProcessorHybridObject()?.setSetting(setting);
}

export function setParameterFilePath(path: string): void {
  getNitroFrameProcessorHybridObject()?.setParameterFilePath(path);
}

export function activateLicense(
  activationKey: string,
  deviceId: string
): boolean {
  const instance = getNitroFrameProcessorHybridObject();
  if (instance == null) {
    return false;
  }
  return instance.activateLicense(activationKey, deviceId);
}

export function processFrame(
  width: number,
  height: number,
  input: ArrayBuffer
): ArrayBuffer {
  const instance = getNitroFrameProcessorHybridObject();
  if (instance == null) {
    return input;
  }
  return instance.processFrame(width, height, input);
}
