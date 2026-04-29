//=============================================================================
///
/// @file cvie2.h
///
/// @brief Deprecated CVIE2 C API header
///
/// Copyright (c) 2015-2024 ContextVision AB.
///
//=============================================================================
#ifndef CVIE2_HEADER_INCLUDED
#define CVIE2_HEADER_INCLUDED
#pragma once

/**
 * \defgroup CVIE2 CVIE2 API Reference
 * \brief Version 2 image enhancement functionality.
 *
 * \warning This API is deprecated and its documentation is included only for existing customers using it. For new integrations prefer the
 * \ref CVIECPPOverview and if C++ is not available the \ref CVIEOverview including its ::CVIEWorker based functions.
 *
 * CONFIDENTIAL
 * \copyright Copyright (c) ContextVision AB.
 *
 * \par Basic calling sequence
 *
 *  - Initialize the library by calling CvieEnter()
 *  - Create an image enhancement instance by calling CvieInstance_Create()
 *  - Create input and output image descriptors by calling one of the Create functions for ::CvieImageDescriptor
 *  - Set a parameter file by calling ::CvieInstance_LoadParameterFile()
 *  - Get a parameter setting object by calling CvieInstance_GetParameterSetting()
 *  - Setup image dimension data by calling CvieParameterSetting_Setup()
 *  - Attach the image descriptors to the image data by calling one of the Attach functions for ::CvieImageDescriptor
 *  - Enhance images by calling ::CvieParameterSetting_Enhance()
 *  - Destroy the created objects to free resources by calling CvieInstance_Destroy() and CvieImageDescriptor_Destroy()
 *  - Finalize the library by calling CvieExit()
 *
 * \{
 **/

#include "cvlm.h"

/** \cond */


/** \endcond */

typedef int CvieResult;      /**< Result code for API functions. \ingroup Results */
typedef int CviePixelFormat; /**< Pixel format for input/output data. \ingroup PixelFormats */
typedef int CvieOptionType;  /**< Option type identifier. \ingroup Options */
typedef int CvieOptionId;    /**< Option identifier. \ingroup Options */

/**
 * \defgroup Instance Instance class
 * This is the Instance class.
 */
typedef struct TCvieInstance* CvieInstance; /**< Handle to a CVIE instance object. \ingroup Instance */

/**
 * \defgroup Setting ParameterSetting class
 * This is the ParameterSetting class.
 */
typedef struct TCvieParameterSetting* CvieParameterSetting; /**< Handle to a parameter setting object. \ingroup Setting */

/**
 * \defgroup ImageDescriptor ImageDescriptor class
 * This is the ImageDescriptor class.
 */
typedef struct TCvieImageDescriptor* CvieImageDescriptor; /**< Handle to an image descriptor object. \ingroup ImageDescriptor */

/** \defgroup Results Result codes from API calls
 * These values are returned by the API calls.
\{*/
#define CVIE_RESULT_OK 0 /**< The call succeeded. */

/** See documentation of function for details. */
#define CVIE_RESULT_NOT_OK 1

/** The handle is invalid. */
#define CVIE_RESULT_BAD_HANDLE 3

/** The operation was aborted by the user. */
#define CVIE_RESULT_USER_ABORTED 4

/** File IO error, for example file not found or not readable. */
#define CVIE_RESULT_FILEIO_ERROR 6

/** The call is made out of sequence and illegal in the current state. */
#define CVIE_RESULT_ILLEGAL_COMMAND 7

/** One of the actual parameters has an invalid value. */
#define CVIE_RESULT_INVALID_INPUT 8

/** The supplied parameter file or buffer is invalid. */
#define CVIE_RESULT_INVALID_PARAMETER_FILE 9

/** The requested action requires a license key that is not found. */
#define CVIE_RESULT_LICENSE_ERROR 10

/** The requested action, while recognized, is not supported in the current configuration. */
#define CVIE_RESULT_NOT_SUPPORTED 11

/** A memory allocation failed.
 * The library is in an invalid state and no further usage of the library API is possible.
 *
 * \note This return code is possible from any API function and is not specifically declared
 * as a possible return value for individual functions.
 */
#define CVIE_RESULT_OUT_OF_MEMORY 16

/** An unexpected fatal error occurred, for example out of memory or memory corruption.
 * The library is in an invalid state and no further usage of the library API is possible.
 *
 * \note This return code is possible from any API function and is not specifically declared
 * as a possible return value for individual functions.
 */
#define CVIE_RESULT_FATAL_ERROR 5

/** An unknown error has occurred. Continued use of the API is possible, but may lead to further errors.
 *
 * \note This return code is possible from any API function and is not specifically declared
 * as a possible return value for individual functions.
 */
#define CVIE_RESULT_UNKNOWN_ERROR 15


/** Macro that checks if a given return value is a success or not. */
#define CVIE_SUCCESS(x) ((x) == CVIE_RESULT_OK)

/**\}*/


/** \defgroup PixelFormats Pixel Formats
 * \{ */
#define CVIE_PIXEL_FORMAT_U8 1  /**< Unsigned 8-bit data */
#define CVIE_PIXEL_FORMAT_U16 2 /**< Unsigned 16-bit data */
#define CVIE_PIXEL_FORMAT_S16 3 /**< Signed 16-bit data */
#define CVIE_PIXEL_FORMAT_F32 4 /**< 32-bit IEEE floating point data */

/** \} */

#ifndef CVIE_HEADER_INCLUDED

#define CVIE_LUMINANCE_CHANNEL 10   /* Calculate luminance from RGB and process this. On output: Copy alpha from input. */
#define CVIE_INTENSITY_CHANNEL 11   /* Calculate intensity from RGB and process this. On output: Set alpha to opaque (max value). */

#endif

/** \defgroup Options Options
 * \{ */

/** \name Option Value Types
 \{
 */

#define CVIE_OPTION_TYPE_INT32 1 /**< Signed 32-bit integer */
#define CVIE_OPTION_TYPE_FLOAT 2 /**< 32-bit float */

/** Null-terminated C character string.

 \note When setting and getting options of this type, the \c valueBuffer pointer shall be the pointer to the first character in the
 string (i.e. a char*). \note When setting options of this type, the terminating null byte shall always be included in the \c
 valueBufferSize parameter. Likewise, when getting options of this type, the \c valueSize returned will also include the terminating
 null byte.

*/
#define CVIE_OPTION_TYPE_CSTR 3



#define CVIE_OPTION_TYPE_IMAGE_DESCRIPTOR 7 /**< \c image_descriptor */

/** Null-terminated C character string. The same rules as for char strings apply, except that each character is a wchar_t. */
#define CVIE_OPTION_TYPE_WCS 12

/** \} Option Value Types */


#ifndef CVIE_HEADER_INCLUDED

#define CVIE_ENABLE_TRACE 1             /* Constant used to enable encrypted trace log cvielog.dat. */
#define CVIE_ENABLE_CUSTOMER_TRACE 2    /* Constant used to enable customer plain text log customer_log.txt. */
#define CVIE_CLEAR_TRACE_FILES 4        /* Constant used clear log files that are enabled now. */

