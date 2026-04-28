//=============================================================================
///
/// @file cvie.h
///
/// @brief CVIE C API header
///
/// Copyright (c) 2013-2024 ContextVision AB.
///
//=============================================================================
#ifndef CVIE_HEADER_INCLUDED
#define CVIE_HEADER_INCLUDED
#pragma once

#include <stdint.h>
#include <stddef.h>

#include <stdbool.h>

#include "cvlm.h"

/** \defgroup CVIE CVIE C API Reference
 *
 * \brief Functionality for performing Image Enhancement
 *
 * Defines, structs and functions for the CVIE image enhancement API in the C programming language.
 *
 * Note that for new projects we recommend using the C++ API defined in cvie.hpp instead as it provides type safe operation and
 * automatic resource management (RAII wrappers for the handle types here).
 *
 * CONFIDENTIAL
 * \copyright Copyright (c) ContextVision AB.
 *
 *
 * \par Basic calling sequence
 *
 *  - Create an image enhancement instance by calling CVIECreate()
 *  - Create a worker for a parameter setting with CVIECreateWorker
 *  - Setup image dimension data by adjusting TCVIEPort fields and calling calling CVIESetup()
 *  - Enhance images by calling CVIERun()
 *  - Destroy the worker to free resources used by the setting CVIEDestroyWorker()
 *  - Destroy the instance to free resources used by the instance by calling CVIEDestroy()
 *
 * \par Legacy calling sequence
 *
 *  - Create an image enhancement instance by calling CVIECreate()
 *  - Set a parameter file by calling CVIESetParameterFile()
 *  - Setup image dimension data by calling CVIEEnhanceSetup()
 *  - Enhance images by calling CVIEEnhanceNext()
 *  - Destroy the instance to free resources by calling CVIEDestroy()
 *
 \{
 */

/** \defgroup CVIE_DataTypes Data Types
 * Data types used with CVIEEnhanceSetup() and in TCVIEPort descriptors.
 * Data types are represented by an int value with different bit fields.
 * Main data types are represented by individual bits while auxiliary data is represented by a numeric value.
 * Additional bits are used to encode the memory type where the data is located (GPU or CPU).
 \{
 */

/**
 * Data stored as 16-bit unsigned integers.
 */
#define CVIE_DATA_U16 0x00000001
/**
 * Data stored as 16-bit signed integers.
 */
#define CVIE_DATA_S16 0x00000002
/**
 * Data stored as 8-bit unsigned integers.
 */
#define CVIE_DATA_U8 0x00000004
/**
 * Data stored as 32-bit floating point values.
 */
#define CVIE_DATA_F32 0x00000008

/**
 * Data stored as 64-bit floating point values. This type is not supported for image data.
 */
#define CVIE_DATA_F64 0x00000010

/** Mask defining bits which encode one pixel format each. */
#define CVIE_PIXEL_TYPE_MASK   0x00000FFF

/** Image data is greyscale with just one color channel */
#define CVIE_GREYSCALE_CONFIGURATION      0x00000000

/** Image data is RGB in pixel packed format with three color channels. Currently only available for 2D processing. */
#define CVIE_RGB_CONFIGURATION   0x00001000

/** Image data is RGBA in pixel packed format with three color channels and one alpha channel. The alpha channel is transferred 
    from input to output without modification. Currently only available for 2D processing. */
#define CVIE_RGBA_CONFIGURATION  0x00002000

/** Mask for the bits encoding color configurations. */
#define CVIE_COLOR_CONFIGURATION_MASK  0x0000F000

/** U8 RGB data constant combining two flag fields */
#define CVIE_DATA_RGB            (CVIE_RGB_CONFIGURATION | CVIE_DATA_U8)
/** U8 RGBA data constant combining two flag fields */
#define CVIE_DATA_RGBA           (CVIE_RGBA_CONFIGURATION | CVIE_DATA_U8)






/** Bits encoding auxiliary data types. When this value is non-zero all the ::CVIE_PIXEL_TYPE_MASK bits must be cleared and
    vice versa. */
#define CVIE_DATA_TYPE_MASK      0x0FC00000

/** This mask defines all bits used to encode the element type of the data. */
#define CVIE_ELEMENT_TYPE_MASK   (CVIE_PIXEL_TYPE_MASK | CVIE_DATA_TYPE_MASK)

/** Must be set in the flags field to signify that the data of a ::CVIE_DATA_SETUP_REQUIRED input has changed.
    The flag is cleared by CVIE when it has read the buffer. Note that if ::CVIESetup fails the buffer may or may
    not have been read depending on the type of error. */
#define CVIE_DATA_CHANGED 0x20000000


/**\} CVIE_DataTypes */

/**
 * \defgroup CVIE_CreateFlags Flags for CVIECreate
 * \{
 */

/**
 * Default processing mode. For GPU enabled SDKs this sets up processing for the available GPU processing mode.
 * 
 * \note In previous versions this always selected CPU processing. Now you need to use ::CVIE_CREATE_CPU to get CPU processing if GPU processing is available.
 */
#define CVIE_CREATE_DEFAULT 0


/**
 * Process on CPU for all SDKs. This is the same as ::CVIE_CREATE_DEFAULT if GPU processing is not available.
 */
#define CVIE_CREATE_CPU 256

/**
 * Create an instance that writes trace information to a file called cvielog.dat in the current directory unless renamed 
 * using environment variable COV_TRACE_FILE or parameter ::CVIE_TRACE_PATH.
 * 
 * Enabling tracing by setting this flag when calling ::CVIECreate functions is deprecated, prefer calling CVIESetParameterValue with the ::CVIE_TRACE_MODE parameter id.
 * 
 * \see \ref TraceLog
 */
#define CVIE_CREATE_TRACE 16
/**
 * Create an instance that writes clear text trace information to a file called customer_log.txt in the directory mentioned in
 * environment variable COV_TRACE_FILE or parameter ::CVIE_TRACE_PATH, or by default in the current directory.
 * 
 * Enabling tracing by setting this flag when calling ::CVIECreate functions is deprecated, prefer calling CVIESetParameterValue with the ::CVIE_TRACE_MODE parameter id.
 * 
 * \see \ref TraceLog
 */
#define CVIE_CREATE_CUSTOMER_TRACE 8
/**
 * Clear previous contents of enabled trace files before starting logging. This flag is only used for the first ::CVIECreate call that
 * enables tracing, it won't clear contents logged in the same program run.
 * 
 * Controlling tracing by setting this flag when calling ::CVIECreate functions is deprecated, prefer calling CVIESetParameterValue with the ::CVIE_TRACE_MODE parameter id.
 * 
 * \see \ref TraceLog
 */
#define CVIE_CREATE_CLEAR_TRACE 4




#ifndef CVIE2_HEADER_INCLUDED

#define CVIE_ENABLE_TRACE 1             /**< Constant used to enable encrypted trace log cvielog.dat. */
#define CVIE_ENABLE_CUSTOMER_TRACE 2    /**< Constant used to enable customer plain text log customer_log.txt. */
#define CVIE_CLEAR_TRACE_FILES 4        /**< Constant used clear log files that are enabled now. */

#endif

/**\} CVIE_CreateFlags */

/** \defgroup CVIE_ChannelSelection Color Channel Selection
 * When color input images are used one color channel must be selected for processing.
 *
 * When color output images are used one color channel only is updated.
 \{
*/


#define CVIE_RED_CHANNEL 0          /**< Red color channel */
#define CVIE_GREEN_CHANNEL 1        /**< Green color channel */
#define CVIE_BLUE_CHANNEL 2         /**< Blue color channel */
#define CVIE_ALPHA_CHANNEL 3        /**< Alpha channel. It is possible to process the alpha channel too. */


/**\} CVIE_ChannelSelection */


/** \defgroup CVIE_ParameterTypes Parameter Data Types
 * Data types set by ::CVIEGetParameterInfo
 \{
*/

