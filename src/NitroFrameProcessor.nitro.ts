import type { HybridObject } from 'react-native-nitro-modules';

export interface NitroFrameProcessor extends HybridObject<{
  ios: 'c++';
  android: 'c++';
}> {
  setEnabled(value: boolean): void;
  setNumThreads(numThreads: number): void;
  setSetting(setting: number): void;
  setNeedleEnhancementEnabled(value: boolean): void;
  setNeedleEnhancementAngle(degrees: number): void;
  setNeedleEnhancementAngleRange(
    minDegrees: number,
    maxDegrees: number,
    stepDegrees: number
  ): void;
  setNeedleEnhancementNeedleLength(needleLengthPx: number): void;
  setNeedleEnhancementDepthMask(
    maskSkinLayer: boolean,
    depthMaskThicknessPx: number
  ): void;
  setNeedleEnhancementPipParams(
    thetaStepDeg: number,
    thetaRangeMinDeg: number,
    thetaRangeMaxDeg: number,
    resizeFactor: number,
    normalize: boolean
  ): void;
  setNeedleEnhancementInsertionSide(rightSide: boolean): void;
  setParameterFilePath(path: string): void;
  activateLicense(activationKey: string, deviceId: string): boolean;
  processFrame(input: ArrayBuffer): ArrayBuffer;
  resetNeedleEnhancementTemporalState(): void;
  processNeedleEnhancementFrame(input: ArrayBuffer): ArrayBuffer;
}