#endif

/** \defgroup GlobalOptions Global Options
 \{
 */

/**
 * This option sets the Device ID used for licensing. (read-write)
 */
#define CVIE_OPTION_CSTR_LICENSE_ID 2


/**
 * Set ::CVIE_ENABLE_TRACE (bit 0) of this option to to enable trace log output. This generates an encrypted log file which can be
 * used by ContextVision engineers to diagnose problems with the processing.
 *
 * Set ::CVIE_ENABLE_CUSTOMER_TRACE (bit 1) of this option to enable log output for customer use. This generates a human readable log
 * file with a subset of the ContextVision log contents.
 *
 * Set ::CVIE_CLEAR_TRACE_FILES (bit 2) of this option to clear the enabled log file(s) before logging, clearing happens when each
 * trace mode is enabled.
 *
 * The constants can be ored together. For instance setting the mode to CVIE_CLEAR_TRACE_FILES|CVIE_ENABLE_TRACE will start
 * encrypted log tracing after first clearing any pre-existing file.
 * (read-write)
 *
 * \note When read this option reports the effective trace mode, including trace mode set by environment variable COV_TRACE_MODE.
 *
 * \see \ref TraceLog
 */
#define CVIE_OPTION_INT32_TRACE_ENABLE 5



/**
 * Returns a string containing version information. (read-only)
 */
#define CVIE_OPTION_CSTR_VERSION_INFO 7

/**\} Global Options */

/** \defgroup InstanceOptions Instance Options
 *
 * For those options that represent a file path (directory or filename) the modifier CVIE_WIDE_PATH can be ored to the option
 * number to handle the path as a wide string in UTF-16 on Windows and UTF-32 on other operating systems.
 *
 *  \{ */

#ifndef CVIE_HEADER_INCLUDED
/// Value that can be ored with option constants for file path variables to indicate that the option value is a wchar_t* string.
#define CVIE_WIDE_PATH 16384
#endif

/**
 * This string may contain a description of the current parameter file,
 * if one has been added to it. (read-only)
 */
#define CVIE_INSTANCE_OPTION_CSTR_PARAMETER_FILE_DESCRIPTION 101

/**
 * Set this option to a non-zero value to use CPU for processing when GPU processing is available in the SDK. (read-write)
 *
 * \par Example
 * \code{.c}
 *      int enableCPU = 1;
 *      CvieInstance_SetOption(instance, CVIE_INSTANCE_OPTION_INT32_CPU_MODE, &enableCPU, sizeof(enableCPU));
 * \endcode
 */
#define CVIE_INSTANCE_OPTION_INT32_CPU_MODE 103








/** The string representation of the Loaded parameter file with modifications due to changes of tuning parameter values (read-only).
 *
 * This option can be read back after loading a parameter file and then performing CvieParameterSetting_SetOption on any of the
 * tuning parameters of any of the the settings in the file. It will return a string representation of the modified parameter file
 * including the set tuning parameter values so that when it is later loaded back the tuning parameters will regain the values set.
 */
#define CVIE_INSTANCE_OPTION_CSTR_PARAMETER_FILE 116

/** The string representation of the Loaded parameter file without modifications due to changes of tuning parameter values
 * (read-only).
 *
 * This option can be read back after loading a parameter file and then optionally performing CvieParameterSetting_SetOption on any
 * of the tuning parameters of any of the the settings in the file. It will return a string representation of the original parameter
 * file delivered by ContextVision regardless of if any of the tuning parameters have been changed (even if those changed values
 * were loaded from a file).
 */
#define CVIE_INSTANCE_OPTION_CSTR_ORIGINAL_PARAMETER_FILE 117


/** The string representation of the Loaded parameter file with modifications due to changes of tuning parameter values (read-only).
 *
 * This option can be read back after loading a parameter file and then performing CvieParameterSetting_SetOption on any of the
 * tuning parameters of any of the the settings in the file. It will return a string representation of the parameter file modified
 * by the tuning parameters as if it were an unmodified file, i.e. no tuning parameter values or original Settings are saved.
 * Further modification of tuning values after reloading the returned string into the CVIE library is not allowed.
 */

#define CVIE_INSTANCE_OPTION_CSTR_FLATTENED_PARAMETER_FILE 118


/** The string representation of the trace file path used for the trace log.
 *
 * To take effect the option must be set before any other \ref CVIE2 API-function calls, including the call to CvieEnter().
 | See \ref TraceLog for further details on the trace log.
 */
#define CVIE_OPTION_CSTR_TRACE_PATH 121
 /// Wide version of the ::CVIE_OPTION_CSTR_TRACE_PATH option.
#define CVIE_OPTION_WCS_TRACE_PATH (CVIE_OPTION_CSTR_TRACE_PATH | CVIE_WIDE_PATH)


/**
* The number of CPU threads used for processing (read-write). The default is to use one thread per physical CPU core.
* This option is only valid for CPU processing.
*
* \par Example
* \code{.c}
*      int numThreads = 4;
*      CvieInstance_SetOption(instance, CVIE_INSTANCE_OPTION_INT32_NUM_THREADS, &numThreads, sizeof(numThreads));
* \endcode
*/
#define CVIE_INSTANCE_OPTION_INT32_NUM_THREADS 130

/** \} Instance Options */

/**
 * \defgroup RSSOptions Options for improved integration to the Context Vision Remote Service System.
 * Extra option IDs which can be used when calling CvieInstance_GetOptionInfo() and CvieInstance_GetOption() which should only have effect
 * when the cvie_remote_service library is present.
 *
 * Both the CVIE_INSTANCE_OPTION_CSTR_RSS_IDENTIFICATION_ANATOMY_LEVEL and
 * CVIE_INSTANCE_OPTION_CSTR_RSS_IDENTIFICATION_FILENAME_SETTING options return the empty string except when a Remote Service
 * Tool or ContextVision Internal Tuning Tool (referred to as just Tool below) has taken over control over which setting the CVIE
 * library is using for processing. When control has been taken over these read-only options will yield string values depending on
 * the selected Anatomy and Level in the controlling Tool user interface. The only difference between the options is how the
 * selection is represented. Applications should just use one, depending on which representation is most convenient.
 *
 * Applications don't have to use these options but it is advantageous to check one of them periodically and adjust the settings on
 * the machine as if the user had selected the same Anatomy and Level in the machine's user interface as in the controlling Tool.
 * This reduces the risk of combining an Anatomy and Level selected on the machine, controlling the post-processing occurring after
 * the CVIE image enhancement with _another_ Anatomy and Level selected in the controlling Tool. If such erroneous combinations are
 * in effect when reviewing the settings this can impair the quality of the evaluation. This is especially important if the Tool and
 * physical machine are placed far apart so that the tool operator can't see what is actually selected on the machine, and only
 * observes the screen shots sent from the machine to the Tool.
 *
 * \note The application code to implement this can be retained in the production application. The string buffer will always be set to an
 * empty string when the cvie_remote_service library is not present or if it is disabled. The CvieInstance_GetOption function call
 * which just sets the string empty is very fast.
 *
 * \{
 */