/** The parameter data is a 32 bit integer */
#define CVIE_PARTYPE_INT 1

/** The parameter data is a 32 bit float */
#define CVIE_PARTYPE_FLOAT 2

/** The parameter data is a zero terminated char array. For ::CVIEGetParameterValue calls the caller must provide a buffer
 * for CVIEGetParameter to store the characters of the string in. By calling ::CVIEGetParameterInfo the currently required size of
 * this buffer can be retrieved. **/
#define CVIE_PARTYPE_STRING 3

/** The parameter data is a pointer to a struct. The exact type of struct varies between parameters. */
#define CVIE_PARTYPE_STRUCT 8

/** The parameter is a mask image. The size is always one byte per pixel. */
#define CVIE_PARTYPE_MASK 10

/** The parameter is a double value. */
#define CVIE_PARTYPE_DOUBLE 11

/** The parameter is a const wchar_t* character string. */
#define CVIE_PARTYPE_WIDE_STRING 12

/** The parameter is a void* handle */
#define CVIE_PARTYPE_HANDLE 13

/**\} CVIE_ParameterTypes */

/* */
/**
 * \defgroup CVIE_SetGetPar Parameters for Set/GetParameterValue
 * These are the possible parameter IDs to use when calling ::CVIEGetParameterInfo, ::CVIEGetParameterValue and ::CVIESetParameterValue.
 *
 * For those parameters that represent a file path (directory or filename) the modifier ::CVIE_WIDE_PATH can be ored to the parameter
 * number to handle the path as a wide string in UTF-16 on Windows and UTF-32 on other operating systems.
 *
 * 
 * \see \ref WideFilenames
 * \see CVIEGetParameterValue
 * \see CVIEGetParameterInfo
 * \see CVIESetParameterValue
 * \{
 */

#ifndef CVIE2_HEADER_INCLUDED
/** Value that can be ored with parameter numbers for file path variables to indicate that the parameter value is a wchar_t*
 ** string.  \see \ref WideFilenames */
#define CVIE_WIDE_PATH 16384
#endif

#define CVIE_MAX_LENGTH 512        /**< Maximum size of string and path parameters. This value is always in bytes. */

/** This parameter is a description of the current parameter file (read-only).
 *
 *  The value parameter must point to a buffer of ::CVIE_MAX_LENGTH chars which will be filled with a null terminated string by
 *  the ::CVIEGetParameterValue function. The setting parameter of the ::CVIEGetParameterValue function is not used when this parameter is
 *  specified.
 */
#define CVIE_PARAMETER_FILE_DESCRIPTION 1

/** This parameter is a description of the selected parameter setting (read-only).
 *
 *  The value parameter must point to a buffer of ::CVIE_MAX_LENGTH chars which will be filled with a null terminated string by
 *  the ::CVIEGetParameterValue function.
 */
#define CVIE_SETTING_DESCRIPTION 2


/** This parameter is a user-settable description of the parameter setting (read/write).
 *
 *  Get: The value parameter must point to a buffer of ::CVIE_MAX_LENGTH chars which will be filled with a null terminated string by
 *  the ::CVIEGetParameterValue function.
 * 
 *  Set: The value parameter must point to a null terminated string of at most ::CVIE_MAX_LENGTH chars, including null. Setting
 *  a longer string is an error.
 */
#define CVIE_CUSTOMER_SETTING_DESCRIPTION 10

/** This parameter returns the string corresponding to the extension of a parameter file containing this type of settings. (read-only).
 *
 *  The value parameter must point to a buffer of ::CVIE_MAX_LENGTH chars which will be filled with a null terminated string by
 *  the ::CVIEGetParameterValue function.
 */
#define CVIE_SETTING_TYPE 9



/**
 * Set ::CVIE_ENABLE_TRACE (bit 0) of this parameter to to enable trace log output. This generates an encrypted log file which can be
 * used by ContextVision engineers to diagnose problems with the processing.
 *
 * Set ::CVIE_ENABLE_CUSTOMER_TRACE (bit 1) of this parameter to enable log output for customer use. This generates a human readable log
 * file with a subset of the ContextVision log contents.
 *
 * Set ::CVIE_CLEAR_TRACE_FILES (bit 2) of this parameter to clear the enabled log file(s) before logging, clearing happens when each
 * trace mode is enabled.
 *
 * The constants can be ored together. For instance setting the mode to CVIE_CLEAR_TRACE_FILES|CVIE_ENABLE_TRACE will start
 * encrypted log tracing after first clearing any pre-existing file.
 * (read-write)
 *
 * \note When read this parameter reports the effective trace mode, including trace mode set by environment variable COV_TRACE_MODE.
 *
 * \see \ref TraceLog
 */
#define CVIE_TRACE_MODE 11

/**
 * \defgroup CVIE_APIPar API Parameters
 *
 * API parameters are used to inform the CVIE library of the conditions of the image exposure.
 *
 * These parameters can be set using ::CVIESetParameterValue, retrieved using ::CVIEGetParameterValue and a comprehensive set of
 * metadata about them, such as the allowed range and neutral value can be retrieved using ::CVIEGetDIInfo.
 *
 * In the CVIE C++ API the corresponding methods are cvie::Worker::setParameterValue, cvie::Worker::getParameterValue and
 * cvie::Worker::getParameterInfo.
 * \ingroup CVIE_CPP
 *
 \{
 */


/** The current relative resolution in X dimension (read/write).
 *
 *  The purpose of this parameter is to adapt the image processing to the resolution of the input image in case it is different from
 *  the resolution for which the current parameter setting was tuned. Minimum value is 0.25 and maximum value is 4.0.
 *
 *  \note The resolution in this case means pixel density, i.e. the number of pixels per inch or millimeter. The relative resolution
 *        is a unitless factor. A value of 1 means that the current resolution is the same as the parameter setting was tuned for. A
 *        value of 1.5 means that the current resolution is 50% higher than what the current parameter setting was tuned for. If the
 *        relative resolution is changed the new values will be used in the next ::CVIEEnhanceNext call.
 *
 * The type of the value is \c float.
 */
#define CVIE_API_RELATIVE_RESOLUTION_X 12

/** The current relative resolution in Y dimension (read/write).
 *
 *  The purpose of this parameter is to adapt the image processing to the resolution of the input image in case it is different
 *  from the resolution for which the current parameter setting was tuned. Minimum value is 0.25 and maximum value is 4.0.
 *
 *  \note The resolution in this case means pixel density, i.e. the number of pixels per inch or millimeter. The relative resolution
 *        is a unitless factor. A value of 1 means that the current resolution is the same as the parameter setting was tuned for. A
 *        value of 1.5 means that the current resolution is 50% higher than what the current parameter setting was tuned for. If the
 *        relative resolution is changed the new values will be used in the next ::CVIEEnhanceNext call.
 *
 * The type of the value is \c float.
 */
#define CVIE_API_RELATIVE_RESOLUTION_Y 13


/* */
/* */

/** Controls the location of the transition start of the transition between the "near field" and "far field" processing zones (read/write).
 *  Near field processing is applied in a zone from the minimum depth (usually zero) to CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START.
 *  Far field processing is applied in a zone from CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END to the maximum depth.
 *  Between CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START and CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END, processing will gradually go from
 *  near field processing to far field processing. It is recommended that the transition zone is not too narrow. If
 *  CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START = 0.0 and CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END = 1.0, the transition zone will contain
 *  the entire image. To have a difference in processing between near and far field, this feature needs to be enabled
 *  in the specific parameter setting used. The magnitude and characteristics of this difference can only be modified
 *  internally, i.e., by creating a new parameter setting. Minimum value is -1.0 and maximum value is 2.0.
 *  CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START must be less than, CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END.
 *  If CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START is changed, the new values will be used in the next ::CVIEEnhanceNext call.
 *
 * The type of the value is \c float.
 */
