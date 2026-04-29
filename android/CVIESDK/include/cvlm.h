//=============================================================================
///
/// @file cvlm.h
///
/// @brief CVLM C Licensing API header
///
/// Copyright (c) 2015-2024 ContextVision AB.
///
//=============================================================================
#ifndef CVLM_HEADER_INCLUDED
#define CVLM_HEADER_INCLUDED
#pragma once

/** \defgroup CVLM CVLM API Reference
 *
 * \brief Functions related to license management.
 *
 * This file contains functions to handle license management via API calls.
 * Additionally, C++ wrappers are available to provide a nicer API, see \ref CVLMCpp "CVLM C++ API".
 * The file also contains functionality to simplify explicit loading of the library.
 *
 * CONFIDENTIAL
 * \copyright Copyright (c) ContextVision AB.
 *
 \{
*/

#include "cvem.h"


/** Special HCVIE handle to use with CVEMGetLastError() when checking the error message from a Licensing function call. */
#define CVLM_ERROR_HANDLE ((HCVIE)43)


/** \defgroup CVLM_SetPar CVLM Parameters
 *  These are the possible parameters for \ref cvlm::setParameter and \ref CVLMSetParameterValue().
 \{
 */
#define CVLM_INIT  1 /**<  Set to NULL, or a pointer to initialization string (16 - 255 characters). */

/**\} CVLM_SetPar */

/**
 * The CVLMIsLicensed function checks if a valid activation key is installed for a Product ID given its
 * relative index.
 *
 * \param [in] productIdIndex  Specifies the index of the Product ID to be checked, starting with 0.
 *                             The indices of the Product IDs are retrieved from the \ref CVLMGetProductIds function.
 *
 * \return If the function succeeds and the license for the checked Product ID is valid, the returned value is
 *         ::CVIE_E_OK, else the returned value is an error code.  Extended error information may be
 *         available by calling \ref CVLMGetLastError().
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
 *     // Check license for Product ID with index 0
 *     cvie_err = CVLMIsLicensed(0);
 *
 *     // Check returned value and act accordingly
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "License OK!");
 *     else if (cvie_err == CVIE_E_LICENSE_ERROR)
 *         fprintf(stderr, "No license! %s", CVLMGetLastError(errstr, sizeof(errstr)));
 *     else if (cvie_err == CVIE_E_INVALID_INPUT)
 *         fprintf(stderr, "Invalid input! %s", CVLMGetLastError(errstr, sizeof(errstr)));
 *     else
 *         fprintf(stderr, "Unknown error!");
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVLMIsLicensed(int productIdIndex);
/** Function pointer type for CVLMIsLicensed() */
typedef ECVIE(CVIE_DECL* TCVLMIsLicensed)(int productIdIndex);


/**
 * The \ref CVLMGetDeviceId function retrieves the Device ID of the specified License Method given by its
 * relative index.
 *
 * \note The value returned is an unsigned long. When used in ContextVision Customer Portal this value must
 * be presented as a 8 character hexidecimal number. Note below how the number is printed with %08x to get a
 * compatible output.
 *
 * \param [in] licenseMethodIndex Specifies the index of the license method for which Device ID is to be retrieved.
 *                                A list of available license method indices can be retrieved by calling \ref
 *                                CVLMGetLicenseMethods.
 * \param [out] deviceId A pointer to an unsigned long where to store the Device ID.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error
 *         code. Extended error information may be available by calling \ref CVLMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     unsigned long deviceId;
 *     char errstr[256];
 *
 *     // Get Device ID for license method index 0
 *     cvie_err = CVLMGetDeviceId(0, &deviceId);
 *     // Check returned value and act accordingly
 *
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "Device ID: %08x\n", deviceId);
 *     else if (cvie_err == CVIE_E_INVALID_INPUT)
 *         fprintf(stderr, "Invalid input! %s", CVLMGetLastError(errstr, sizeof(errstr)));
 *     else
 *         fprintf(stderr, "Unknown error!");
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVLMGetDeviceId(int licenseMethodIndex, unsigned long* deviceId);
/** Function pointer type for CVLMGetDeviceId() */
typedef ECVIE(CVIE_DECL* TCVLMGetDeviceId)(int licenseMethodIndex, unsigned long* deviceId);