/** A string representation of the currently selected Anatomy and Level in a controlling Tool (read-only).
 *
 * This option will always return the empty string except when a controlling Tool has
 * taken over control over which setting the CVIE library is using for processing. When control has been taken over this
 * option will contain a string value representing selected Anatomy and Level in the Remote Service Tool user interface.
 *
 * The value is the Anatomy name followed by a colon and the Level name. No spaces are inserted before or after the colon.
 *
 *
 */

#define CVIE_INSTANCE_OPTION_CSTR_RSS_IDENTIFICATION_ANATOMY_LEVEL 2000

/** A string representation of the currently selected parameter filename and setting index in a controlling Tool (read-only).
 *
 * This option will always return the empty string except when a controlling Tool has
 * taken over control over which setting the CVIE library is using for processing. When control has been taken over this
 * option will contain a string value representing the parameter file filename and setting index corresponding to the selected
 * Anatomy and Level in the Remote Service Tool user interface.
 *
 * The value is the filename (without directories) followed by a colon and the setting index that is mapped to the selected
 * Anatomy and Level in the Tool user interface. No spaces are inserted before or after the colon.
 *
 */

#define CVIE_INSTANCE_OPTION_CSTR_RSS_IDENTIFICATION_FILENAME_SETTING 2001


/** \} RSSOptions */

/** \defgroup SettingOptions Setting Options
 *  \{ */

/**
 * Returns a description for the setting from the parameter file. (read-only)
 */

#define CVIE_SETTING_OPTION_CSTR_DESCRIPTION 201
/**
 * This is a user-settable description of the setting. (read/write)
 */
#define CVIE_SETTING_OPTION_CSTR_CUSTOMER_DESCRIPTION 220

 /** This option is the string corresponding to the extension of a parameter file containing this type of settings. (read-only).
  */
#define CVIE_SETTING_OPTION_CSTR_SETTING_TYPE 206


/**
 * Controls the relative resolution in the X direction for USPlusView. (read-write)
 *
 * The purpose of this option is to adapt the image processing to the resolution of the
 * input image in case it is different from the resolution for which the current parameter setting was tuned.
 *
 * \note resolution in this case means pixel density, ie the number of pixels per inch or millimeter.
 * The relative resolution is a unitless factor. A value of 1 means that the current resolution is the same
 * as the parameter setting was tuned for.
 * A value of 1.5 means that the current resolution is 50% higher than what the current parameter setting was tuned for.
 * If the relative resolution is changed the new values will be used for the next CvieParameterSetting_Enhance() call.
 *
 * \par Example
 * \code{.c}
 *      float relativeResolutionX = 1.5f;
 *      CvieParameterSetting_SetOption(setting, CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_X, &relativeResolutionX,
 * sizeof(relativeResolutionX)); \endcode
 */
#define CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_X 208

/**
 * Controls the relative resolution in the Y direction for USPlusView. (read-write)
 *
 * The purpose of this option is to adapt the image processing to the resolution of the
 * input image in case it is different from the resolution for which the current parameter setting was tuned.
 *
 * \note resolution in this case means pixel density, ie the number of pixels per inch or millimeter.
 * The relative resolution is a unitless factor. A value of 1 means that the current resolution is the same
 * as the parameter setting was tuned for.
 * A value of 1.5 means that the current resolution is 50% higher than what the current parameter setting was tuned for.
 * If the relative resolution is changed the new values will be used for the next CvieParameterSetting_Enhance() call.
 *
 * \par Example
 * \code{.c}
 *      float relativeResolutionY = 1.5f;
 *      CvieParameterSetting_SetOption(setting, CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_Y, &relativeResolutionY,
 * sizeof(relativeResolutionY)); \endcode
 */
#define CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_Y 209


/**
 * Controls the location of the transition start of the "near field" and "far field" processing zones (read/write).
 * Near field processing is applied in a zone from the minimum depth (usually zero) to
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START. Far field processing is applied in a zone from
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END to the maximum depth. Between
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START and CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END,
 * processing will gradually go from near field processing to far field processing. It is recommended that the transition zone is
 * not too narrow. If CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START = 0.0 and
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END = 1.0, the transition zone will contain the entire image. To have a
 * difference in processing between near and far field, this feature needs to be enabled in the specific parameter setting used.
 * The magnitude and characteristics of this difference can only be modified internally, i.e., by creating a new parameter setting.
 * Minimum value is 0.0 and maximum value is 1.0. CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START must be less than,
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END. If CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START is
 * changed, the new values will be used for the next CvieParameterSetting_Enhance() call.
 *
 * \par Example
 * \code{.c}
 *      float depthDependenceTransitionStart = 0.25f;
 *      CvieParameterSetting_SetOption(setting, CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START,
 * &depthDependenceTransitionStart, sizeof(depthDependenceTransitionStart)); \endcode
 */
#define CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START 214

/**
 * Controls the location of the transition end of the "near field" and "far field" processing zones (read/write).
 * Near field processing is applied in a zone from the minimum depth (usually zero) to
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START. Far field processing is applied in a zone from
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END to the maximum depth. Between
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START and CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END,
 * processing will gradually go from near field processing to far field processing. It is recommended that the transition zone is
 * not too narrow. If CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START = 0.0 and
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END = 1.0, the transition zone will contain the entire image. To have a
 * difference in processing between near and far field, this feature needs to be enabled in the specific parameter setting used.
 * The magnitude and characteristics of this difference can only be modified internally, i.e., by creating a new parameter setting.
 * Minimum value is 0.0 and maximum value is 1.0. CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START must be less than,
 * CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END. If CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END is
 * changed, the new values will be used for the next CvieParameterSetting_Enhance() call.
 *
 * \par Example
 * \code{.c}
 *      float depthDependenceTransitionEnd = 0.25f;
 *      CvieParameterSetting_SetOption(setting, CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END,
 * &depthDependenceTransitionEnd, sizeof(depthDependenceTransitionEnd)); \endcode
 */
#define CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END 215

/**
 * Sets the number of CPU threads used for processing. The default is to use one thread per physical CPU core. (read-write)
 * This option is only valid for CPU processing.
 * 
 * \deprecated This setting is deprecated since the thread count now applies to the whole instance rather than the setting.
 * Use CVIE_INSTANCE_OPTION_INT32_NUM_THREADS instead. If this option is set it affects all settings on the same instance.
 *
 * \note When changing this setting, it will only take effect after ::CvieParameterSetting_Setup is
 * called again.
 *
 * \par Example
 * \code{.c}
 *      int numThreads = 4;
 *      CvieParameterSetting_SetOption(setting, CVIE_SETTING_OPTION_INT32_NUM_THREADS, &numThreads, sizeof(numThreads));
 * \endcode
 */
#define CVIE_SETTING_OPTION_INT32_NUM_THREADS 210

