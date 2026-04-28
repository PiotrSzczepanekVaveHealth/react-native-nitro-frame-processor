/**
 * @file us2d_tp.h
 *
 * @brief Version 1 Tuning Parameters for us2d generation 4 - 9.
 *
 * Copyright (c) 2024 ContextVision AB.
 **/

#ifndef US2D_TP_H
#define US2D_TP_H
///@addtogroup tp_parameters Tuning Parameters
/// List of all available Tuning Parameters in the version 1 tuning interface for us2d, generation 4 to 9.
///
///Set with cvie::Worker::setParameterValue, ::CVIESetParameterValue or ::CvieParameterSetting_SetOption.

/// Different parameters are available for different us2d generations as noted below:
///
/// * Generation 4 and 5 have three temporal filtering related parameters.
/// * Generation 8 adds four taper related parameters.
///@{

/**
 * This parameter controls the level of noise suppression by adding an offset to the level
 * defined in a given parameter file.
 * Negative values result in less noise suppression, i.e. more noise in the processed volume.
 * Positive values result in more noise suppression, i.e. less noise in the processed volume.
 * Valid value range for this option is [-10.0 to 10.0].
 *
 */
#define CVIE_US2D_V1_NOISE_SUPPRESSION 1001

/**
 * This parameter controls the level of sharpness in details, edges and borders by adding an
 * offset to the level defined in a given parameter file.
 * Negative values result in less sharpness.
 * Positive values result in less more sharpness.
 * Valid value range for this option is [-10.0 to 10.0].
 */
#define CVIE_US2D_V1_EDGE 1002

/**
 * This parameter controls the level of contrast between structures by adding an offset to the
 * level defined in a given parameter file.
 * Negative values result in less contrast.
 * Positive values result in more contrast.
 * Valid value range for this option is [-10.0 to 10.0].
 */
#define CVIE_US2D_V1_DETAIL 1003

/**
 * This parameter is used to turn on or off the limit feature.
 * The limit feature is used to minimize under- and overshoots.
 * A value of 1.0 means enabled and a value of 0.0 means disabled.
 */
#define CVIE_US2D_V1_LIMIT_ENABLED 1004

/**
 * This parameter controls the amount of overshoot reduction.
 * Low values result in less reduction.
 * Valid value range for this option is [0.0 to 100.0].
 */
#define CVIE_US2D_V1_LIMIT_OVERSHOOTREDUCTION 1005

/**
 * This parameter controls the amount of undershoot reduction.
 * Low values result in less reduction.
 * Valid value range for this option is [0.0 to 100.0].
 */
#define CVIE_US2D_V1_LIMIT_UNDERSHOOTREDUCTION 1006

/**
 * This parameter is used to turn on or off the strength feature.
 * The strength feature is used to control the overall strength of the processing.
 * A value of 1.0 means enabled and a value of 0.0 means disabled.
 */
#define CVIE_US2D_V1_STRENGTH_ENABLED 1007

/**
 * This parameter controls the processing strength for fine structures.
 * High value gives stronger processing.
 * Valid value range for this option is [0.0 to 100.0].
 */
#define CVIE_US2D_V1_STRENGTH_FINESTRUCTURES 1008

/**
 * This parameter controls the processing strength for larger structures.
 * High value gives stronger processing.
 * Valid value range for this option is [0.0 to 100.0].
 */
#define CVIE_US2D_V1_STRENGTH_LARGESTRUCTURES 1009

/**
 * This parameter is used to turn on or off the temporal filter.
 * A value of 1.0 means enable and a value of 0.0 means disable.
 *
 * \note This parameter is available on us2d4 and us2d5 only.
 */
#define CVIE_US2D_V1_TEMPORAL_ENABLED 1010

/**
 * This parameter controls the impact of previous frames for temporal smoothing.
 * High value gives stronger influence of previous frames.
 * Valid value range for this option is [-10.0 to 10.0]
 *
 * \note This parameter is available on us2d4 and us2d5 only.
 */
#define CVIE_US2D_V1_TEMPORAL_STRENGTH 1011

/**
 * This parameter controls the amount of temporal smoothing.
 * High value gives increased temporal smoothing.
 * Valid value range for this option is [-10.0 to 10.0].
 *
 * \note This parameter is available on us2d4 and us2d5 only.
 */
#define CVIE_US2D_V1_TEMPORAL_HOMOGENEITY 1012

/**
 * This parameter controls the taper processing strength in the X dimension
 * of the image, in the near field. High value gives stronger processing.
 * Negative values give a reverse effect.
 * Valid value range for this option is [-30.0 to 100.0].
 * \note If Taper is not enabled in the setting this parameter has no effect, and the current_range is 0 to 0.
 *
 * \note This parameter is available from us2d8 only.
 */
#define CVIE_US2D_V1_TAPER_STRENGTH_X_NEAR 1010