#define CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START 23

/** Controls the location of the transition end of the "near field" and "far field" processing zones (read/write).
 *  Near field processing is applied in a zone from the minimum depth (usually zero) to CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START.
 *  Far field processing is applied in a zone from CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END to the maximum depth.
 *  Between CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START and CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END, processing will gradually go from
 *  near field processing to far field processing. It is recommended that the transition zone is not too narrow. If
 *  CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START = 0.0 and CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END = 1.0, the transition zone will contain
 *  the entire image. To have a difference in processing between near and far field, this feature needs to be enabled
 *  in the specific parameter setting used. The magnitude and characteristics of this difference can only be modified
 *  internally, i.e., by creating a new parameter setting. Minimum value is -1.0 and maximum value is 2.0.
 *  CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START must be less than, CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END.
 *  If CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END is changed, the new values will be used in the next ::CVIEEnhanceNext call.
 *
 * The type of the value is \c float.
 */
#define CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END 24


/**\} CVIE_APIPar */








/** The string representation of the Loaded parameter file with any modifications due to changes of tuning parameter values (read-only).
 *
 * This parameter can be read back after loading a parameter file and then performing ::CVIESetParameterValue on any of the tuning
 * parameters on any of the the settings in the file. It will return a string representation of the modified parameter file
 * including the set tuning parameter values so that when it is later loaded back the tuning parameters will get the values as set now.
 *
 * The value parameter must point to a buffer of adequate length. As parameter files can be large and it is not feasible to
 * truncate their contents the application must first call ::CVIEGetParameterInfo to get the size of the buffer.
 *
 * \note The settings parameter is not used when retrieving the parameter file, all settings are always included in the string.
 *
 * \note This is a read-only parameter. To load parameter file contents from a buffer use the ::CVIESetParameterBuffer function or the appropriate cvie::Worker constructor.
 */
#define CVIE_PARAMETER_FILE 30

/** The string representation of the Loaded parameter file without modifications due to changes of tuning parameter values (read-only).
 *
 * This parameter can be read back after loading a parameter file. It will return a string representation of the original parameter
 * file delivered by ContextVision regardless of any tuning parameter values set in the loaded parameter file or using
 * ::CVIESetParameterValue.
 *
 * The value parameter must point to a buffer of adequate length. As parameter files can be large and it is not feasible to
 * truncate their contents the application must first call ::CVIEGetParameterInfo to get the size of the buffer.
 *
 * \note The settings parameter is not used when retrieving the parameter file, all settings are always included in the string.
 *
 * \note This is a read-only parameter. To load parameter file contents from a buffer use the ::CVIESetParameterBuffer function.
 */
#define CVIE_ORIGINAL_PARAMETER_FILE 31

/** The string representation of the Loaded parameter file with modifications due to changes of tuning parameter values (read-only).
 *
 * This parameter can be read back after loading a parameter file and then performing ::CVIESetParameterValue on any of the tuning
 * parameters of any of the the settings in the file. It will return a string representation of the modified parameter file as if it
 * was a clean parameter file but with the settings modified by the set tuning parameter values. No tuning parameter values or
 * original Settings are saved. When this string is subsequently loaded back into the CVIE library it is not allowed to further
 * change the tuning parameter values.
 *
 * The value parameter must point to a buffer of adequate length. As parameter files can be large and it is not feasible to
 * truncate their contents the application must first call ::CVIEGetParameterInfo to get the size of the buffer.
 *
 * \note The settings parameter is not used when retrieving the parameter file, all settings are always included in the string.
 */
#define CVIE_FLATTENED_PARAMETER_FILE 32


/** The string representation of the trace file path used for the trace log.
 *
 * The parameter must be set before other \ref CVIEOverview "CVIE C API" API-function calls to take effect. See \ref TraceLog for further details on the trace log.
 Example of a set call using narrow character representation.
 * \code{.c}
 *     std::string tracePath = "/home/user/logfiles/";
 *     ECVIE cvieResult = CVIESetParameterValue(NULL, 0, CVIE_TRACE_PATH, tracePath.c_str());
 * \endcode
 *
 * When getting the path the value parameter must point to a buffer of at least CVIE_MAX_LENGTH bytes. If the parameter
 * value is retrieved it is always the full path of the log file, regardless of if the value was explicitly set, set via
 * environment variable or defaulted. The customer log file will be written in the same directory but under the name
 * customer_log.txt (if enabled).
 */
#define CVIE_TRACE_PATH 33
 /** Wide version of the ::CVIE_TRACE_PATH parameter. \see \ref WideFilenames
  */
#define CVIE_TRACE_PATH_WIDE (CVIE_TRACE_PATH | CVIE_WIDE_PATH)

/**
 * Returns a string containing version information. (read-only)
 * A buffer length of ::CVIE_MAX_LENGTH bytes is enough to represent the string value.
 */
#define CVIE_VERSION_INFO 40

/**
 * Limits the amount of GPU memory used by 3D algorithms. The value is an integer. (read-write)
 *
 * \note The value is in MiB (i.e. multiply by 1024*1024 to get the number of bytes).
 *
 */
#define CVIE_MEMORY_LIMIT 211

/**
 * Returns the amount of CPU or GPU memory currently used. The value is an integer. For GPU this is per Instance, while for CPU this
 * is a global value. (read-only)
 *
 * \note The value is in MiB (i.e. multiply by 1024*1024 to get the number of bytes).
 *
 */
#define CVIE_MEMORY_USAGE 212

/** The filename of the parameter file loaded by a worker (read-only).
 *
 * The value parameter must point to a buffer of at least ::CVIE_MAX_LENGTH bytes, which will be
 * filled by the function. If the parameters were loaded from a buffer rather than a file, an empty string is returned.
 @{
 */
#define CVIE_PARAMETER_FILE_NAME 700
/** Wide version of the ::CVIE_PARAMETER_FILE_NAME parameter. \see \ref WideFilenames */
#define CVIE_PARAMETER_FILE_NAME_WIDE (CVIE_PARAMETER_FILE_NAME | CVIE_WIDE_PATH)
/** @} */

/** The setting index loaded by a worker (read-only).
 *
 * The type of value is \c int.
 */
#define CVIE_PARAMETER_FILE_SETTING_INDEX 701


/** \} CVIE_SetGetPar */


/**
 * \defgroup CVIE_RSSPar Parameters for improved integration to the Context Vision Remote Service System.
 * Extra parameter IDs which can be used when calling CVIEGetParameterInfo() and CVIEGetParameterValue() which should only effect
 * the application when the cvie_remote_service library is present.
 *
 * Both the CVIE_RSS_IDENTIFICATION_ANATOMY_LEVEL and CVIE_RSS_IDENTIFICATION_FILENAME_SETTING parameters return the empty string except when a Remote Service
 * Tool or ContextVision Internal Tuning Tool (referred to as just Tool below) has taken over control over which setting the CVIE
 * library is using for processing. When control has been taken over these read-only parameters will yield string values depending on
 * the Anatomy and Level selected in the controlling Tool user interface. The only difference between these parameters is how the
 * selection is represented. Applications should just use one, depending on which representation is most convenient.
 *
 * Applications don't have to use these parameters but it is advantageous to use check of them periodically and adjust the settings
 * of the machine as if the user had selected the same Anatomy and Level in the machine's user interface as in the controlling
 * Tool. This reduces the risk of combining an Anatomy and Level selected on the machine, controlling the post-processing
 * occurring after the CVIE image enhancement with _another_ Anatomy and Level selected in the controlling Tool. If such erroneous
 * combinations are in effect when reviewing the settings this can impair the quality of the evaluation. This is especially
 * important if the Tool and physical machine are placed far apart so that the tool operator can't see what is actually selected on
 * the machine, and only observes the screen shots sent from the machine to the Tool.
 *
 * \note The application code to implement this can be retained in the production application. The string buffer will always be set to an
 * empty string when the cvie_remote_service library is not present or if it is disabled. The ::CVIEGetParameterValue function call
 * which just sets the string empty is very quick.
 *
 * \{
 */