/**
 * Limits the amount of GPU memory used by 3D algorithms. (read-write)
 *
 * \note The value is in MiB (i.e. multiply by 1024*1024 to get the number of bytes).
 *
 * \par Example
 * \code{.c}
 *      int gpuMemoryLimitInMB = 1000;
 *      CvieParameterSetting_SetOption(setting, CVIE_SETTING_OPTION_INT32_MEMORY_LIMIT_IN_MB, &gpuMemoryLimitInMB,
 * sizeof(gpuMemoryLimitInMB)); \endcode
 */
#define CVIE_SETTING_OPTION_INT32_MEMORY_LIMIT_IN_MB 211

/**
 * Returns the amount of CPU or GPU memory currently used. For GPU this is per Instance, while for CPU this is a global value. (read-only)
 *
 * \note The value is in MiB (i.e. multiply by 1024*1024 to get the number of bytes).
 * \note This is a per setting option for historical reasons.
 *
 * \par Example
 * \code{.c}
 *      int memoryUsage;
 *      CvieParameterSetting_GetOption(setting, CVIE_SETTING_OPTION_INT32_MEMORY_USAGE_IN_MB, &memoryUsage, sizeof(memoryUsage), 0);
 * \endcode
 */
#define CVIE_SETTING_OPTION_INT32_MEMORY_USAGE_IN_MB 212

/**
 * Sets a binary mask used for processing. (read/write)
 * To disable mask processing, pass a CvieImageDescriptor handle which is null.
 *
 * The mask defines which parts of the image shall be processed.
 *  - Where the mask contains a zero, the pixels will be copied directly from the input image without processing.
 *  - Where the mask is non-zero, the pixels will be processed normally.
 *
 * The value shall be a ::CvieImageDescriptor handle, referring to an image descriptor defining the mask.
 *  - The descriptor must be the same size as the input image
 *  - The descriptor must have the format ::CVIE_PIXEL_FORMAT_U8
 *
 * \note This option can be set at any time during processing, but if called after CvieParameterSetting_Setup(), the next call to
 *       CvieParameterSetting_Enhance() will implicitly call CvieParameterSetting_Setup() again, leading to a small delay in
 *       processing.
 *
 * \warning If a temporal filter is in effect, this will also reset the temporal filter state (see note for
CvieParameterSetting_Setup())
 *
 * \see CvieImageDescriptor_CreateForBuffer2D()
 * \see CvieImageDescriptor_AttachBuffer2D()
 *
 * \par Example
 * \code{.c}
 *      CvieImageDescriptor maskDesc;
 *      char* mask = (char*)malloc(width*height);
 *      CvieImageDescriptor_CreateForBuffer2D(&maskDesc, width, height, CVIE_PIXEL_FORMAT_U8);
 *      CvieImageDescriptor_AttachBuffer2D(maskDesc, mask, width);
 *
 *      // Enable the mask
 *      CvieParameterSetting_SetOption(setting, CVIE_SETTING_OPTION_IMAGE_DESCRIPTOR_MASK, &maskDesc, sizeof(maskDesc));
 *
 *      // Disable the mask again
 *      CvieImageDescriptor nullDesc = nullptr;
 *      CvieParameterSetting_SetOption(setting, CVIE_SETTING_OPTION_IMAGE_DESCRIPTOR_MASK, &nullDesc, sizeof(nullDesc));
 * \endcode

 */
#define CVIE_SETTING_OPTION_IMAGE_DESCRIPTOR_MASK 213





/** \} Setting Options */

/** \} Options */

/**
This is the version of the CVIE2 API that is defined in this file.
It should not be changed, as it is used to verify that the version
of the header matches the version of the dynamic library.

*/
#define CVIE_API_VERSION 2007

/**
 * Initialize library.
 *
 * This function is not thread safe and should be called exactly once before starting to use the library.
 *
 * \pre No other CVIE function has been called
 * \post The CVIE library is ready to use
 * \param [in] apiVersion the requested API version.  Should always be ::CVIE_API_VERSION.
 *
 */
CVIE_API CvieResult CVIE_DECL CvieEnter(int apiVersion);
typedef CvieResult(CVIE_DECL* TCvieEnter)(int apiVersion); /**< Function pointer type for CvieEnter() */

/**
 * Deinitialize library.
 *
 * This function can be called to free all resources used by the library.
 * After this function is called, all existing handles will become invalid.
 *
 * \post Any CVIE handles are invalid (regardless of the returned error code)
 * \post All memory allocated by the CVIE is freed (regardless of the returned error code)
 *
 */
CVIE_API CvieResult CVIE_DECL CvieExit();
typedef CvieResult(CVIE_DECL* TCvieExit)(); /**< Function pointer type for CvieExit() */

/**
 * Gets the last encountered error for the current thread.
 *
 * \note The returned pointer is owned by the library and may be invalidated by other library calls.
 *
 * \returns A pointer to the error message
 */
CVIE_API const char* CVIE_DECL CvieGetLastError();
typedef const char*(CVIE_DECL* TCvieGetLastError)(); /**< Function pointer type for CvieGetLastError() */

/**
 * Gets a global option.
 *
 * \pre CvieEnter() has been called
 * \pre \c option is a valid global option
 * \pre \c valueBuffer points to a memory area of size \c valueBufferSize
 * \pre \c valueBufferSize is larger than or equal to the value size (as reported by CvieGetOptionInfo())
 * \post valueBuffer is filled with the option's current value
 * \post if \c valueSize is not \c NULL, \c *valueSize is equal to the actual size of the value, which may be less than \c valueBufferSize
 *
 * \param [in] option the option to get (see \ref GlobalOptions)
 * \param [out] valueBuffer the buffer where the option's value is stored
 * \param [in] valueBufferSize the size (in bytes) of \c valueBuffer
 * \param [out] valueSize the size (in bytes) of the returned value
 *
 * \note If \c valueBuffer points to a value that is not of the same type as the option, the results are undefined.
 * \see GlobalOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieGetOption(CvieOptionId option, void* valueBuffer, int valueBufferSize, int* valueSize);
typedef CvieResult(CVIE_DECL* TCvieGetOption)(CvieOptionId option, void* valueBuffer, int valueBufferSize,
        int* valueSize); /**< Function pointer type for CvieGetOption() */

/**
 * Gets information for a global option.
 *
 * \pre CvieEnter() has been called
 * \pre \c option is a valid global option
 * \post if \c valueType is not \c NULL, \c *valueType will be set to the type id of the option's value
 * \post if \c valueSize is not \c NULL, \c *valueSize will be set to the size of the option's current value
 *
 * \param [in] option the option to get information for (see \ref GlobalOptions)
 * \param [out] valueType the data type of the option value
 * \param [out] valueSize the size (in bytes) of the option value
 *
 * \see GlobalOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieGetOptionInfo(CvieOptionId option, CvieOptionType* valueType, int* valueSize);
typedef CvieResult(CVIE_DECL* TCvieGetOptionInfo)(
        CvieOptionId option, CvieOptionType* valueType, int* valueSize); /**< Function pointer type for CvieGetOptionInfo() */

