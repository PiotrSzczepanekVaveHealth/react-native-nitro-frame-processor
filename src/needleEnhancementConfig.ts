export type NeedleEnhancementSliderConfig = {
  min: number;
  max: number;
  defaultValue: number;
  updateBy: number;
};

export type NeedleEnhancementConfig = {
  angle: NeedleEnhancementSliderConfig;
};

export const NEEDLE_ENHANCEMENT_CONFIG: NeedleEnhancementConfig = {
  angle: {
    min: 25,
    max: 35,
    defaultValue: 25,
    updateBy: 2,
  },
};