/** A string representation of the currently selected Anatomy and Level in a controlling Tool (read-only).
 *
 * This parameter will always return the empty string except when a controlling Tool has
 * taken over control over which setting the CVIE library is using for processing. When control has been taken over this
 * parameter will contain a string value representing selected Anatomy and Level in the Remote Service Tool user interface.
 *
 * The value is the Anatomy name followed by a colon and the Level name. No spaces are inserted before or after the colon.
 *
 * A buffer length of ::CVIE_MAX_LENGTH bytes is enough to represent the string value.
 *
 */

#define CVIE_RSS_IDENTIFICATION_ANATOMY_LEVEL 2000

/** A string representation of the currently selected parameter filename and setting index in a controlling Tool (read-only).
 *
 * This parameter will always return the empty string except when a controlling Tool has
 * taken over control over which setting the CVIE library is using for processing. When control has been taken over this
 * parameter will contain a string value representing the parameter file filename and setting index corresponding to the selected
 * Anatomy and Level in the Remote Service Tool user interface.
 *
 * The value is the filename (without directories) followed by a colon and the setting index that is mapped to the selected
 * Anatomy and level in the Tool user interface. No spaces are inserted before or after the colon.
 *
 * A buffer length of ::CVIE_MAX_LENGTH bytes is enough to represent the string value.
 */

#define CVIE_RSS_IDENTIFICATION_FILENAME_SETTING 2001


/** \} CVIE_RSSPar */

/** \} CVIE */

/** \addtogroup CVIE
 * \{
 */

/**
 * The CVIECreate function is used to create and initiate an image enhancement instance.  This function is not
 * performance optimized and should not be called in a processing loop where performance is critical.
 *
 * \param [out] handle A pointer to a HCVIE variable.
 *                     If the function succeeds, it sets the variable with a handle to the instance
 *                     created by the function. This handle is then used in subsequent calls to identify
 *                     the particular instance.  If the function fails, the variable is set to zero.
 * \param [in] flags   See \ref CVIE_CreateFlags. Multiple flags can be OR:ed together.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ECVIE cvie_err;
 *     HCVIE handle;
 *
 *     cvie_err = CVIECreate(&handle, CVIE_CREATE_DEFAULT);
 *
 *     if (cvie_err != CVIE_E_OK) {
 *         fprintf(stderr, "Error in CVIECreate!\n");
 *         return 1;
 *     }
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIECreate(HCVIE* handle, int flags);
typedef ECVIE(CVIE_DECL* TCVIECreate)(HCVIE* handle, int flags); /**< Function pointer type for CVIECreate() */



/**
 * The CVIEDestroy function is used to close the image enhancement instance created by the CVIECreate()
 * function. The function frees any allocated resources.
 *
 * \param [in, out] handle A pointer to ::HCVIE variable specifying which instance to be destroyed.  The
 *                         variable is set to zero by the function and is invalid as a handle after the call.
 *
 * \return If the function succeeds, the returned value is CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling CVEMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *
 *     ECVIE cvie_err;
 *     char errstr[256];
 *
 *     cvie_err = CVIEDestroy(&handle);
 *
 *     if (cvie_err != CVIE_E_OK) {
 *         fprintf(stderr, "CVIEDestroy: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIEDestroy(HCVIE* handle);
typedef ECVIE(CVIE_DECL* TCVIEDestroy)(HCVIE* handle); /**< Function pointer type for ::CVIEDestroy */


/**
 * Release any image memory used by the instance, or if NULL is provided as parameter, all GPU memory used for GPU programs not
 * currently in use. Use of this function may affect the performance of the next enhancement call, or if NULL is provided the
 * performance of succeeding setup calls.
 * 
 *
 * \param [in] handle A ::HCVIE handle specifying the instance or NULL.
 *
 * \return If the function succeeds, the returned value is CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling CVEMGetLastError().
 *
 * \note For CPU instances image memory for all instances is released as CPU memory is shared between CPU instances.
 */
CVIE_API ECVIE CVIE_DECL CVIEReleaseMemory(HCVIE handle);
typedef ECVIE(CVIE_DECL* TCVIEReleaseMemory)(HCVIE handle); /**< Function pointer type for CVIEReleaseMemory() */

/**
 * Prepare the CVIE library for exiting the program. This function may be called even if there are Workers, but these
 * can no longer be used after this call. The same goes for legacy settings that have been successfully set up using
 * ::CVIEEnhanceSetup. Failing to call CVIEExit may cause spurious errors when subordinate libraries are
 * unloaded, especially if AI is used by the algorithm.
 *
 * After calling this function all other CVIE functions may fail for pre-existing Workers and settings as some essential objects
 * have been destroyed. New Workers can be created after CVIEExit has been called. Old Workers must still be
 * destroyed using ::CVIEDestroyWorker. When using the legacy CVIE functions ::CVIESetParameterFile or ::CVIESetParameterBuffer
 * followed by ::CVIEEnhanceSetup these must be called again after calling ::CVIEExit before the settings are usable.
 *
 * \return If the function succeeds, the returned value is CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling ::CVEMGetLastError.
 *
 */
CVIE_API ECVIE CVIE_DECL CVIEExit();
typedef ECVIE(CVIE_DECL* TCVIEExit)();          /**< Function pointer type for ::CVIEExit */


/* Extra flags in 'allowed' field. */
#define CVIE_DATA_OPTIONAL_INPUT 0x10000000     /**< Input is optional. Set in allowed field. */
#define CVIE_DATA_PARTIAL_OUTPUT 0x10000000     /**< Output is partially filled by Run. (last dimension only).  Set in allowed field. */
#define CVIE_DATA_SETUP_REQUIRED 0x20000000     /**< Setup must be redone after changing contents of this input.  Set in allowed field. */
#define CVIE_DATA_OUTPUT         0x40000000     /**< Port is an output. Set in allowed field. */

#define CVIE_MAX_RANK 6                         /**< 6 dimensional ports max. Set to be enough for any conceivable use case. */

/** Description of a input or output in a Worker. When created the ports are set up by the CVIE library depending on the Setting.
 ** Sizes and flags are set up by the application before calling CVIESetup. */
typedef struct TCVIEPort {
    const char name[32];    /**< Name of port */
    int flags;              /**< Combination of element type, channel combination and memory type that the data pointer refers to. */

    /** allowed Contains a set of allowed data types. Setup initially by CVIECreateWorker. Also contains the output flag
        and a flag indicating (for inputs) that setup has to be called again if its contents is changed. From the C++ API this is
        indicated by attaching the buffer (even if it has the same address). This is used for Mask and similar inputs.
        It also contains flags for different GPU modes. */
    const int allowed;      /**< Set by CVIECreateWorker* to indicate what is allowed for this port. Must not be changed by application. Includes a possible CVIE_DATA_SETUP_REQUIRED flag on inputs. */
    int channel;            /**< Used with color data to indicate how processing should pick input and inject output data. */
    float min_value;        /**< Range of input/output values. Set from the selected setting by CVIECreateWorker*(). */
    float max_value;        /**< Used in CVIESetup to adjust range of the input/output. Can't be changed between run calls. */

    const int rank;         /**< Dimensionality of port. Usually 2 (for images) or 3 (for volumes) but could be 0 for scalars or 1 for vectors. */
    int size[CVIE_MAX_RANK];/**< Prepare for ranks up to 6. This should be plenty. */
    int row_bytes;          /**< Row stride in bytes, used if rank >= 2. */
    int plane_bytes;        /**< Plane stride in bytes, used if rank >=3. For higher ranks volumes are assumed to be packed. */
    int result_count;       /**< Number of elements actually filled out of size[rank - 1]. Set by each CVIERun. */
    union {
        void* data;         /**< Data points to host memory or GPU buffer or handle depending on flags. */
        uint64_t offset;    /**< Offset used internally when communicating with server. Makes sure 32 and 64 bit structs have same size */
    };
} TCVIEPort;

