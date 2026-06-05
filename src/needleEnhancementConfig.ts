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
    min: 4,
    max: 35,
    defaultValue: 4,
    updateBy: 2,
  },
};