/**
 * The CVLMGetLicenseMethods function is used to explore which license methods are available in the CVIE
 * library. The best way to explore the possibilities is to use the cvietest64 application provided with the
 * CVIE SDK. The indices may change when the CVIE library is updated to a new version or a new .cov file is
 * installed.
 *
 * \param [out] licenseMethods A pointer to an array of char pointers.
 *                             The function sets the referenced pointer to a list of possible license methods.
 *                             The position of the license method name in the list indicates the license
 *                             method indices. The list is terminated by an entry which is the empty string.
 *                             Note: Not a NULL pointer.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error
 *         code. Extended error information may be available by calling \ref CVLMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *
 *     ECVIE err;
 *     const char **licenseMethodNames;
 *
 *     // Get possible license methods
 *     err = CVLMGetLicenseMethods(&licenseMethodNames);
 *
 *     // Check returned value and act accordingly
 *     if (err == CVIE_E_OK) {
 *         fprintf(stderr, "\n\nAvailable License Methods:\n");
 *
 *         for (int i=0; licenseMethodNames[i][0] != '\0'; i++)
 *             fprintf(stderr, "  Index: %d  Name: %s\n", i, licenseMethodNames[i]);

 *         fprintf(stderr, "\n\n");
 *     }
 *     else
 *         fprintf(stderr, "Unknown error!");
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVLMGetLicenseMethods(const char*** licenseMethods);
/** Function pointer type for CVLMGetLicenseMethods() */
typedef ECVIE(CVIE_DECL* TCVLMGetLicenseMethods)(const char*** licenseMethods);

/**
 * The CVLMGetProductIds function is used to explore which Product IDs are useful for the CVIE dynamic
 * library with the current .cov file(s). The best way to explore the possibilities is to use the cvietest64
 * application provided with the CVIE SDK. The indices may change when the library is updated or the .cov file is changed.
 *
 * \param [out] productIds  A pointer to an array of char pointers.
 *                          The function sets the referenced pointer to a list of possible Product IDs
 *                          represented as strings. The position of in the list indicates the Product ID indices.
 *                          The list is terminated by an entry which is the empty string. Note: Not a NULL
 *                          pointer.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error
 *         code. Extended error information may be available by calling \ref CVLMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     const char **productIds;
 *     char errstr[256];
 *
 *     // Get valid Product IDs
 *     cvie_err = CVLMGetProductIds(&productIds);
 *
 *     // Check returned value and act accordingly
 *     if (cvie_err == CVIE_E_OK) {
 *         fprintf(stderr, "\n\nValid Product IDs:\n");
 *
 *         for (int i=0; productIds[i][0] != '\0'; i++)
 *             fprintf(stderr, "  Index: %d  Name: %s\n", i, productIds[i]);
 *
 *         fprintf(stderr, "\n\n");
 *     }
 *     else
 *         fprintf(stderr, "Unknown error!");
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVLMGetProductIds(const char*** productIds);
/** Function pointer type for CVLMGetProductIds() */
typedef ECVIE(CVIE_DECL* TCVLMGetProductIds)(const char*** productIds);