#define CVIE_MAX_PORTS 10       /**< Max number of ports. Actual number is in input_count and output_count depending on the Setting selected. */
#define CVIE_WORKER_VERSION 1   /**< Worker struct version. Makes sure this code on the customer side and the CVIE Library have corresponding versions. */

/** A TCVIEWorker specifies the processing detail of a worker. All TCVIEWorkers are created by the CVIE library. The CreateWorker functions allocates a
 * TCVIEWorker and sets up the incoming CVIEWorker pointer to point at it. The contract is that CVIE has read/write access to it only
 * when one of the functions taking a CVIEWorker is called, otherwise the application is free to change it, except for the const fields
 * which are fixed after CVIECreateWorker function initiates them. All other functions that take a CVIEWorker check that its address is one
 * of the workers it has allocated. */
typedef struct TCVIEWorker {
    union {
        const HCVIE handle;             /**< HCVIE handle of the instance the worker belongs to */
        const uint64_t pad;             /**< only to make sure struct is same size in 32 and 64 bit builds. Used by server. */
    };
    const int id;                       /**< Worker id within the instance. */

    const int input_count;              /**< Number of inputs for the active setting. Note that some inputs can be optional. */
    TCVIEPort inputs[CVIE_MAX_PORTS];   /**< Description of each input port. Only the input_count first ones are used. */
    const int output_count;             /**< Number of outputs. All outputs are optional. */
    TCVIEPort outputs[CVIE_MAX_PORTS];  /**< Description of each output port. If the data member of the port is NULL the output is not generated. */
} TCVIEWorker;
typedef TCVIEWorker* CVIEWorker;        /**< Pointer to a TCVIEWorker struct. Used in most Worker related function APIs */

/* The meaning of HCVIE is changed to be a "device". This means that it can support _Workers_ which have settings from different
   parameter files. Workers are represented by TCVIEWorker structs. A new function pair CVIECreateWorker and
   CVIECreateWorkerFromBuffer are used to create individual workers from one setting in a file/buffer. A new function
   CVIEDestroyWorker can be used to destroy an individual worker, freeing up its resources.
   
   Calling CVIEDestroy for a HCVIE with active workers is an error. */

/**@{*/
/** CreateWorkerVer functions are the ones implemented in the dynamic library. They take a version parameter which must match between
   the value provided by the application and the version expected by the dynamic library. This prevents usage of code compiled with old header
   file versions and thereby allows CV a possibility to update the struct contents of TCVIEWorker and TCVIEPort. */
CVIE_API ECVIE CVIE_DECL CVIECreateWorkerVer(CVIEWorker* worker, HCVIE handle, const char* filename, int setting, int version);

CVIE_API ECVIE CVIE_DECL CVIECreateWorkerVerW(CVIEWorker* worker, HCVIE handle, const wchar_t* filename, int setting, int version);

CVIE_API ECVIE CVIE_DECL CVIECreateWorkerFromBufferVer(CVIEWorker* worker, HCVIE handle, const char* buffer, int length, int setting, int version);
/**@}*/

/** Function pointer type for CVIECreateWorkerVer */
typedef ECVIE (CVIE_DECL* TCVIECreateWorkerVer)(CVIEWorker* worker, HCVIE handle, const char* filename, int setting, int version); 
/** Function pointer type for CVIECreateWorkerVerW */
typedef ECVIE (CVIE_DECL* TCVIECreateWorkerVerW)(CVIEWorker* worker, HCVIE handle, const wchar_t* filename, int setting, int version);  
/** Function pointer type for CVIECreateWorkerFromBufferVer */
typedef ECVIE (CVIE_DECL* TCVIECreateWorkerFromBufferVer)(CVIEWorker* worker, HCVIE handle, const char* buffer, int length, int setting, int version);

/** Create a Worker in the instance specified by handle, using a Setting from the parameter file given by the narrow character name. 
    The filename length is limited to ::CVIE_MAX_LENGTH bytes.
*/
static inline ECVIE CVIECreateWorker(CVIEWorker* worker, HCVIE handle, const char* filename, int setting)
{
    return CVIECreateWorkerVer(worker, handle, filename, setting, CVIE_WORKER_VERSION);
}

/** Create a Worker in the instance specified by handle, using a Setting from the parameter file given  by the wide character name. 
    The filename length is limited to ::CVIE_MAX_LENGTH bytes.
*/
static inline ECVIE CVIECreateWorkerW(CVIEWorker* worker, HCVIE handle, const wchar_t* filename, int setting)
{
    return CVIECreateWorkerVerW(worker, handle, filename, setting, CVIE_WORKER_VERSION);
}

/** Create a Worker using a setting from the parameter file contents in buffer. */
static inline ECVIE CVIECreateWorkerFromBuffer(CVIEWorker* worker, HCVIE handle, const char* buffer, int length, int setting)
{
    return CVIECreateWorkerFromBufferVer(worker, handle, buffer, length, setting, CVIE_WORKER_VERSION);
}

/**@{*/
/** Just as for CVIEDestroy the worker pointer is set to NULL by this function. Note that it is an error to destroy the HCVIE with
 ** active workers. */
CVIE_API ECVIE CVIE_DECL CVIEDestroyWorker(CVIEWorker* worker);
/**@}*/

/** Function pointer type for CVIEDestroyWorker */
typedef ECVIE (CVIE_DECL* TCVIEDestroyWorker)(CVIEWorker* worker);



/**@{*/

/** To handle that different workers can setup the same setting in the same parameter file and do different TP value changes, we
    need a new variant of SaveParameterFile which only incorporates TP changes made in a specific worker. It does save all settings
    from the original parameter file, but only adjusts TP values of the one selected when the worker was created.
    The filename length is limited to ::CVIE_MAX_LENGTH bytes.
*/
CVIE_API ECVIE CVIE_DECL CVIESaveWorkerParameterFile(CVIEWorker worker, const char* filename);
CVIE_API ECVIE CVIE_DECL CVIESaveWorkerParameterFileW(CVIEWorker worker, const wchar_t* filename);

/**@}*/

/** Function pointer type for TCVIESaveWorkerParameterFile */
typedef ECVIE (CVIE_DECL* TCVIESaveWorkerParameterFile)(CVIEWorker worker, const char* filename);     
/** Function pointer type for TCVIESaveWorkerParameterFileW */
typedef ECVIE (CVIE_DECL* TCVIESaveWorkerParameterFileW)(CVIEWorker worker, const wchar_t* filename); 


/**@{*/
/** CVIESetup is called when the application has filled in or changed flags and input sizes. If these are consistent and
 ** allowed by the Setting, CVIE_E_OK is returned after setting up output sizes. Optionally the application can also change the
 ** color channel, value range and line/plane strides of each input and output before calling CVIESetup.
 **
 ** It is not necessary to call Setup after changing the data pointers unless the CVIE_DATA_SETUP_REQUIRED bit is set in the allowed
 ** field of the port. This bit is set by the library for inputs which are not read in each enhancement, such as input
 ** mask. For changed data to take effect for an input marked with CVIE_DATA_SETUP_REQUIRED CVIESetup must be called, setting the
 ** CVIE_DATA_CHANGED bit to indicate that the CVIE library should re-read the data. If CVIESetup successfully reads the data it
 ** will clear CVIE_DATA_CHANGED, but if it encounters an error it may not, and the application must
 ** keep the data available until a successful CVIESetup is performed.
 **
 ** If a port size is zero when CVIESetup is called it is filled in with the size that the enhancement will produce. This is
 ** often the same as the primary input size. If the port size is non-zero but differs from the calculated size the data pointer
 ** is cleared and an error is returned. If the size is set correctly no error occurs.
 **
 ** \note The temporal state is always cleared when calling CVIESetup, so this function should not be called between consecutive
 ** frames of an image sequence. For more information, see \ref TemporalStateReset.
 */
