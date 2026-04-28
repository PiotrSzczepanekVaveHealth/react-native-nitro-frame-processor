import { NitroModules } from 'react-native-nitro-modules';
import type { NitroFrameProcessor } from './NitroFrameProcessor.nitro';

const NitroFrameProcessorHybridObject =
  NitroModules.createHybridObject<NitroFrameProcessor>('NitroFrameProcessor');

export function multiply(a: number, b: number): number {
  return NitroFrameProcessorHybridObject.multiply(a, b);
}