/**
 * Sets a global option for the whole library. This function must be called before any instances are created.
 *
 * \pre CvieEnter() has been called
 * \pre CvieInstance_Create() has not been called
 * \pre \c option is a valid global option
 * \pre \c valueBuffer points to a valid value of the correct type for the selected option
 * \pre \c valueBufferSize corresponds to the size of the \c valueBuffer and the expected size of the value type
 * \post The option is effective
 *
 * \note If \c valueBuffer points to a value that is not of the same type as the option, the results are undefined.
 *
 * \param [in] option the option to set (see \ref GlobalOptions)
 * \param [in] valueBuffer pointer to the value to set
 * \param [in] valueBufferSize the size (in bytes) of the value to set
 *
 * \see GlobalOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieSetOption(CvieOptionId option, const void* valueBuffer, int valueBufferSize);
typedef CvieResult(CVIE_DECL* TCvieSetOption)(
        CvieOptionId option, const void* valueBuffer, int valueBufferSize); /**< Function pointer type for CvieSetOption() */

/** \addtogroup Instance
 * \{ */

/**
 * Creates a processing instance.
 *
 * \pre CvieEnter() has been called
 * \pre instance is a pointer to a CvieInstance handle
 * \post *instance is a valid CvieInstance handle
 *
 * \param [out] instance the created instance
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_Create(CvieInstance* instance);
typedef CvieResult(CVIE_DECL* TCvieInstance_Create)(CvieInstance* instance); /**< Function pointer type for CvieInstance_Create() */

/**
 * Destroys a processing instance.
 *
 * \pre instance is a pointer to a valid CvieInstance handle
 * \post *instance is deinitialized and its memory freed
 * \post *instance is a NULL pointer
 * \post any CvieParameterSetting handles belonging to the instance are invalid
 *
 * \param [in,out] instance the instance to destroy
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_Destroy(CvieInstance* instance);
typedef CvieResult(CVIE_DECL* TCvieInstance_Destroy)(
        CvieInstance* instance); /**< Function pointer type for CvieInstance_Destroy() */

/**
 * Loads a parameter file into an instance.
 *
 * \pre instance is a valid CvieInstance handle
 * \pre fileName is a path to a valid parameter file
 * \post the parameter file is loaded into the instance
 *
 * The CvieInstance_LoadParameterFileW function works the same but takes the parameter filename as a wide string.
 * \see WideFilenames
 *
 * Parameter files loaded from disk files are cached in memory. When CvieInstance_LoadParameterFile is called again with the same
 * filename the cached contents is used unless the time stamp has changed. On file systems where time stamps are coarse it may be
 * possible to write code that produces two distinct file contents with the same time stamp, thus fooling the cache.
 *
 * \param [in] instance the instance to use
 * \param [in] fileName the path to the parameter file to load
 *                      The string shall use the character encoding defined by the current system code page.
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_LoadParameterFile(CvieInstance instance, const char* fileName);
typedef CvieResult(CVIE_DECL* TCvieInstance_LoadParameterFile)(
        CvieInstance instance, const char* fileName); /**< Function pointer type for CvieInstance_LoadParameterFile() */

/// Version of ::CvieInstance_LoadParameterFile which takes a wide string as the filename parameter
CVIE_API CvieResult CVIE_DECL CvieInstance_LoadParameterFileW(CvieInstance instance, const wchar_t* fileName);
typedef CvieResult(CVIE_DECL* TCvieInstance_LoadParameterFileW)(
    CvieInstance instance, const wchar_t* fileName); /**< Function pointer type for CvieInstance_LoadParameterFileW() */

/**
 * Loads a parameter buffer into an instance.
 *
 * \pre instance is a valid CvieInstance handle
 * \pre data is a char pointer to a valid parameter buffer
 * \pre dataSize is the size in bytes of the buffer pointed to by \c data
 * \post the parameter data is loaded into the instance
 *
 * \param [in] instance the instance to use
 * \param [in] data the parameter data to load
 * \param [in] dataSize the size in bytes of the data to load
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_LoadParameterBuffer(CvieInstance instance, const char* data, int dataSize);
typedef CvieResult(CVIE_DECL* TCvieInstance_LoadParameterBuffer)(
        CvieInstance instance, const char* data, int dataSize); /**< Function pointer type for CvieInstance_LoadParameterBuffer() */


/**
 * Saves a parameter file with all set tuning parameter values to a named file. When this file is subsequently loaded
 * using CvieInstance_LoadParameterFile the tuning parameters get their values from the file. The computer where the
 * file is loaded does not have to be licensed for Doctor's Interface to be able to load the file.
 *
 * The CvieInstance_SaveParameterFileW function works the same but takes the parameter filename as a wide string.
 * \see WideFilenames
 *
 * \pre instance is a valid CvieInstance handle
 * \pre fileName is a path in an existing directory
 * \post the parameter data is saved in the file
 *
 * \param [in] instance the instance to use
 * \param [in] fileName the path to the parameter file to save
 *                      The string shall use the character encoding defined by the current system code page.
 *
 * \note This function is not useful if no tuning parameters have been changed as the saved file would just be a copy of
 * the currently loaded file.
 *
 * \note Using this function is equivalent to retrieving the currently loaded file using the
 * ::CVIE_INSTANCE_OPTION_CSTR_PARAMETER_FILE parameter to ::CvieInstance_GetOption function and then saving the result to a file.
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_SaveParameterFile(CvieInstance instance, const char* fileName);
typedef CvieResult(CVIE_DECL* TCvieInstance_SaveParameterFile)(
        CvieInstance instance, const char* fileName); /**< Function pointer type for CvieInstance_SaveParameterFile() */

/// Version of ::CvieInstance_SaveParameterFile which takes a wide string as the filename parameter
CVIE_API CvieResult CVIE_DECL CvieInstance_SaveParameterFileW(CvieInstance instance, const wchar_t* fileName);
typedef CvieResult(CVIE_DECL* TCvieInstance_SaveParameterFileW)(
    CvieInstance instance, const wchar_t* fileName); /**< Function pointer type for CvieInstance_SaveParameterFile() */


/**
 * Gets the number of parameter settings in an instance.
 *
 * \pre instance is a valid CvieInstance handle
 * \pre CvieInstance_LoadParameterFile() or CvieInstance_LoadParameterBuffer() has been called to load a parameter
 * \pre nSettings is a pointer to an int variable
 * \post the number of parameter settings available in the instance is stored in nSettings
 *
 * \param [in] instance the instance to use
 * \param [out] nSettings the number of parameter settings in the instance
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_GetNumberOfSettings(CvieInstance instance, int* nSettings);
typedef CvieResult(CVIE_DECL* TCvieInstance_GetNumberOfSettings)(
        CvieInstance instance, int* nSettings); /**< Function pointer type for CvieInstance_GetNumberOfSettings() */