/**
 * The CVLMGetLicenseInfo is used to identify which Product IDs are licensed on the device.
 * Information about licensed Product IDs, such as the activation key and any expiry date is also retrieved.
 *
 * \param [out] productIdIndices A pointer to an integer array.
 *                          The function sets the elements in the referenced array according to the
 *                          indices of the licensed Product IDs. Further information about the meaning of the
 *                          indices may be obtained by calling \ref CVLMGetProductIds.  The last element
 *                          of the array is set to -1 indicating a termination of the index list. The array
 *                          is never filled with more than 16 + 1 values.
 * \param [out] licenseMethodIndices A pointer to an integer array.
 *                          The function sets the elements in the referenced array according to the
 *                          indices of the license methods for each licensed Product ID.
 *                          The last element of the array is set to -1, indicating a termination
 *                          of the indices list. The array is never filled with more than 16 + 1 values.
 * \param [out] info A pointer to a character buffer.
 *                   The function sets the characters in the buffer according to the
 *                   information available about the licensed Product IDs. For each Product ID a
 *                   null terminating string is created. The strings immediately follow
 *                   each other in an order corresponding to the productIdIndices array.
 *                   The last string is followed by an extra terminating NUL character.
 *                   The size of the buffer must be at least 1024 characters.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK,
 *         else the returned value is an error code. Extended error information
 *         may be available by calling \ref CVLMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *
 *     ECVIE cvie_err;
 *     int productIdIndices[17];
 *     int licenseMethodIndices[17];
 *     char info[1024];
 *     char *pInfo;
 *
 *     // Get licensed Product IDs
 *     cvie_err = CVLMGetLicenseInfo(productIdIndices, licenseMethodIndices, info);
 *
 *     // Check returned value and act accordingly
 *     if (cvie_err == CVIE_E_OK) {
 *         pInfo = info;
 *         fprintf(stderr, "\n\productIdIndex:  licenseMethodIndex:  Info:\n");
 *
 *         for (int i=0; productIdIndices[i] != -1; i++) {
 *             fprintf(stderr,"%12d  %10d  %s\n", productIdIndices[i], licenseMethodIndices[i], pInfo);
 *             pInfo += strlen(pInfo) + 1;
 *         }
 *
 *         fprintf(stderr, "\n\n");
 *     }
 *     else if (cvie_err == CVIE_E_LICENSE_ERROR)
 *         fprintf(stderr, "Error: No license!");
 *     else
 *         fprintf(stderr, "Unknown error!");
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVLMGetLicenseInfo(int* productIdIndices, int* licenseMethodIndices, char* info);
/** Function pointer type for CVLMGetLicenseInfo() */
typedef ECVIE(CVIE_DECL* TCVLMGetLicenseInfo)(int* productIdIndices, int* licenseMethodIndices, char* info);

/**
 * The CVLMActivateProduct function licenses a given Product ID. The license method to use is embedded in the
 * activation key provided by ContextVision.
 *
 * \param [in] productIdIndex Specifies the Product ID index for which license key is to be set.
 *             The indices of the Product IDs are retrieved from the \ref CVLMGetProductIds function.
 *             If -1 is passed, the Product ID is fetched from the key if possible.
 * \param [in] key A pointer to a string containing the key.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error
 *         code. Extended error information may be available by calling \ref CVLMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *
 *     // Set the activation key for Product ID with index 0
 *     cvie_err = CVLMActivateProduct(0, "1234567890123456");
 *
 *     // Check returned value and act accordingly
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "License set!");
 *     else if (cvie_err == CVIE_E_LICENSE_ERROR)
 *         fprintf(stderr, "License error! %s", CVLMGetLastError(errstr, sizeof(errstr)));
 *     else if (cvie_err == CVIE_E_INVALID_INPUT)
 *         fprintf(stderr, "Invalid input! %s", CVLMGetLastError(errstr, sizeof(errstr)));
 *     else
 *         fprintf(stderr, "Unknown error!");
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVLMActivateProduct(int productIdIndex, const char* key);
/** Function pointer type for CVLMActivateProduct() */
typedef ECVIE(CVIE_DECL* TCVLMActivateProduct)(int productIdIndex, const char* key);

