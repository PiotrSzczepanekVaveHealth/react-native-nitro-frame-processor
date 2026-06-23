import { NitroModules } from 'react-native-nitro-modules';
import type { NitroFrameProcessor } from './NitroFrameProcessor.nitro';

let nitroFrameProcessorHybridObject: NitroFrameProcessor | null | undefined;
let isVerbose = false;

type VerboseParams = Record<string, boolean | number | string>;

function logVerbose(methodName: string, params: VerboseParams): void {
  if (!isVerbose) {
    return;
  }

  // Keep this log in TypeScript so it is visible in the React Native console.
  console.log(`[NeedleEnhancement] ${methodName}`, params);
}

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

export function setVerbose(value: boolean): void {
  isVerbose = value;
  logVerbose('setVerbose', { value });
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

export function setNeedleEnhancementEnabled(value: boolean): void {
  logVerbose('setNeedleEnhancementEnabled', { value });
  getNitroFrameProcessorHybridObject()?.setNeedleEnhancementEnabled(value);
}

export function setNeedleEnhancementFuseMode(mode: 1 | 2): void {
  logVerbose('setNeedleEnhancementFuseMode', { mode });
  getNitroFrameProcessorHybridObject()?.setNeedleEnhancementFuseMode(mode);
}

export function setNeedleEnhancementAngle(degrees: number): void {
  logVerbose('setNeedleEnhancementAngle', { degrees });
  getNitroFrameProcessorHybridObject()?.setNeedleEnhancementAngle(degrees);
}

export function setNeedleEnhancementAngleRange(
  minDegrees: number,
  maxDegrees: number,
  stepDegrees: number
): void {
  logVerbose('setNeedleEnhancementAngleRange', {
    minDegrees,
    maxDegrees,
    stepDegrees,
  });
  getNitroFrameProcessorHybridObject()?.setNeedleEnhancementAngleRange(
    minDegrees,
    maxDegrees,
    stepDegrees
  );
}

export function setNeedleEnhancementNeedleLength(needleLengthPx: number): void {
  logVerbose('setNeedleEnhancementNeedleLength', { needleLengthPx });
  getNitroFrameProcessorHybridObject()?.setNeedleEnhancementNeedleLength(
    needleLengthPx
  );
}

export function setNeedleEnhancementDepthMask(
  maskSkinLayer: boolean,
  depthMaskThicknessPx: number
): void {
  logVerbose('setNeedleEnhancementDepthMask', {
    maskSkinLayer,
    depthMaskThicknessPx,
  });
  getNitroFrameProcessorHybridObject()?.setNeedleEnhancementDepthMask(
    maskSkinLayer,
    depthMaskThicknessPx
  );
}

export function setNeedleEnhancementPipParams(
  thetaStepDeg: number,
  thetaRangeMinDeg: number,
  thetaRangeMaxDeg: number,
  resizeFactor: number,
  normalize: boolean
): void {
  logVerbose('setNeedleEnhancementPipParams', {
    thetaStepDeg,
    thetaRangeMinDeg,
    thetaRangeMaxDeg,
    resizeFactor,
    normalize,
  });
  getNitroFrameProcessorHybridObject()?.setNeedleEnhancementPipParams(
    thetaStepDeg,
    thetaRangeMinDeg,
    thetaRangeMaxDeg,
    resizeFactor,
    normalize
  );
}

export function setNeedleEnhancementInsertionSide(rightSide: boolean): void {
  logVerbose('setNeedleEnhancementInsertionSide', { rightSide });
  getNitroFrameProcessorHybridObject()?.setNeedleEnhancementInsertionSide(
    rightSide
  );
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

export function processFrame(input: ArrayBuffer): ArrayBuffer {
  const instance = getNitroFrameProcessorHybridObject();
  if (instance == null) {
    return input;
  }
  return instance.processFrame(input);
}

export function resetNeedleEnhancementTemporalState(): void {
  logVerbose('resetNeedleEnhancementTemporalState', {});
  getNitroFrameProcessorHybridObject()?.resetNeedleEnhancementTemporalState();
}

export function processNeedleEnhancementFrame(input: ArrayBuffer): ArrayBuffer {
  logVerbose('processNeedleEnhancementFrame', { byteLength: input.byteLength });
  const instance = getNitroFrameProcessorHybridObject();
  if (instance == null) {
    return input;
  }
  return instance.processNeedleEnhancementFrame(input);
}
