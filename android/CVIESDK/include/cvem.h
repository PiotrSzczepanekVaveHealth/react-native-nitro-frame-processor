//=============================================================================
///
/// @file cvem.h
///
/// @brief Error management used by CVIE and CVLM.
///
/// Copyright (c) 2023-2024 ContextVision AB.
///
//=============================================================================
#ifndef CVEM_HEADER_INCLUDED
#define CVEM_HEADER_INCLUDED
#pragma once

/** \defgroup ErrorManagement Error Management
 * \brief Retrieving information about CVIE and CVLM errors.
 *
 * CONFIDENTIAL
 * \copyright Copyright (c) ContextVision AB.
 * \{
 */

/** \cond */

#ifdef _WIN32
/*  __cdecl is default when creating DLL but not necessarily when using it... */
#   define CVIE_DECL __cdecl
#   ifdef CVIE_EXPORTS
#       define CVIE_API_SPEC __declspec(dllexport)
#   else
#       define CVIE_API_SPEC __declspec(dllimport)
#   endif
#else
#   define CVIE_DECL
#   define CVIE_API_SPEC __attribute__((__visibility__("default")))
#endif

#ifdef __cplusplus
#   define CVIE_API extern "C" CVIE_API_SPEC
#   define CVIE_API_DEPRECATED(reason) extern "C" [[deprecated(reason)]] CVIE_API_SPEC
#else
#   define CVIE_API CVIE_API_SPEC
#   define CVIE_API_DEPRECATED(reason) CVIE_API_SPEC
#endif


/** \endcond */

/** Error type */
typedef int ECVIE;

/** Instance handle type */
typedef void* HCVIE;

/**
 * \defgroup CVIE_Results Result codes from API calls
 * These values are returned by the API calls.
\{*/
#define CVIE_E_OK 0                     /**< The call succeeded. */
#define CVIE_E_NOT_OK 1                 /**< See documentation of function for details. */
#define CVIE_E_CREATE_HANDLE 2          /**< Failed to create instance handle. */
#define CVIE_E_BAD_HANDLE 3             /**< The supplied instance handle is invalid. */
#define CVIE_E_USER_ABORTED 4           /**< The operation was aborted by the user (by returning 0 from a progress callback function). */
#define CVIE_E_FATAL_ERROR  5           /**< \anchor CVIE_E_FATAL_ERROR A fatal error has occurred (e.g. out of memory). The library state is undefined and no further calls should be made. */
#define CVIE_E_FILEIO_ERROR 6           /**< \anchor CVIE_E_FILEIO_ERROR The parameter file could not be opened or read, or other file/directory related error. */
#define CVIE_E_ILLEGAL_COMMAND 7        /**< The prerequisites for calling this function has not been met. */
#define CVIE_E_INVALID_INPUT 8          /**< One or more parameters are invalid or out of range. */
#define CVIE_E_INVALID_PARAMETER_FILE 9 /**< The supplied parameter file is invalid or corrupted. */
#define CVIE_E_LICENSE_ERROR 10         /**< There is no valid license for the requested operation. */
#define CVIE_E_NOT_SUPPORTED 11         /**< The supplied parameters are not supported. */
#define CVIE_E_SETTING_NOT_READY 12     /**< The requested setting has not been set up.  Call CVIEEnhanceSetup() first. */
#define CVIE_E_UNKNOWN_ERROR 15         /**< An unknown error has occurred. */
#define CVIE_E_OUT_OF_MEMORY 16         /**< There was not enough heap memory. */
/** \} */

/**
 * The CVEMGetLastError function is used for retrieving error information after a call to any of the CVIE
 * or CVLM library functions.
 *
 * \param [in] handle Specifies a handle to a CVIE instance from a ::CVIECreate call.
 * \param [out] errorString A pointer to a string where to store the error message.
 * \param [in] length Specifies the length of the string. A string length of CVEM_MAX_LENGTH characters is sufficient.
 *
 * \note If the CVEMGetLastError function is called to get information about a call to a license function,
 * \ref CVLM_ERROR_HANDLE should be used as handle.
 *
 * \return The function returns the errorString buffer.
 */
CVIE_API const char* CVIE_DECL CVEMGetLastError(HCVIE handle, char* errorString, int length);
/** Function pointer type for CVEMGetLastError() */
typedef const char* (CVIE_DECL* TCVEMGetLastError)(HCVIE handle, char* errorString, int length);

#define CVEM_MAX_LENGTH 512             /**< Maximum length of error messages */

/**\} ErrorManagement */

/**
 * \defgroup DeprecatedDefines Deprecated define names
 * These defines are deprecated and should not be used.
 * To make sure that the application does not use deprecated names
 * \#define CVIE_NO_DEPRECATED_NAMES before including any CVIE header file.
\{*/
#ifndef CVIE_NO_DEPRECATED_NAMES

#define MAX_ERROR_MESSAGE_LENGTH 512    /**< \deprecated Old name for CVEM_MAX_LENGTH */

#endif
/**\} */

#endif /* Include guard */