/**
 * Gets a ParameterSetting handle from an instance.
 *
 * \pre instance is a valid CvieInstance handle
 * \pre 0 <= settingIndex < nSettings (as returned by CvieInstance_GetNumberOfSettings())
 * \pre parameterSetting is a pointer to a CvieParameterSetting handle
 * \post parameterSetting contains the handle to the requested parameter setting index
 *
 * \param [in] instance the instance to use
 * \param [in] settingIndex the index (zero-based) of the parameter setting to get
 * \param [out] parameterSetting the requested parameter setting handle
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_GetParameterSetting(
        CvieInstance instance, int settingIndex, CvieParameterSetting* parameterSetting);
typedef CvieResult(CVIE_DECL* TCvieInstance_GetParameterSetting)(CvieInstance instance, int settingIndex,
        CvieParameterSetting* parameterSetting); /**< Function pointer type for CvieInstance_GetParameterSetting() */

/**
 * Gets an instance option.
 *
 * \pre \c instance is a valid ::CvieInstance handle
 * \pre \c option is a valid instance option
 * \pre \c valueBuffer points to a memory area of size \c valueBufferSize
 * \pre \c valueBufferSize is larger than or equal to the value size (as reported by CvieInstance_GetOptionInfo())
 * \post valueBuffer is filled with the option's current value
 * \post if \c valueSize is non-NULL, \c *valueSize is set to the actual size of the value, which may be less than valueBufferSize
 *
 * \param [in] instance the instance to get an option for
 * \param [in] option the option to get (see \ref InstanceOptions)
 * \param [out] valueBuffer the buffer where the option's value is stored
 * \param [in] valueBufferSize the size (in bytes) of valueBuffer
 * \param [out] valueSize the size (in bytes) of the returned value
 *
 * \note If valueBuffer points to a value that is not of the same type as the option, the results are undefined.
 *
 * \see InstanceOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_GetOption(
        CvieInstance instance, CvieOptionId option, void* valueBuffer, int valueBufferSize, int* valueSize);
typedef CvieResult(CVIE_DECL* TCvieInstance_GetOption)(CvieInstance instance, CvieOptionId option, void* valueBuffer,
        int valueBufferSize, int* valueSize); /**< Function pointer type for CvieInstance_GetOption() */

/**
 * Gets information for an instance option.
 *
 * \pre instance is a valid ::CvieInstance handle
 * \pre option is a valid instance option
 * \post if valueType is not null, *valueType will be set to the type id of the option's value
 * \post if valueSize is not null, *valueSize will be set to the size of the option's current value
 *
 * \param [in] instance the instance to get an option for
 * \param [in] option the option to get information for (see \ref InstanceOptions)
 * \param [out] valueType the data type of the option value
 * \param [out] valueSize the size (in bytes) of the option value
 *
 * \see InstanceOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_GetOptionInfo(
        CvieInstance instance, CvieOptionId option, CvieOptionType* valueType, int* valueSize);
typedef CvieResult(CVIE_DECL* TCvieInstance_GetOptionInfo)(CvieInstance instance, CvieOptionId option, CvieOptionType* valueType,
        int* valueSize); /**< Function pointer type for CvieInstance_GetOptionInfo() */

/**
 * Sets a processing option for a specific instance. This function must be called before a parameter file is loaded,
 * and will be effective for all parameter settings in this instance.
 *
 * \pre instance is a valid ::CvieInstance handle
 * \pre option is a valid instance option
 * \pre valueBuffer points to a valid value of the correct type for the selected option
 * \pre valueBufferSize corresponds to the size of the valueBuffer and the expected size of the value type
 * \pre CvieInstance_LoadParameterFile() or CvieInstance_LoadParameterBuffer() has not been called for this instance
 * \post The option is effective
 *
 * \param [in] instance the instance to set an option for
 * \param [in] option the option to set (see \ref InstanceOptions)
 * \param [in] valueBuffer pointer to the value to set
 * \param [in] valueBufferSize the size (in bytes) of the value to set
 *
 * \note If valueBuffer points to a value that is not of the same type as the option, the results are undefined.
 *
 * \see InstanceOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_SetOption(
        CvieInstance instance, CvieOptionId option, const void* valueBuffer, int valueBufferSize);
typedef CvieResult(CVIE_DECL* TCvieInstance_SetOption)(CvieInstance instance, CvieOptionId option, const void* valueBuffer,
        int valueBufferSize); /**< Function pointer type for CvieInstance_SetOption() */

/**
 * Forces the release of image memory allocated by an instance.
 * Use of this function may affect the performance of the next enhancement call.
 * It is advised to not call this function between enhancement calls unless you really need to release GPU memory for your other use.
 * If this function is called with NULL as argument it will release all GPU memory used for programs not currently in use.
 *
 *
 * \param [in] instance A valid CvieInstance to release memory for or NULL.
 *
 * \note For CPU instances image memory for all instances is released as CPU memory is shared between CPU instances.
 */
CVIE_API CvieResult CVIE_DECL CvieInstance_ReleaseMemory(CvieInstance instance);
typedef CvieResult(CVIE_DECL* TCvieInstance_ReleaseMemory)(
        CvieInstance instance); /**< Function pointer type for CvieInstance_ReleaseMemory() */

/** \} */

/** \addtogroup Setting
 * \{ */

/**
 * Sets the parameter setting up for enhancement by specifying an input and output descriptor to use.
 * The descriptors need not point to any data at the time of this call.
 * This function must be called before calling CvieParameterSetting_Enhance() for the first time,
 * or whenever the image geometry is changed, to associate the new CvieImageDescriptor object(s) with the parameter setting.
 *
 * \pre \c setting is a valid ::CvieParameterSetting handle
 * \pre \c input is a valid ::CvieImageDescriptor handle
 * \pre \c output is a valid ::CvieImageDescriptor handle
 * \post The setting is ready for enhancement using CvieParameterSetting_Enhance()
 *
 * \param [in] setting the parameter setting to set up
 * \param [in] input an image descriptor describing the input data layout
 * \param [in] output an image descriptor describing the output data layout
 *
 * \note When calling this function the temporal state is reset.
 *       This means that image sequence processing algorithms gets the
 *       indication that the next frame to process is the first frame in the
 *       sequence.  In other words, this function should not be called between
 *       consecutive frames of an image sequence when a temporal filter is in use.
 *       For more information, see \ref TemporalStateReset.
 *
 */
CVIE_API CvieResult CVIE_DECL CvieParameterSetting_Setup(
        CvieParameterSetting setting, CvieImageDescriptor input, CvieImageDescriptor output);
typedef CvieResult(CVIE_DECL* TCvieParameterSetting_Setup)(CvieParameterSetting setting, CvieImageDescriptor input,
        CvieImageDescriptor output); /**< Function pointer type for CvieParameterSetting_Setup() */

/**
 * Runs the image enhancement operation.
 *
 * \pre \c setting is a valid ::CvieParameterSetting handle
 * \pre CvieParameterSetting_Setup() has been called on \c setting
 * \pre The image descriptors used in the CvieParameterSetting_Setup() call have been attached to valid image data
 * \post The data attached to the output image descriptor contains the enhanced image data.
 *
 * \param [in] setting the parameter setting to set up
 *
 */