CVIE_API ECVIE CVIE_DECL CVIESetup(CVIEWorker worker);
/**@}*/

/** Function pointer type for CVIESetup */
typedef ECVIE (CVIE_DECL* TCVIESetup)(CVIEWorker worker);

/**@{*/
/** Perform enhancement according to the worker setup. If changes of any field except data has been done after CVIESetup
 ** was last called this is an error. Only the data pointers of inputs and outputs may be changed between CVIERun calls.

 ** \note Inputs with the CVIE_DATA_SETUP_REQUIRED bit set does not read the data in each CVIERun.
 */
CVIE_API ECVIE CVIE_DECL CVIERun(CVIEWorker worker);
/**@}*/

/** Function pointer type for CVIERun */
typedef ECVIE (CVIE_DECL* TCVIERun)(CVIEWorker worker);

/**@{*/
/** Reset the temporal state, or do nothing if there is no temporal state.
 ** For more information, see \ref TemporalStateReset.
 */
CVIE_API ECVIE CVIE_DECL CVIEResetTemporalState(CVIEWorker worker);
/**@}*/

/*' Function pointer type for CVIEResetTemporalState */
typedef ECVIE(CVIE_DECL* TCVIEResetTemporalState)(CVIEWorker worker);


/**
 * The CVIESetParameterFile function loads the library with the settings of one parameter file. Parameter files containing one or
 * more settings are produced during tuning instances performed by ContextVision personnel. When using this legacy API one parameter
 * file at a time may be used for a specific CVIE instance. To avoid this limitation use the newer Worker based API or the C++
 * wrapper. This function is not performance optimized and should not be called in a
 * processing loop where performance is critical.
 *
 * The CVIESetParameterFileW function works the same but takes the parameter filename as a wide string.
 * \see WideFilenames
 *
 * Parameter files loaded from disk files are cached in memory. When CVIESetParameterFile is called again with the same file name
 * the cached contents is used unless the time stamp has changed. On file systems where time stamps are coarse it may be possible
 * to write code that produces two distinct file contents with the same time stamp, thus fooling the cache.
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] fileName A pointer to a string containing the full path of the parameter file to be used.
 *                      The string shall use the character encoding defined by the current system code page, except for the W
 *                      function version. The filename length is limited to ::CVIE_MAX_LENGTH bytes.
 * \param [out] settings A pointer to an integer variable. The function sets the variable value to the number
 *                       of settings included in the parameter file. May be NULL.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *     int nsettings;
 *
 *     cvie_err = CVIESetParameterFile(handle, "ParameterFile.gop", &nsettings);
 *
 *     if (cvie_err != CVIE_E_OK) {
 *         fprintf(stderr, "CVIESetParameterFile: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIESetParameterFile(HCVIE handle, const char* fileName, int* settings);
/** Function pointer type for CVIESetParameterFile() */
typedef ECVIE(CVIE_DECL* TCVIESetParameterFile)(HCVIE handle, const char* fileName, int* settings);

/** Wide version of ::CVIESetParameterFile */
CVIE_API ECVIE CVIE_DECL CVIESetParameterFileW(HCVIE handle, const wchar_t* fileName, int* settings);
/** Function pointer type for CVIESetParameterFileW() */
typedef ECVIE(CVIE_DECL* TCVIESetParameterFileW)(HCVIE handle, const wchar_t* fileName, int* settings);

/**
 * This function is an alternative to the ::CVIESetParameterFile function; the only
 * difference is that ::CVIESetParameterBuffer takes a buffer that contains the whole content of a
 * parameter file as argument, whereas ::CVIESetParameterFile takes a path to a parameter file as argument.
 * This function is not performance optimized and should not be called in a processing loop where performance
 * is critical.
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] buffer A pointer to a null-terminated buffer containing the full content of the parameter file to be used.
 * \param [out] settings A pointer to an integer variable. The function sets the variable value to the number
 *                       of settings included in the parameter file. May be NULL.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *
 *     char* parbuffer;
 *     char errstr[256];
 *     int nsettings;
 *
 *     // Fill parbuffer with whole content of a parameter file
 *     // For example from a database or other external storage
 *     parbuffer = load_parameters();
 *
 *     cvie_err = CVIESetParameterBuffer(handle, parbuffer, &nsettings);
 *
 *     if (cvie_err != CVIE_E_OK) {
 *         fprintf(stderr, "CVIESetParameterBuffer: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIESetParameterBuffer(HCVIE handle, const char* buffer, int* settings);
/** Function pointer type for CVIESetParameterBuffer() */
typedef ECVIE(CVIE_DECL* TCVIESetParameterBuffer)(HCVIE handle, const char* buffer, int* settings);


/**
 * The CVIESaveParameterFile function saves the currently loaded Parameter file or buffer with all set tuning parameter values to a named
 * file. When this file is subsequently loaded using CVIESetParameterFile the tuning parameters get their values from
 * the file. The computer where the file is loaded does not have to be licensed for Doctors Interface to be able to load the file.
 *
 * 
 * The CVIESaveParameterFileW function works the same but takes the parameter filename as a wide string.
 * \see WideFilenames
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] filename A pointer to a string containing the full path of the parameter file to be saved.
 *                      The string shall use the character encoding defined by the current system code page, except for the W
 *                      function version. The filename length is limited to ::CVIE_MAX_LENGTH bytes.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \note This function is not useful if no tuning parameters have been changed as the saved file would just be a copy of
 * the currently loaded file.
 *
 * \note Using this function is equivalent to retrieving the currently loaded file using the ::CVIE_PARAMETER_FILE parameter to
 * ::CVIEGetParameterValue function and then saving the result to a file.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *
 *     cvie_err = CVIESaveParameterFile(handle, "MyFileName.gop");
 *
 *     if (cvie_err != CVIE_E_OK) {
 *         fprintf(stderr, "CVIESaveParameterFile: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIESaveParameterFile(HCVIE handle, const char* filename);
/** Function pointer type for ECVIESaveParameterFile() */
typedef ECVIE(CVIE_DECL* TCVIESaveParameterFile)(HCVIE handle, const char* filename);

/** Wide version of ::CVIESaveParameterFile */
CVIE_API ECVIE CVIE_DECL CVIESaveParameterFileW(HCVIE handle, const wchar_t* filename);
/** Function pointer type for ECVIESaveParameterFileW() */
typedef ECVIE(CVIE_DECL* TCVIESaveParameterFileW)(HCVIE handle, const wchar_t* filename);


/**
 * This function is used to specify image dimension and data type.  The data is then used in subsequent calls
 * to ::CVIEEnhanceNext to enhance images with same dimension and data type.  This function is not
 * performance optimized and should not be called in a processing loop where performance is critical.
 *
 *
 * \note When calling the function the temporal state is reset.
 *       This means that image sequence processing algorithms gets the indication that the next frame to
 *       process is the first frame in the sequence.  In other words, this function should not be called
 *       between consecutive frames of an image sequence. For more information, see \ref TemporalStateReset.
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] width Specifies the width of the image in pixels.
 * \param [in] height Specifies the height of the image in pixels.
 * \param [in] flags Specifies pixel data type for the InImage and OutImage buffers passed to ::CVIEEnhanceNext.
 *                   See \ref CVIE_DataTypes.  Multiple flags can be OR:ed together.
 *
 * \param [in] setting Specifies which setting in the parameter file to use.
 * \param [in] mask A pointer to an image buffer containing a mask.
 *                  The buffer must be of the same size as the input image
 *                  buffer, and the pixel type must be unsigned 8-bit integer (U8).
 *
 * The mask specifies which pixels that are affected by image processing. The mask should have non-zero values
 * in these positions. The mask is used in subsequent calls to ::CVIEEnhanceNext until calling \ref
 * CVIEDestroy.
 *
 * If Mask is set to NULL masking is not used, and the whole image is processed.
 *
 *
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *     int setting = 0;
 *
 *     cvie_err = CVIEEnhanceSetup(handle, wid, hgh, dataType, setting, NULL);
 *
 *     if (cvie_err != CVIE_E_OK) {
 *         fprintf(stderr, "CVIEEnhanceSetup: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIEEnhanceSetup(HCVIE handle, int width, int height, int flags, int setting, const unsigned char* mask);
/** Function pointer type for CVIEEnhanceSetup() */
typedef ECVIE(CVIE_DECL* TCVIEEnhanceSetup)(HCVIE handle, int width, int height, int flags, int setting, const unsigned char* mask);

