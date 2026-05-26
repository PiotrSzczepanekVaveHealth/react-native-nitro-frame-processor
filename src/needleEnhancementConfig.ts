export type NeedleEnhancementSliderConfig = {
  min: number;
  max: number;
  defaultValue: number;
  updateBy: number;
};

export type NeedleEnhancementConfig = {
  angle: NeedleEnhancementSliderConfig;
  length: NeedleEnhancementSliderConfig;
};

export const NEEDLE_ENHANCEMENT_CONFIG: NeedleEnhancementConfig = {
  angle: {
    min: 25,
    max: 35,
    defaultValue: 25,
    updateBy: 2,
  },
  length: {
    min: 0,
    max: 512,
    defaultValue: 100,
    updateBy: 1,
  },
};