CVIE_API CvieResult CVIE_DECL CvieParameterSetting_Enhance(CvieParameterSetting setting);
typedef CvieResult(CVIE_DECL* TCvieParameterSetting_Enhance)(
        CvieParameterSetting setting); /**< Function pointer type for CvieParameterSetting_Enhance() */

/**
 * Gets a parameter setting option.
 *
 * \pre \c setting is a valid ::CvieParameterSetting handle
 * \pre \c option is a valid parameter setting option
 * \pre \c valueBuffer points to a memory area of size \c valueBufferSize
 * \pre \c valueBufferSize is larger than or equal to the value size (as reported by CvieParameterSetting_GetOptionInfo())
 * \post valueBuffer is filled with the option's current value
 * \post if \c valueSize is non-NULL, \c *valueSize is set to the actual size of the value, which may be less than valueBufferSize
 *
 * \param [in] setting the parameter setting to get an option for
 * \param [in] option the option to get (see \ref SettingOptions)
 * \param [out] valueBuffer the buffer where the option's value is stored
 * \param [in] valueBufferSize the size (in bytes) of valueBuffer
 * \param [out] valueSize the size (in bytes) of the returned value
 *
 * \note If valueBuffer points to a value that is not of the same type as the option, the results are undefined.
 *
 * \see SettingOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieParameterSetting_GetOption(
        CvieParameterSetting setting, CvieOptionId option, void* valueBuffer, int valueBufferSize, int* valueSize);
typedef CvieResult(CVIE_DECL* TCvieParameterSetting_GetOption)(CvieParameterSetting setting, CvieOptionId option,
        void* valueBuffer, int valueBufferSize, int* valueSize); /**< Function pointer type for CvieParameterSetting_GetOption() */

/**
 * Gets information for a parameter setting option.
 *
 * \pre \c setting is a valid ::CvieParameterSetting handle
 * \pre \c option is a valid parameter setting option
 * \post if valueType is not null, *valueType will be set to the type id of the option's value
 * \post if valueSize is not null, *valueSize will be set to the size of the option's current value
 *
 * \param [in] setting the parameter setting to get information for
 * \param [in] option the option to get information for (see \ref SettingOptions)
 * \param [out] valueType the data type of the option value
 * \param [out] valueSize the size (in bytes) of the option value
 *
 * \see SettingOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieParameterSetting_GetOptionInfo(
        CvieParameterSetting setting, CvieOptionId option, CvieOptionType* valueType, int* valueSize);
typedef CvieResult(CVIE_DECL* TCvieParameterSetting_GetOptionInfo)(CvieParameterSetting setting, CvieOptionId option,
        CvieOptionType* valueType, int* valueSize); /**< Function pointer type for CvieParameterSetting_GetOptionInfo() */

/**
 * Sets a processing option for a specific parameter setting. This function can be called at any time and the option
 * will be effective for all subsequent Enhance calls.
 *
 * \pre \c setting is a valid ::CvieParameterSetting handle
 * \pre \c option is a valid parameter setting option
 * \pre valueBuffer points to a valid value of the correct type for the selected option
 * \pre valueBufferSize corresponds to the size of the valueBuffer and the expected size of the value type
 * \post The option is effective
 *
 * \param [in] setting the parameter setting to set an option for
 * \param [in] option the option to set (see \ref SettingOptions)
 * \param [in] valueBuffer pointer to the value to set
 * \param [in] valueBufferSize the size (in bytes) of the value to set
 *
 * \note If valueBuffer points to a value that is not of the same type as the option, the results are undefined.
 *
 * \see SettingOptions
 *
 */
CVIE_API CvieResult CVIE_DECL CvieParameterSetting_SetOption(
        CvieParameterSetting setting, CvieOptionId option, const void* valueBuffer, int valueBufferSize);
typedef CvieResult(CVIE_DECL* TCvieParameterSetting_SetOption)(CvieParameterSetting setting, CvieOptionId option,
        const void* valueBuffer, int valueBufferSize); /**< Function pointer type for CvieParameterSetting_SetOption() */


#ifndef CVIE_HEADER_INCLUDED

/** The TCVIEDIInfo struct is set by calls to ::CvieParameterSetting_GetDIInfo to indicate properties of the requested
 *  tuning parameter.  Some parameters return fixed values while some may vary depending on the setting and the currently
 *  set tuning parameter values. */
typedef struct TCVIEDIInfo {
    float nominal_min; /**< Low end of scale for any setting. Lowest allowed value. */
    float current_min; /**< Min value affected by current setting. Can be higher than nominal_min. */

    float neutral_value; /**< The value that does not affect the tuned setting. */
    float loaded_value; /**< The value loaded from file (same as nominal_value unless the setting was saved with tuning parameter values). */
    float set_value;       /**< The value that was most recently set. */
    float effective_value; /**< The effective value, i.e. set_value clamped to the current min/max range at the time it was set. */

    float current_max; /**< Max value affected by current setting. Can be lower than nominal_max. */
    float nominal_max; /**< High end of scale for any setting. Highest allowed value. */
} TCVIEDIInfo;

#endif

/** TCvieDIInfo is a typedef name of TCVIEDIInfo to follow CVIE2 convention. */
typedef struct TCVIEDIInfo TCvieDIInfo;

/**
 * Gets information for a tuning parameter.
 *
 * \pre \c setting is a valid ::CvieParameterSetting handle
 * \pre \c option is a valid tuning parameter number
 * \post, *info will be set to the relevant information about the tuning parameter.
 *
 * \param [in] setting the parameter setting to get information for
 * \param [in] option A value indicating which tuning or API parameter to get info for. Available API parameters are in \ref CVIE_APIPar.
 *                    Available tuning parameters are \ref tp_parameters.
 * \param [out] info is a struct containing information about the parameter.
 *
 */


CVIE_API CvieResult CVIE_DECL CvieParameterSetting_GetDIInfo(CvieParameterSetting setting, CvieOptionId option, TCvieDIInfo* info);
typedef CvieResult(CVIE_DECL* TCvieParameterSetting_GetDIInfo)(CvieParameterSetting setting, CvieOptionId option,
        TCvieDIInfo* info); /**< Function pointer type for CvieParameterSetting_GetDIInfo() */

/** \} */

/** \addtogroup ImageDescriptor
 * \{ */

/**
 * Creates a new image descriptor for a 2D memory buffer.
 *
 * \pre CvieEnter() has been called
 * \pre \c desc is a pointer to a ::CvieImageDescriptor handle
 * \pre \c width is a positive number
 * \pre \c height is a positive number
 * \pre \c format is a valid ::CviePixelFormat value
 * \post \c *desc is a valid ::CvieImageDescriptor handle
 *
 * \param [out] desc the created image descriptor
 * \param [in] width the width of the image in pixels
 * \param [in] height the height of the image in pixels
 * \param [in] format the format of the pixels in the image
 *
 */