/**
 * This parameter controls the taper processing strength in the X dimension
 * of the image, in the far field. High value gives stronger processing.
 * Negative values give a reverse effect.
 * Valid value range for this option is [-30.0 to 100.0].
 * \note If Taper is not enabled in the setting this parameter has no effect, and the current_range is 0 to 0.
 *
 * \note This parameter is available from us2d8 only.
 */
#define CVIE_US2D_V1_TAPER_STRENGTH_X_FAR 1011

/**
 * This parameter controls the taper processing strength in the Y dimension
 * of the image, in the near field. High value gives stronger processing.
 * Negative values give a reverse effect.
 * Valid value range for this option is [-30.0 to 100.0].
 * \note If Taper is not enabled or Depth dependence is not enabled in the setting this parameter has no effect, and the current_range is 0 to 0.
 *
 * \note This parameter is available from us2d8 only.
 */
#define CVIE_US2D_V1_TAPER_STRENGTH_Y_NEAR 1012

/**
 * This parameter controls the taper processing strength in the Y dimension
 * of the image, in the far field. High value gives stronger processing.
 * Negative values give a reverse effect.
 * Valid value range for this option is [-30.0 to 100.0].
 * \note If Taper is not enabled or Depth dependence is not enabled in the setting this parameter has no effect, and the current_range is 0 to 0.
 *
 * \note This parameter is available from us2d8 only.
 */
#define CVIE_US2D_V1_TAPER_STRENGTH_Y_FAR 1013



/**
 * This parameter controls noise level in the image. 
 * Higher values increase the noise while lower values decrease the noise.   
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_NOISE_LEVEL 1001

/**
 * This parameter controls the level of structure enhancement in the NEAR field with focus on edges and borders.
 * If Depth Control is disabled in the original parameter file, this parameter will affect both the near field and the far field.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_EDGE_PRIMARY_NEAR 1002

/**
 * This parameter controls the level of structure enhancement in the FAR field with focus on edges and borders.
 * If Depth Control is disabled in the original parameter file, this parameter will not affect the processing.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_EDGE_PRIMARY_FAR 1003

/**
 * This parameter control structure enhancement at an earlier stage in the algorithm than Edge Primary.  
 * This parameter can bring about different structure enhancement effects compared with Edge Primary. 
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_EDGE_SECONDARY 1004

/**
 * This parameter controls the level of detail enhancement in the NEAR field.
 * If Depth Control is disabled in the original parameter file, this parameter will affect both the near field and the far field.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_DETAIL_PRIMARY_NEAR 1005

/**
 * This parameter controls the level of detail enhancement in the FAR field.
 * If Depth Control is disabled in the original parameter file, this parameter will not affect the processing.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_DETAIL_PRIMARY_FAR 1006

/**
 * This parameter controls detail enhancement at an earlier stage in the algorithm than Detail Primary.
 * This parameter can bring about different detail enhancement effects compared with Detail Primary.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_DETAIL_SECONDARY 1007

/**
 * This parameter is used to set and/or check if the four reduction parameters (1009 - 1012) are enabled or disabled in the original
 * parameter file. If this parameter is disabled, the next four tuning parameters will not affect the processing. Boolean parameter
 * encoded as a float. 0.0 is treated as false (disabled) and 1.0 as true (enabled).
 */
#define CVIE_US2D_V2_ENABLE_REDUCTION 1008

/**
 * This parameter is used to control the amount of undershoot reduction. 
 * Undershoots are also sometimes referred to as black borders or lipliners. 
 * Increasing this value will reduce undershoots in the resulting image.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_UNDERSHOOT_REDUCTION 1009

/**
 * This parameter controls the threshold of the above undershoot reduction.
 * Greyvalues in undershoot regions below the set value are affected by the undershoot reduction.
 * Valid value range for this parameter is [0.0 to 100.0]
 *
 * \note The current_max value for this parameter is the same as the set value of the CVIE_US2D_V2_OVERSHOOT_THRESHOLD. Any value
 * above this is clamped to prevent a reversed interval being used in the algorithm.
 */
#define CVIE_US2D_V2_UNDERSHOOT_THRESHOLD 1010

/**
 * This parameter controls the amount of overshoot reduction. 
 * Overshoots occur when the enhancement causes saturation of pixels.
 * Increasing this value will reduce the overshoots in the resulting image. 
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_OVERSHOOT_REDUCTION 1011

/**
 * This parameter controls the threshold of the above overshoot reduction.
 * Greyvalues in overshoot regions above this value are affected by the overshoot reduction. 
 * Note that the value of Undershoot Threshold cannot be higher than the value of 
 * Valid value range for this parameter is [0.0 to 100.0]
 *
 * \note The current_min value for this parameter is the same as the set value of the CVIE_US2D_V2_UNDERSHOOT_THRESHOLD. Any value
 * below this is clamped to prevent a reversed interval being used in the algorithm.
 */