/**
 * The CVIEEnhanceNext function performs image enhancement when image data is already set by the \ref
 * CVIEEnhanceSetup function.  This function is performance optimized and is the only function that should be
 * called inside a performance critical processing loop when real-time performance is needed.
 *
 *
 * The image shall be stored starting with all the pixels of the first line followed by all the pixels of the
 * second line and so on. There shall be no padding between pixels or lines. Each line shall contain as many
 * pixels as specified by the width parameter in ::CVIEEnhanceSetup.  The image shall contain as many lines
 * as specified by the height parameter ::CVIEEnhanceSetup.  The pixels shall be formatted as specified by
 * the flags parameter in ::CVIEEnhanceSetup.  The width, height and pixel pixel type shall be the same for
 * both input and output image.  In-place processing is supported, meaning that the input and output image may
 * use the same buffer.
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] inImage A pointer to the input image buffer containing the image to be enhanced.
 * \param [out] outImage A pointer to the location where to put the enhanced result.
 *                       The buffer must be of the same size as the input image buffer.
 * \param [in] setting Specifies which setting in the parameter file to use.
 *                     For instance, setting = 0 indicates that the first setting shall be used.
 *
 *
*
*
 * \note The setting to be used must have been setup by a call to ::CVIEEnhanceSetup.
 *       Example: If there are 3 settings in the parameter file,
 *       ::CVIEEnhanceSetup can be called three times to setup for
 *       all settings, and then ::CVIEEnhanceNext can be called with
 *       any setting 0, 1 or 2.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *
 *     cvie_err = CVIEEnhanceNext(handle, pInImage, pOutImage, setting);
 *
 *     if (cvie_err != CVIE_E_OK) {
 *         fprintf(stderr, "CVIEEnhanceNext: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIEEnhanceNext(HCVIE handle, const void* inImage, void* outImage, int setting);
/** Function pointer type for CVIEEnhanceNext() */
typedef ECVIE(CVIE_DECL* TCVIEEnhanceNext)(HCVIE handle, const void* inImage, void* outImage, int setting);


/**
 * The CVIESetThreads function sets the number of threads to be created by the library during image
 * enhancement.  If this function is not called, the library sets the number of threads itself (normally the
 * number of physical CPU cores in the computer).
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] threads The number of threads to be created during image enhancement for this instance.
 *
 * \note If the number of threads is set to 0 or below, the
 *       library sets the number of threads itself (normally the number of
 *       physical CPU cores in the computer).
 *
 * \note CVIESetThreads will cause preexisting workers or settings to re-setup on the next call to \ref
         CVIERun or ::CVIEEnhanceNext. To avoid this, call ::CVIESetup or ::CVIEEnhanceSetup to
         do the setup in advance.
 *
 * \warning There is no upper limit on the number of threads; however, if the number is set extremely high (in
 *          the magnitude of thousands of threads) a lot of memory will be used for thread management, which
 *          may cause a severe system error. There is no benefit in setting the value higher than the number
 *          of logical CPU cores in the system.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *
 *     cvie_err = CVIESetThreads(handle, 7);
 *
 *     if (cvie_err != CVIE_E_OK) {
 *         fprintf(stderr, "CVIESetThreads: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIESetThreads(HCVIE handle, int threads);
typedef ECVIE(CVIE_DECL* TCVIESetThreads)(HCVIE handle, int threads); /**< Function pointer type for CVIESetThreads() */


/**
 * The CVIEGetParameterInfo function is used to get information about a specific parameter.
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] setting Specifies which parameter setting to get information for.
 * \param [in] parameter A value indicating which parameter to get information for. (see \ref CVIE_SetGetPar)
 * \param [out] type A pointer to an int receiving the parameter type, one of the CVIE_PARTYPE_* values.
 * \param [out] length A pointer to an int receiving the parameter length, for strings including the trailing NUL character.
 *              The length is in bytes even for wide strings. To get length in wide characters divide by sizeof(wchar_t).
 *
 * \see CVIE_ParameterTypes
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *
 *     int type, length;
 *
 *     cvie_err = CVIEGetParameterInfo(handle, setting, CVIE_SETTING_DESCRIPTION, &type, &length);
 *
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "Operation description length: %d\n", length);
 *     else {
 *         fprintf(stderr, "CVIEGetParameterInfo: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIEGetParameterInfo(HCVIE handle, int setting, int parameter, int* type, int* length);
/** Function pointer type for CVIEGetParameterInfo() */
typedef ECVIE(CVIE_DECL* TCVIEGetParameterInfo)(HCVIE handle, int setting, int parameter, int* type, int* length);

/** Get parameter info given a CVIEWorker */
static inline ECVIE CVIEWorkerGetParameterInfo(CVIEWorker worker, int parameter, int* type, int* length)
{
    return CVIEGetParameterInfo(worker->handle, worker->id, parameter, type, length);
}


#ifndef CVIE2_HEADER_INCLUDED

/** The TCVIEDIInfo struct is set by calls to ::CVIEGetDIInfo to indicate properties of the requested tuning parameter.
 *  Some parameters return fixed info values while some may vary depending on the setting and the currently
 *  set tuning parameter values. */
typedef struct TCVIEDIInfo {
    float nominal_min; /**< Low end of scale for any setting. Lowest allowed value. */
    float current_min; /**< Min value affected by current setting. Can be higher than nominal_min. */

    float neutral_value; /**< The value that does not affect the tuned setting. */
    float loaded_value; /**< The value loaded from file (same as neutral_value unless the setting was saved with tuning parameter values). */
    float set_value;       /**< The value that was most recently set. */
    float effective_value; /**< The effective value, i.e. set_value clamped to the current min/max range at the time it was set. */

    float current_max; /**< Max value affected by current setting. Can be lower than nominal_max. */
    float nominal_max; /**< High end of scale for any setting. Highest allowed value. */
} TCVIEDIInfo;
#endif


/**
 * The CVIEGetDIInfo function is used to get information about a specific Tuning or API parameter.
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] setting Specifies which parameter setting that should be read.
* \param [in] parameter A value indicating which tuning or API parameter to get info for. Available API parameters are in \ref CVIE_APIPar.
*                       Available tuning parameters are \ref tp_parameters.
 * \param [out] info A pointer to a ::TCVIEDIInfo struct receiving the parameter information.
 *
 * \see CVIE_ParameterTypes
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *
 *     TCVIEDIInfo info;
 *
 *     cvie_err = CVIEGetDIInfo(handle, setting, SomeDIParameter, &info);
 *
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "Nominal min, max: %f, %f\n", info.nominal_min, info.nominal_max);
 *     else {
 *         fprintf(stderr, "CVIEGetDIInfo: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */

CVIE_API ECVIE CVIE_DECL CVIEGetDIInfo(HCVIE handle, int setting, int parameter, TCVIEDIInfo* info);
/** Function pointer type for CVIEGetDIInfo() */
typedef ECVIE(CVIE_DECL* TCVIEGetDIInfo)(HCVIE handle, int setting, int parameter, TCVIEDIInfo* info);

