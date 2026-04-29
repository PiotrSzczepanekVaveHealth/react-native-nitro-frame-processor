/**
 * @file us2d-7_tp.h
 *
 * @brief Rivent Tuning Parameters.
 *
 * Copyright (c) 2020 ContextVision AB.
 **/

#ifndef US2D7_TP_H
#define US2D7_TP_H
///@addtogroup tp_parameters Tuning Parameters
/// List of all available Tuning Parameters.
///@{

/**
 * This parameter controls the level of noise suppression by adding an offset to the level
 * defined in a given parameter file.
 * Negative values result in less noise suppression, i.e. more noise in the processed volume.
 * Positive values result in more noise suppression, i.e. less noise in the processed volume.
 * Valid value range for this option is [-10.0 to 10.0].
 *
 */
#define US2D7_TP_NOISE_SUPPRESSION 1001

/**
 * This parameter controls the level of sharpness in details, edges and borders by adding an
 * offset to the level defined in a given parameter file.
 * Negative values result in less sharpness.
 * Positive values result in less more sharpness.
 * Valid value range for this option is [-10.0 to 10.0].
 */
#define US2D7_TP_EDGE 1002

/**
 * This parameter controls the level of contrast between structures by adding an offset to the
 * level defined in a given parameter file.
 * Negative values result in less contrast.
 * Positive values result in more contrast.
 * Valid value range for this option is [-10.0 to 10.0].
 */
#define US2D7_TP_DETAIL 1003

/**
 * This parameter is used to check if the limit feature of US PlusView is enabled.
 * The limit feature is used to minimize under- and overshoots.
 * A value of 1.0 means enabled and a value of 0.0 means disabled.
 */
#define US2D7_TP_LIMIT_ENABLED 1004

/**
 * This parameter controls the amount of overshoot reduction.
 * Low values result in less reduction.
 * Valid value range for this option is [0.0 to 100.0].
 */
#define US2D7_TP_LIMIT_OVERSHOOTREDUCTION 1005

/**
 * This parameter controls the amount of undershoot reduction.
 * Low values result in less reduction.
 * Valid value range for this option is [0.0 to 100.0].
 */
#define US2D7_TP_LIMIT_UNDERSHOOTREDUCTION 1006

/**
 * This parameter is used to check if the strength feature of US PlusView is enabled.
 * The strength feature is used to control the overall strength of the USPlusView processing.
 * A value of 1.0 means enabled and a value of 0.0 means disabled.
 */
#define US2D7_TP_STRENGTH_ENABLED 1007

/**
 * This parameter controls the processing strength for fine structures.
 * High value gives stronger processing.
 * Valid value range for this option is [0.0 to 100.0].
 */
#define US2D7_TP_STRENGTH_FINESTRUCTURES 1008

/**
 * This parameter controls the processing strength for larger structures.
 * High value gives stronger processing.
 * Valid value range for this option is [0.0 to 100.0].
 */
#define US2D7_TP_STRENGTH_LARGESTRUCTURES 1009

///@}

#endif