#define CVIE_US2D_V2_OVERSHOOT_THRESHOLD 1012

/**
 * This parameter is used to set and/or check if the next two structure enhancement parameters (1013 & 1014) 
 * are enabled or disabled in the original parameter file.
 * If this parameter is disabled, the next two tuning parameters will not affect the processing.
 * Boolean parameter encoded as a float. 0.0 is treated as false (disabled) and 1.0 as true (enabled).
 */
#define CVIE_US2D_V2_ENABLE_STRUCTURE_ENHANCEMENT 1013

/**
 * This parameter controls the overall processing strength of FINER structures in the image.
 * High values result in a more processed output while low values result in a less processed output.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_FINE_STRUCTURE_ENHANCEMENT 1014

/**
 * This parameter controls the overall processing strength of LARGER structures in the image.
 * High values result in a more processed output while low values result in a less processed output.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_LARGE_STRUCTURE_ENHANCEMENT 1015

/**
 * This parameter controls the connectivity of edge-like structures in the NEAR field.
 * A higher value increases the connectivity.
 * If Depth Control is disabled in the original parameter file, this parameter will affect the near field and the far field.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_CONNECTIVITY_EDGE_NEAR 1016

/**
 * This parameter controls the connectivity of edge-like structures in the FAR field.
 * A higher value increases the connectivity.
 * If Depth Control is disabled in the original parameter file, this parameter will not affect the processing.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_CONNECTIVITY_EDGE_FAR 1017


/**
 * This parameter controls the connectivity of tissue-like structures in the NEAR field.
 * A higher value increases the connectivity.
 * If Depth Control is disabled in the original parameter file, this parameter will affect the near field and the far field.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_CONNECTIVITY_TISSUE_NEAR 1018

/**
 * This parameter controls the connectivity of tissue-like structures in the FAR field.
 * A higher value increases the connectivity.
 * If Depth Control is disabled in the original parameter file, this parameter will not affect the processing.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_CONNECTIVITY_TISSUE_FAR 1019


/**
 * This parameter is used to set a threshold value for the next TaperLine & TaperEdge parameters. 
 * The threshold can be set to focus the Taper effect on different types of structures.
 * A higher threshold value results in less Taper effect on uniform areas.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_THRESHOLD 1020

/**
 * The eight parameters below control the strength of Taper in the near- and far field and in X and Y direction.
 * If the skin surface is at the left of the processed image, then X is axial and Y is lateral.
 */

/**
 * This parameter controls the AXIAL strength of TaperLine in the NEAR field. 
 * A higher value makes lines thinner and increases the perceived spatial resolution.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_STRENGTH 1021                            ///< For us2d_7
#define CVIE_US2D_V2_TAPER_STRENGTH_X_NEAR 1021                     ///< For us2d_8
#define CVIE_US2D_V2_TAPER_STRENGTH_LINE_X_NEAR 1021                ///< For us2d_9

/**
 * This parameter controls the AXIAL strength of TaperLine in the FAR field.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_STRENGTH_X_FAR 1022                     ///< For us2d_8
#define CVIE_US2D_V2_TAPER_STRENGTH_LINE_X_FAR 1022                ///< For us2d_9

/**
 * This parameter controls the LATERAL strength of TaperLine in the NEAR field.
 * A higher value makes lines thinner and increases the perceived spatial resolution.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_STRENGTH_Y_NEAR 1023                     ///< For us2d_8
#define CVIE_US2D_V2_TAPER_STRENGTH_LINE_Y_NEAR 1023                ///< For us2d_9

/**
 * This parameter controls the LATERAL strength of TaperLine in the FAR field.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_STRENGTH_Y_FAR 1024                     ///< For us2d_8
#define CVIE_US2D_V2_TAPER_STRENGTH_LINE_Y_FAR 1024                ///< For us2d_9

/**
 * This parameter controls the AXIAL strength of TaperEdge in the NEAR field.
 * A higher value will make edges sharper.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_STRENGTH_EDGE_X_NEAR 1025              ///< For us2d_9

/**
 * This parameter contrls the AXIAL strength of TaperEdge in the FAR field.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_STRENGTH_EDGE_X_FAR 1026                ///< For us2d_9

/**
 * This parameter controls the LATERAL strength of TaperEdge in the NEAR field.
 * A higher value will make edges sharper.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_STRENGTH_EDGE_Y_NEAR 1027              ///< For us2d_9

/**
 * This parameter controls the LATERAL strength of TaperEdge in the FAR field.
 * Valid value range for this parameter is [0.0 to 100.0]
 */
#define CVIE_US2D_V2_TAPER_STRENGTH_EDGE_Y_FAR 1028                ///< For us2d_9


///@}

#endif