/**
 * The CVLMSetParameterValue function is used to perform a license check prior to ::CVIECreate. If the
 * license check has not been initialized when CVIECreate is called, CVIECreate will initialize the
 * license check automatically, but only
 * the first time of each program run. It is advised to do the license check early as it may sometimes take a few
 * seconds. This call can also be used to re-check that a dongle is inserted if an operation fails due to the
 * dongle being temporarily removed or not initially inserted. It is advised not to perform the recheck
 * unless a license error has occurred due to the above mentioned potential delay.
 *
 * \param [in] parameter Specifies which parameter to set. See \ref CVLM_SetPar.
 * \param [in] value     Pointer to parameter data.  The parameter type depends on the parameter.
 *
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error
 *         code. Extended error information may be available by calling \ref CVLMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *
 *     // Initialize license
 *     cvie_err = CVLMSetParameterValue(CVLM_INIT, NULL);
 *
 *     // Check returned value and act accordingly
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "License initialized!");
 *
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVLMSetParameterValue(int parameter, const void* value);
/** Function pointer type for CVLMSetParameterValue(). The handle parameter is not used (use NULL as value) */
typedef ECVIE(CVIE_DECL* TCVLMSetParameterValue)(int parameter, const void* value);


/**
 * The CVLMDeactivateProduct function is used to remove Product ID activation keys. Theoretically the same
 * Product ID can be simultaneously licensed by more than one license method. This function only removes _one_
 * activation key. After uninstalling an activation key, the Product ID is no longer licensed with this
 * license method and will not appear when calling \ref CVLMGetRegisteredModules unless it was simultaneously
 * licensed by another method.
 *
 * \param [in] productIdIndex Specifies the index of the Product ID activation to be removed.
 *             The indices of the Product IDs are retrieved from the \ref CVLMGetProductIds function.
 * \param [in] licenseMethodIndex Specifies the license method index of the activation to be removed.
 *             A list of available license method indices can be retrieved by calling \ref CVLMGetLicenseMethods.
 *             If -1 is passed, the License Method is automatically detected as long as the Product ID is not
 *             licensed using multiple License Methods.
 * \return If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error
 *         code. Extended error information may be available by calling \ref CVLMGetLastError().
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ...
 *     ECVIE cvie_err;
 *     char errstr[256];
 *
 *     // Uninstall license for Product ID with index 0 and license method index 0
 *     cvie_err = CVLMDeactivateProduct(0, 0);
 *
 *     // Check returned value and act accordingly
 *     if (cvie_err == CVIE_E_OK)
 *         fprintf(stderr, "License removed!");
 *     else if (cvie_err == CVIE_E_LICENSE_ERROR)
 *         fprintf(stderr, "License error! %s", CVLMGetLastError(errstr, sizeof(errstr)));
 *     else if (cvie_err == CVIE_E_INVALID_INPUT)
 *         fprintf(stderr, "Invalid input! %s", CVLMGetLastError(errstr, sizeof(errstr)));
 *     else
 *         fprintf(stderr, "Unknown error!");
 *     ...
 * }
 * \endcode
 */
CVIE_API ECVIE CVIE_DECL CVLMDeactivateProduct(int productIdIndex, int licenseMethodIndex);
/** Function pointer type for CVLMUninstall() */
typedef ECVIE(CVIE_DECL* TCVLMDeactivateProduct)(int productIdIndex, int licenseMethodIndex);

/**
 * The CVLMGetLastError function is used for retrieving error information after a call to any of the CVLM library functions.
 *
 * \param [out] errorString A pointer to a string where to store the error message.
 * \param [in] length Specifies the length of the string. A string length of CVEM_MAX_LENGTH characters is sufficient.
 *
 * \note This function is a shorthand for CVEMGetLastError(CVLM_ERROR_HANDLE, errorString, length).
 *
 * \return The function returns the errorString buffer.
 */
static inline const char* CVLMGetLastError(char* errorString, int length) { return CVEMGetLastError(CVLM_ERROR_HANDLE, errorString, length); }

#ifndef CVIE_NO_DEPRECATED_NAMES