/** Get Tuning or API parameter info given a CVIEWorker. */
static inline ECVIE CVIEWorkerGetDIInfo(CVIEWorker worker, int parameter, TCVIEDIInfo* info)
{
    return CVIEGetDIInfo(worker->handle, worker->id, parameter, info);
}


/**
 * The CVIEGetParameterValue function is used to read specific parameter values.
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
 * \param [in] setting Specifies which parameter setting that should be read.
 * \param [in] parameter A value indicating which parameter to read. (see \ref CVIE_SetGetPar)
 * \param [out] value A pointer to a buffer receiving the parameter value.
 *                    The type and size of the value depends on the specified parameter ID. The caller must
 *                    ensure that the buffer is big enough to hold the value.
 *
 * \see CVIE_SetGetPar
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling \ref CVEMGetLastError.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *     char buffer[512];
 *
 *     cvie_err = CVIEGetParameterValue(handle, setting, CVIE_SETTING_DESCRIPTION, buffer);
 *
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "Operation description: %s\n", buffer);
 *     else {
 *         fprintf(stderr, "CVIEGetParameterValue: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIEGetParameterValue(HCVIE handle, int setting, int parameter, void* value);
/** Function pointer type for CVIEGetParameterValue() */
typedef ECVIE(CVIE_DECL* TCVIEGetParameterValue)(HCVIE handle, int setting, int parameter, void* value);

/** Get parameter value given a CVIEWorker. */
static inline ECVIE CVIEWorkerGetParameterValue(CVIEWorker worker, int parameter, void* value)
{
    return CVIEGetParameterValue(worker->handle, worker->id, parameter, value);
}


/**
 * The CVIESetParameterValue function is used to set specific parameter values.
 *
 * \param [in] handle Specifies a handle to a CVIE instance, created when CVIECreate() is called.
 * \param [in] setting Specifies which parameter setting that should be changed.
 * \param [in] parameter A value indicating which parameter to change.  See \ref CVIE_SetGetPar.

 *
 * \param [in] value A pointer to the parameter value to set.

 *                   The type and size of the value depends on the specified parameter ID.
 *
 * \see CVIE_SetGetPar
 *
 * \return If the function succeeds, the returned value is CVIE_E_OK, else the returned value is an error code.
 *         Extended error information may be available by calling CVEMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *     float value = 0.5f;
 *
 *     cvie_err = CVIESetParameterValue(handle, setting, CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START, &value);
 *
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "Parameter successfully set");
 *     else {
 *         fprintf(stderr, "CVIESetParameterValue: %s\n", CVEMGetLastError(handle, errstr, sizeof(errstr)));
 *         return 1;
 *     }
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVIESetParameterValue(HCVIE handle, int setting, int parameter, const void* value);
/** Function pointer type for CVIESetParameterValue() */
typedef ECVIE(CVIE_DECL* TCVIESetParameterValue)(HCVIE handle, int setting, int parameter, const void* value);

/** Set parameter value given a CVIEWorker. */
static inline ECVIE CVIEWorkerSetParameterValue(CVIEWorker worker, int parameter, const void* value)
{
    return CVIESetParameterValue(worker->handle, worker->id, parameter, value);
}



/** \} CVIE */


#ifndef CVIE_NO_DEPRECATED_NAMES

/* Deprecated functions. Should not be used in new code */

/**
 * \addtogroup CVIE
 * \{
 */

/**
 * \deprecated This function is not supported anymore.
 *
 * \return ::CVIE_E_NOT_SUPPORTED
 */
CVIE_API_DEPRECATED("Not supported anymore")
ECVIE CVIE_DECL CVSLImageLut(HCVIE handle, void* inImage, void* outImage, int height, int width, int flags, char* fileName);
/** \deprecated Function pointer type for CVSLImageLut() */
typedef ECVIE(CVIE_DECL* TCVSLImageLut)(HCVIE handle, void* inImage, void* outImage, int height, int width, int flags, char* fileName);

/** The CVIEEnhance function performs setup and image enhancement in a single function
  *
  * \deprecated Use CVIEEnhanceSetup() and CVIEEnhanceNext() separately instead, which results in better
  *             performance when processing multiple images.
  *
  * \param [in]  handle   Specifies a handle to a CVIE instance, created when ::CVIECreate is called.
  * \param [in]  inImage  A pointer to the input image buffer containing the image to be enhanced.
  * \param [out] outImage A pointer to the location where to put the enhanced result.
  *                       The buffer must be of the same size as the input image buffer.
  * \param [in]  width    Specifies the width of the image in pixels.
  * \param [in]  height   Specifies the height of the image in pixels.
  * \param [in]  flags    Specifies pixel data type for the \p inImage and \p outImage buffers.
  *                       See \ref CVIE_DataTypes.  Multiple flags can be OR:ed together.
  *
  * \param [in]  setting  Specifies which setting in the parameter file to use.
  * \param [in]  mask     A pointer to an image buffer containing a mask.
  *                       The buffer must be of the same size as the input image
  *                       buffer, and the pixel type must be unsigned 8-bit integer (U8).
  *
  * \see CVIEEnhanceSetup()
  * \see CVIEEnhanceNext()
  *
  * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code.
  *         Extended error information may be available by calling \ref CVEMGetLastError.
  */
CVIE_API_DEPRECATED("Use CVIEEnhanceSetup() and CVIEEnhanceNext() separately instead, which results in better performance when processing multiple images.")
ECVIE CVIE_DECL CVIEEnhance(HCVIE handle, const void* inImage, void* outImage, int width, int height, int flags, int setting, const unsigned char* mask);
/** \deprecated Function pointer type for CVIEEnhance() */
typedef ECVIE(CVIE_DECL* TCVIEEnhance)(HCVIE handle, const void* inImage, void* outImage, int width, int height, int flags, int setting, const unsigned char* mask);

/**
 * \deprecated This function is deprecated as it was designed when computers were much slower than today. Also, just as
 *             so many progress bars, it was never very accurate.
 */
CVIE_API_DEPRECATED("This function is deprecated as it was designed when computers were much slower than today.")
ECVIE CVIE_DECL CVIESetProgressCallback(HCVIE handle, int (*CallbackFunction)(float percent));
/** \deprecated Function pointer type for CVIESetProgressCallback() */
typedef ECVIE(CVIE_DECL* TCVIESetProgressCallback)(HCVIE handle, int (*CallbackFunction)(float percent));
/** \} CVIE */

/**
 * \addtogroup DeprecatedDefines
 \{*/

#define MAX_DESCRIPTION_LENGTH 512    /**< \deprecated Old name for ::CVIE_MAX_LENGTH */
#define PARAMETER_DESCRIPTION 1       /**< \deprecated Old name for ::CVIE_PARAMETER_FILE_DESCRIPTION */
#define OPERATION_DESCRIPTION 2       /**< \deprecated Old name for ::CVIE_SETTING_DESCRIPTION */

#define RELATIVE_RESOLUTION_X 12      /**< \deprecated Old name for ::CVIE_API_RELATIVE_RESOLUTION_X */
#define RELATIVE_RESOLUTION_Y 13      /**< \deprecated Old name for ::CVIE_API_RELATIVE_RESOLUTION_Y */

#define DEPTH_DEPENDENCE_TRANSITION_START 23    /**< \deprecated Old name for ::CVIE_API_DEPTH_DEPENDENCE_TRANSITION_START */
#define DEPTH_DEPENDENCE_TRANSITION_END 24      /**< \deprecated Old name for ::CVIE_API_DEPTH_DEPENDENCE_TRANSITION_END */



#endif  // CVIE_NO_DEPRECATED_NAMES
/**\} */


#endif /* __CVIE_HEADER */