CVIE_API CvieResult CVIE_DECL CvieImageDescriptor_CreateForBuffer2D(
        CvieImageDescriptor* desc, int width, int height, CviePixelFormat format);
typedef CvieResult(CVIE_DECL* TCvieImageDescriptor_CreateForBuffer2D)(CvieImageDescriptor* desc, int width, int height,
        CviePixelFormat format); /**< Function pointer type for CvieImageDescriptor_CreateForBuffer2D() */

/**
 * Attaches a memory buffer to a 2D image descriptor. This buffer will be used as
 * the source and/or destination for pixel data when this descriptor is connected to a ::CvieParameterSetting
 * using CvieParameterSetting_Setup().
 *
 * \note This function can be called multiple times without needing to call CvieParameterSetting_Setup() again.
 * When CvieParameterSetting_Enhance() is called, the currently attached buffer(s) will be used for input
 * and output.
 *
 * \note The attached buffer is assumed to be valid until it is detached from the descriptor using CvieImageDescriptor_Detach().
 *
 * \pre \c desc is a ::CvieImageDescriptor handle created using CvieImageDescriptor_CreateForBuffer2D()
 * \pre \c data points to a buffer at least (\c rowStrideInBytes times the height of \c desc) bytes large
 * \pre \c rowStrideInBytes is >= (the width of \c desc times the number of bytes per pixel)
 * \pre \c rowStrideInBytes is divisible by the pixel size (number of bytes)
 * \post the data buffer is attached to the descriptor
 *
 * \param [in] desc the image descriptor to attach to
 * \param [in] data a pointer to the image data buffer
 * \param [in] rowStrideInBytes the number of bytes in each line of the data buffer
 *
 */
CVIE_API CvieResult CVIE_DECL CvieImageDescriptor_AttachBuffer2D(CvieImageDescriptor desc, void* data, int rowStrideInBytes);
typedef CvieResult(CVIE_DECL* TCvieImageDescriptor_AttachBuffer2D)(CvieImageDescriptor desc, void* data,
        int rowStrideInBytes); /**< Function pointer type for CvieImageDescriptor_AttachBuffer2D() */




/**
 * Creates a new image descriptor for a 3D memory buffer.
 *
 * \pre CvieEnter() has been called
 * \pre \c desc is a pointer to a ::CvieImageDescriptor handle
 * \pre \c width is a positive number
 * \pre \c height is a positive number
 * \pre \c depth is a positive number
 * \pre \c format is a valid ::CviePixelFormat value
 * \post \c *desc is a valid ::CvieImageDescriptor handle
 *
 * \param [out] desc the created image descriptor
 * \param [in] width the width of the image in pixels
 * \param [in] height the height of the image in pixels
 * \param [in] depth the depth of the image in pixels
 * \param [in] format the format of the pixels in the image
 *
 */
CVIE_API CvieResult CVIE_DECL CvieImageDescriptor_CreateForBuffer3D(
        CvieImageDescriptor* desc, int width, int height, int depth, CviePixelFormat format);
typedef CvieResult(CVIE_DECL* TCvieImageDescriptor_CreateForBuffer3D)(CvieImageDescriptor* desc, int width, int height, int depth,
        CviePixelFormat format); /**< Function pointer type for CvieImageDescriptor_CreateForBuffer3D() */

/**
 * Attaches a memory buffer to a 3D image descriptor. This buffer will be used as
 * the source and/or destination for pixel data when this descriptor is connected to a ::CvieParameterSetting
 * using CvieParameterSetting_Setup().
 *
 * \note This function can be called multiple times without needing to call CvieParameterSetting_Setup() again.
 * When CvieParameterSetting_Enhance() is called, the currently attached buffer(s) will be used for input
 * and output.
 *
 * \note The attached buffer is assumed to be valid until it is detached from the descriptor using CvieImageDescriptor_Detach().
 *
 * \pre \c desc is a ::CvieImageDescriptor handle created using CvieImageDescriptor_CreateForBuffer3D()
 * \pre \c data points to a buffer at least (\c rowStrideInBytes times the height of \c desc) bytes large
 * \pre \c rowStrideInBytes is >= (the width of \c desc times the number of bytes per pixel)
 * \pre \c rowStrideInBytes is divisible by the pixel size (number of bytes)
 * \pre \c sliceStrideInBytes is >= (the height of \c desc times the width of \c desc times the number of bytes per pixel)
 * \post the data buffer is attached to the descriptor
 *
 * \param [in] desc the image descriptor to attach to
 * \param [in] data a pointer to the image data buffer
 * \param [in] rowStrideInBytes the number of bytes in each line of the data buffer
 * \param [in] sliceStrideInBytes the number of bytes in each slice (z plane) of the data buffer
 *
 */
CVIE_API CvieResult CVIE_DECL CvieImageDescriptor_AttachBuffer3D(
        CvieImageDescriptor desc, void* data, int rowStrideInBytes, int sliceStrideInBytes);
typedef CvieResult(CVIE_DECL* TCvieImageDescriptor_AttachBuffer3D)(CvieImageDescriptor desc, void* data, int rowStrideInBytes,
        int sliceStrideInBytes); /**< Function pointer type for CvieImageDescriptor_AttachBuffer3D() */





/**
 * Detaches the attached data from an image descriptor.
 *
 * \pre \c desc is a valid ::CvieImageDescriptor handle
 * \post \c desc is not attached to any image data previously attached to the descriptor
 *
 * \param [in] desc the image descriptor to detach
 *
 */
CVIE_API CvieResult CVIE_DECL CvieImageDescriptor_Detach(CvieImageDescriptor desc);
typedef CvieResult(CVIE_DECL* TCvieImageDescriptor_Detach)(
        CvieImageDescriptor desc); /**< Function pointer type for CvieImageDescriptor_Detach() */

/**
 * Destroys an image descriptor.
 *
 * \pre \c desc is a pointer to a valid ::CvieImageDescriptor handle
 * \post \c *desc is \c NULL
 * \post the memory used by desc is released
 *
 * \param [in] desc the image descriptor to destroy
 *
 */
CVIE_API CvieResult CVIE_DECL CvieImageDescriptor_Destroy(CvieImageDescriptor* desc);
typedef CvieResult(CVIE_DECL* TCvieImageDescriptor_Destroy)(
        CvieImageDescriptor* desc); /**< Function pointer type for CvieImageDescriptor_Destroy() */

/** \} */

/** \} */

/**
 * \addtogroup DeprecatedDefines
 \{*/
#ifndef CVIE_NO_DEPRECATED_NAMES

#define CVIE_INSTANCE_OPTION_CSTR_PARAMETER_DESCRIPTION 101         /**< \deprecated Old name for ::CVIE_INSTANCE_OPTION_CSTR_PARAMETER_FILE_DESCRIPTION */
#define CVIE_SETTING_OPTION_CSTR_OPERATION_DESCRIPTION 201          /**< \deprecated Old name for ::CVIE_SETTING_OPTION_CSTR_DESCRIPTION */


#endif
/**\} */


#endif /* __CVIE2_HEADER */