/** \deprecated Old name for CVLMIsLicensed(). The handle parameter is not used (use NULL as value) */
CVIE_API_DEPRECATED("Replaced by CVLMIsLicensed") ECVIE CVIE_DECL CVLMCheckLicense(HCVIE handle, int productIdIndex);
/** Function pointer type for CVLMCheckLicense() */
typedef ECVIE(CVIE_DECL* TCVLMCheckLicense)(HCVIE handle, int productIdIndex);

/** \deprecated Old name for CVLMGetDeviceId(). The handle parameter is not used (use NULL as value)*/
CVIE_API_DEPRECATED("Replaced by CVLMGetDeviceId") ECVIE CVIE_DECL CVLMGetHostId(HCVIE handle, int licenseMethodIndex, unsigned long* deviceId);
/** Function pointer type for CVLMGetHostId() */
typedef ECVIE(CVIE_DECL* TCVLMGetHostId)(HCVIE handle, int licenseMethodIndex, unsigned long* deviceId);

/** \deprecated Old name for CVLMGetLicenseMethods(). The handle parameter is not used (use NULL as value) */
CVIE_API_DEPRECATED("Replaced by CVLMGetLicenseMethods") ECVIE CVIE_DECL CVLMGetPossibleHostTypes(HCVIE handle, const char*** licenseMethods);
/** Function pointer type for CVLMGetPossibleHostTypes() */
typedef ECVIE(CVIE_DECL* TCVLMGetPossibleHostTypes)(HCVIE handle, const char*** licenseMethods);

/** \deprecated Old name for CVLMGetProductIds(). The handle parameter is not used (use NULL as value) */
CVIE_API_DEPRECATED("Replaced by CVLMGetProductIds") ECVIE CVIE_DECL CVLMGetPossibleModules(HCVIE handle, const char*** productIds);
/** Function pointer type for CVLMGetPossibleModules() */
typedef ECVIE(CVIE_DECL* TCVLMGetPossibleModules)(HCVIE handle, const char*** productIds);

/** \deprecated Old name for CVLMGetLicenseInfo(). The handle parameter is not used (use NULL as value) */
CVIE_API_DEPRECATED("Replaced by CVLMGetLicenseInfo") ECVIE CVIE_DECL CVLMGetRegisteredModules(HCVIE handle, int* productIdIndices, int* licenseMethodIndices, char* info);
/** Function pointer type for CVLMGetRegisteredModules() */
typedef ECVIE(CVIE_DECL* TCVLMGetRegisteredModules)(HCVIE handle, int* productIdIndices, int* licenseMethodIndices, char* info);

/** \deprecated Old name for CVLMActivateProduct(). The handle parameter is not used (use NULL as value) */
CVIE_API_DEPRECATED("Replaced by CVLMActivateProduct") ECVIE CVIE_DECL CVLMSetKey(HCVIE handle, int productIdIndex, const char* key);
/** Function pointer type for CVLMSetKey() */
typedef ECVIE(CVIE_DECL* TCVLMSetKey)(HCVIE handle, int productIdIndex, const char* key);

/** \deprecated Old name for CVLMDeactivateProduct(). The handle parameter is not used (use NULL as value) */
CVIE_API_DEPRECATED("Replaced by CVLMDeactivateProduct") ECVIE CVIE_DECL CVLMUninstall(HCVIE handle, int productIdIndex, int licenseMethodIndex);
/** Function pointer type for CVLMUninstall() */
typedef ECVIE(CVIE_DECL* TCVLMUninstall)(HCVIE handle, int productIdIndex, int licenseMethodIndex);

#endif

/**\} CVLM */

/**
 * \addtogroup DeprecatedDefines
 \{*/
#ifndef CVIE_NO_DEPRECATED_NAMES
#define RESERVED_LICERROR_HANDLE ((HCVIE)43)       /**< \deprecated Old name for ::CVLM_ERROR_HANDLE */
#define LM_INIT 1                                  /**< \deprecated Old name for ::CVLM_INIT */
#endif
/**\} */


#endif /* CVLM_HEADER_INCLUDED */
