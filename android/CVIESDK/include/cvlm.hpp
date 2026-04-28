//=============================================================================
///
/// @file cvlm.hpp
///
/// @brief CVLM C++ Licensing API header
///
/// Copyright (c) 2023-2024 ContextVision AB.
///
//=============================================================================
#ifndef CVLM_HPP_HEADER_INCLUDED
#define CVLM_HPP_HEADER_INCLUDED
#pragma once

#include "dynamic_library.hpp"

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace cvie {

class Library;
CVIEDynamicLibrary& getCVIEDynamicLibrary(cvie::Library& lib);
const CVIEDynamicLibrary& getCVIEDynamicLibrary(const cvie::Library& lib);

}

namespace cvlm {


/// \defgroup CVLMCpp CVLM C++ API Reference
///
/// \brief C++ wrapper functions for the CVLM API.
///
/// This group of functions constitutes a C++ API that provides C++ friendly types and overloads that hide
/// the index system used by the CVLM C API.
/// 
/// All functions are available with or without a library parameter. The library parameter can be a cvie::CVIEDynamicLibrary& or a
/// cvie::Library which is used to access the CVIE dynamic library functions. Without library parameter the functions
/// use the default cvie::CVIEDynamicLibrary (see cvie::CVIEDynamicLibrary::getDefault()). This is the same as the default
/// cvie::Library object.
///
/// \note If a product is activated using one library that license is not immediately available in another library as the license
/// information is loaded when the library is first used only. To update license information in another library call cvlm::reinitialize for it.
/// 
/// \note If the different CVIE shared library files are located in different directories they will find the .cov files located in
/// their directories. Thus the lists of available product IDs and License Methods may vary between the libraries.
///
/// These functions throw \c std::runtime_error or \c std::logic_error if an error occurs.
///
/// CONFIDENTIAL
/// \copyright Copyright (c) ContextVision AB.
/// \{

/// Contains information about a combination of Product ID and License Method
struct LicenseInfo {
    LicenseInfo(const std::string& prodId, const std::string& licMethod, const std::string& key, const std::string& expiry)
        : productId(prodId), licenseMethod(licMethod), key(key), expiry(expiry) {}
    std::string productId;      ///< The Product ID.
    std::string licenseMethod;  ///< The License Method.
    std::string key;            ///< The activation key. Empty string if none.
    std::string expiry;         ///< The expiration of the key in the format yyyy-MM-dd. Empty string if no key set. 'Perpetual' for perpetual licenses.
};

/// @{
/// Checks if the the specified Product ID is licensed. If the function returns false, additional information
/// can be retrieved by calling cvlm::getLastError().
inline bool isLicensed(const cvie::CVIEDynamicLibrary& library, const std::string& productId);
inline bool isLicensed(const cvie::Library& library, const std::string& productId) { return isLicensed(cvie::getCVIEDynamicLibrary(library), productId); }
inline bool isLicensed(const std::string& productId) { return isLicensed(cvie::CVIEDynamicLibrary::getDefault(), productId); }
/// @}

/// @{
/// Checks if the the specified Product ID is licensed. If the productIdOrIndex parameter is lower
/// than 16 it is interpreted as an index into the list returned by getProductIds(), otherwise it
/// is interpreted as a numeric Product ID. If the function returns false, additional
/// information can be retrieved by calling cvlm::getLastError().
inline bool isLicensed(const cvie::CVIEDynamicLibrary& library, int productIdOrIndex);
inline bool isLicensed(const cvie::Library& library, int productIdOrIndex) { return isLicensed(cvie::getCVIEDynamicLibrary(library), productIdOrIndex); }
inline bool isLicensed(int productIdOrIndex) { return isLicensed(cvie::CVIEDynamicLibrary::getDefault(), productIdOrIndex); }
/// @}

/// @{
/// Returns a list of all Product IDs currently available.
inline std::vector<std::string> getProductIds(const cvie::CVIEDynamicLibrary& library);
inline std::vector<std::string> getProductIds(const cvie::Library& library) { return getProductIds(cvie::getCVIEDynamicLibrary(library)); }
inline std::vector<std::string> getProductIds() { return getProductIds(cvie::CVIEDynamicLibrary::getDefault()); }
/// @}

/// @{
/// Returns a list of all available License Methods.
inline std::vector<std::string> getLicenseMethods(const cvie::CVIEDynamicLibrary& library);
inline std::vector<std::string> getLicenseMethods(const cvie::Library& library) { return getLicenseMethods(cvie::getCVIEDynamicLibrary(library)); }
inline std::vector<std::string> getLicenseMethods() { return getLicenseMethods(cvie::CVIEDynamicLibrary::getDefault()); }
/// @}

/// @{
/// The getLicenseInfo function is used to identify which Product IDs are licensed.
inline std::vector<cvlm::LicenseInfo> getLicenseInfo(const cvie::CVIEDynamicLibrary& library);
inline std::vector<cvlm::LicenseInfo> getLicenseInfo(const cvie::Library& library) { return getLicenseInfo(cvie::getCVIEDynamicLibrary(library)); }
inline std::vector<cvlm::LicenseInfo> getLicenseInfo() { return getLicenseInfo(cvie::CVIEDynamicLibrary::getDefault()); }
/// @}

/// @{
/// Sets the activation key. Automatically determines the Product ID using the Activation Key.
inline void activateProduct(cvie::CVIEDynamicLibrary& library, const std::string& key);
inline void activateProduct(cvie::Library& library, const std::string& key) { activateProduct(cvie::getCVIEDynamicLibrary(library), key); }
inline void activateProduct(const std::string& key) { return activateProduct(cvie::CVIEDynamicLibrary::getDefault(), key); }
/// @}

/// @{
/// Sets the activation key for the specified Product ID.
inline void activateProduct(cvie::CVIEDynamicLibrary& library, const std::string& productId, const std::string& key);
inline void activateProduct(cvie::Library& library, const std::string& productId, const std::string& key) { activateProduct(cvie::getCVIEDynamicLibrary(library), productId, key); }
inline void activateProduct(const std::string& productId, const std::string& key) { return activateProduct(cvie::CVIEDynamicLibrary::getDefault(), productId, key); }
/// @}

/// @{
/// Sets the activation key for the specified Product ID. If the productIdOrIndex parameter is
/// lower than 16 it is interpreted as an index into the list returned by getProductIds(),
/// otherwise it is interpreted as a numeric Product ID.
inline void activateProduct(cvie::CVIEDynamicLibrary& library, int productIdOrIndex, const std::string& key);
inline void activateProduct(cvie::Library& library, int productIdOrIndex, const std::string& key) { activateProduct(cvie::getCVIEDynamicLibrary(library), productIdOrIndex, key); }
inline void activateProduct(int productIdOrIndex, const std::string& key) { return activateProduct(cvie::CVIEDynamicLibrary::getDefault(), productIdOrIndex, key); }
/// @}

/// @{
/// Removes the Product ID activation key associated with the specified Product ID. If there are multiple
/// licenses associated with the same Product ID, the license method must be specified explicitly using
/// another overload.
inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, const std::string& productId);
inline void deactivateProduct(cvie::Library& library, const std::string& productId) { deactivateProduct(cvie::getCVIEDynamicLibrary(library), productId); }
inline void deactivateProduct(const std::string& productId) { return deactivateProduct(cvie::CVIEDynamicLibrary::getDefault(), productId); }
/// @}

/// @{
/// Removes the Product ID activation key associated with the specified Product ID and License Method.
inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, const std::string& productId, const std::string& licenseMethod);
inline void deactivateProduct(cvie::Library& library, const std::string& productId, const std::string& licenseMethod) { deactivateProduct(cvie::getCVIEDynamicLibrary(library), productId, licenseMethod); }
inline void deactivateProduct(const std::string& productId, const std::string& licenseMethod) { return deactivateProduct(cvie::CVIEDynamicLibrary::getDefault(), productId, licenseMethod); }
/// @}

/// @{
/// Removes the Product ID activation key associated with the specified Product ID.
/// If the productIdOrIndex parameter is lower than 16 it is interpreted as an index into the
/// list returned by getProductIds(), otherwise it is interpreted as a numeric Product ID.
/// If there are multiple licenses associated with the same Product ID, the license method must
/// be specified explicitly using another overload.
inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, int productIdOrIndex);
inline void deactivateProduct(cvie::Library& library, int productIdOrIndex) { deactivateProduct(cvie::getCVIEDynamicLibrary(library), productIdOrIndex); }
inline void deactivateProduct(int productIdOrIndex) { return deactivateProduct(cvie::CVIEDynamicLibrary::getDefault(), productIdOrIndex); }
/// @}

/// @{
/// Removes the Product ID activation key associated with the specified Product ID and License Method.
/// If the productIdOrIndex parameter is lower than 16 it is interpreted as an index into the
/// list returned by getProductIds(), otherwise it is interpreted as a numeric Product ID.
inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, int productIdOrIndex, const std::string& licenseMethod);
inline void deactivateProduct(cvie::Library& library, int productIdOrIndex, const std::string& licenseMethod) { deactivateProduct(cvie::getCVIEDynamicLibrary(library), productIdOrIndex, licenseMethod); }
inline void deactivateProduct(int productIdOrIndex, const std::string& licenseMethod) { return deactivateProduct(cvie::CVIEDynamicLibrary::getDefault(), productIdOrIndex, licenseMethod); }
/// @}

/// @{
/// Removes the Product ID activation key associated with the specified Product ID and License Method.
/// The License Method is specified as an index into the list returned by getLicenseMethods().
inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, const std::string& productId, int licenseMethodIndex);
inline void deactivateProduct(cvie::Library& library, const std::string& productId, int licenseMethodIndex) { deactivateProduct(cvie::getCVIEDynamicLibrary(library), productId, licenseMethodIndex); }
inline void deactivateProduct(const std::string& productId, int licenseMethodIndex) { return deactivateProduct(cvie::CVIEDynamicLibrary::getDefault(), productId, licenseMethodIndex); }
/// @}

/// @{
/// Removes the Product ID activation key associated with the specified Product ID and License Method.
/// If the productIdOrIndex parameter is lower than 16 it is interpreted as an index into the
/// list returned by getProductIds(), otherwise it is interpreted as a numeric Product ID.
/// The License Method is specified as an index into the list returned by getLicenseMethods().
inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, int productIdOrIndex, int licenseMethodIndex);
inline void deactivateProduct(cvie::Library& library, int productIdOrIndex, int licenseMethodIndex) { deactivateProduct(cvie::getCVIEDynamicLibrary(library), productIdOrIndex, licenseMethodIndex); }
inline void deactivateProduct(int productIdOrIndex, int licenseMethodIndex) { return deactivateProduct(cvie::CVIEDynamicLibrary::getDefault(), productIdOrIndex, licenseMethodIndex); }
/// @}

/// @{
/// Get the Device ID for a specific License Method specified by a string.
inline std::uint32_t getDeviceId(const cvie::CVIEDynamicLibrary& library, const std::string& licenseMethod);
inline std::uint32_t getDeviceId(const cvie::Library& library, const std::string& licenseMethod) { return getDeviceId(cvie::getCVIEDynamicLibrary(library), licenseMethod); }
inline std::uint32_t getDeviceId(const std::string& licenseMethod) { return getDeviceId(cvie::CVIEDynamicLibrary::getDefault(), licenseMethod); }
/// @}

/// @{
/// Get the Device ID for a specific License Method specified as an index into the list returned by getLicenseMethods().
inline std::uint32_t getDeviceId(const cvie::CVIEDynamicLibrary& library, int licenseMethodIndex);
inline std::uint32_t getDeviceId(const cvie::Library& library, int licenseMethodIndex) { return getDeviceId(cvie::getCVIEDynamicLibrary(library), licenseMethodIndex); }
inline std::uint32_t getDeviceId(int licenseMethodIndex) { return getDeviceId(cvie::CVIEDynamicLibrary::getDefault(), licenseMethodIndex); }
/// @}

/// @{
/// Get the Device ID as a string in hexadecimal representation for a License Method specified as a string.
inline std::string getDeviceIdString(const cvie::CVIEDynamicLibrary& library, const std::string& licenseMethod);
inline std::string getDeviceIdString(const cvie::Library& library, const std::string& licenseMethod) { return getDeviceIdString(cvie::getCVIEDynamicLibrary(library), licenseMethod); }
inline std::string getDeviceIdString(const std::string& licenseMethod) { return getDeviceIdString(cvie::CVIEDynamicLibrary::getDefault(), licenseMethod); }
/// @}

/// @{
/// Get the Device ID as a string in hexadecimal representation for a License Method specified as
/// an index into the list returned by getLicenseMethods().
inline std::string getDeviceIdString(const cvie::CVIEDynamicLibrary& library, int licenseMethodIndex);
inline std::string getDeviceIdString(const cvie::Library& library, int licenseMethodIndex) { return getDeviceIdString(cvie::getCVIEDynamicLibrary(library), licenseMethodIndex); }
inline std::string getDeviceIdString(int licenseMethodIndex) { return getDeviceIdString(cvie::CVIEDynamicLibrary::getDefault(), licenseMethodIndex); }
/// @}

/// @{
/// Set the value of the specified parameter. See \ref CVLM_SetPar for available parameters.
inline void setParameter(cvie::CVIEDynamicLibrary& library, int parameter, const std::string& value);
inline void setParameter(cvie::Library& library, int parameter, const std::string& value) { return setParameter(cvie::getCVIEDynamicLibrary(library), parameter, value); }
inline void setParameter(int parameter, const std::string& value) { setParameter(cvie::CVIEDynamicLibrary::getDefault(), parameter, value); }

inline void setParameter(cvie::CVIEDynamicLibrary& library, int parameter, std::nullptr_t);
inline void setParameter(cvie::Library& library, int parameter, std::nullptr_t) { return setParameter(cvie::getCVIEDynamicLibrary(library), parameter, nullptr); }
inline void setParameter(int parameter, std::nullptr_t) { setParameter(cvie::CVIEDynamicLibrary::getDefault(), parameter, nullptr); }
/// @}

/// @{
/// Reinitializes CVLM. This can for example be useful to detect inserted dongles.
inline void reinitialize(cvie::CVIEDynamicLibrary& library) { setParameter(library, CVLM_INIT, nullptr); }
inline void reinitialize(cvie::Library& library) { setParameter(library, CVLM_INIT, nullptr); }
inline void reinitialize() { setParameter(cvie::CVIEDynamicLibrary::getDefault(), CVLM_INIT, nullptr); }
/// @}

/// @{
inline std::string getLastError(const cvie::CVIEDynamicLibrary& library) { return library.getLastError(CVLM_ERROR_HANDLE); }
inline std::string getLastError(const cvie::Library& library) { return cvie::getCVIEDynamicLibrary(library).getLastError(CVLM_ERROR_HANDLE); }
inline std::string getLastError() { return getLastError(cvie::CVIEDynamicLibrary::getDefault()); }
/// @}
/// \} CVLMCpp

/// \cond
// Helpers
namespace detail {
    inline void checkReturn(const cvie::CVIEDynamicLibrary& library, ECVIE err)
    {
        if (err != CVIE_E_OK)
            throw std::runtime_error("CVLM Error: " + getLastError(library));
    }

    inline int getIndex(const std::vector<std::string>& values, const std::string& val)
    {
        auto it = std::find(values.begin(), values.end(), val);
        if (it == values.end())
            throw std::logic_error("The value " + val + " does not exist in the list of possible values.");
        return int(std::distance(values.begin(), it));
    }

    inline std::string toProductIdString(int num)
    {
        std::string numStr = std::to_string(num);
        if (numStr.size() > 7)
            throw std::logic_error("Too many digits in Product ID.");
        return std::string(7 - numStr.size(), '0') + numStr;
    }

    inline int toProductIndex(const cvie::CVIEDynamicLibrary& library, int productIdOrIndex)
    {
        // 16 is the max Product ID *index*. Everything above this is assumed to be a Product ID given directly.
        if (productIdOrIndex >= 16) {
            auto prodIds = getProductIds(library);
            productIdOrIndex = detail::getIndex(prodIds, toProductIdString(productIdOrIndex));
        }
        return productIdOrIndex;
    }
}
/// \endcond


inline bool isLicensed(const cvie::CVIEDynamicLibrary& library, const std::string& productId)
{
    int productIdIndex = detail::getIndex(getProductIds(library), productId);
    ECVIE err = library.isLicensed(productIdIndex);
    if (err == CVIE_E_OK)
        return true;
    if (err == CVIE_E_LICENSE_ERROR)
        return false;

    throw std::runtime_error("Error when checking license: " + getLastError(library));
}

inline bool isLicensed(const cvie::CVIEDynamicLibrary& library, int productIdOrIndex)
{
    ECVIE err = library.isLicensed(detail::toProductIndex(library, productIdOrIndex));
    if (err == CVIE_E_OK)
        return true;
    if (err == CVIE_E_LICENSE_ERROR)
        return false;

    throw std::runtime_error("Error when checking license: " + getLastError(library));
}

inline std::vector<std::string> getProductIds(const cvie::CVIEDynamicLibrary& library)
{
    std::vector<std::string> productIds;
    detail::checkReturn(library, library.getProductIds(productIds));
    return productIds;
}

inline std::vector<std::string> getLicenseMethods(const cvie::CVIEDynamicLibrary& library)
{
    std::vector<std::string> productIds;
    detail::checkReturn(library, library.getLicenseMethods(productIds));
    return productIds;
}

inline std::vector<cvlm::LicenseInfo> getLicenseInfo(const cvie::CVIEDynamicLibrary& library)
{
    std::vector<cvlm::LicenseInfo> ret;

    int productIdIndices[17];
    int licenseMethodIndices[17];
    char info[1024];
    detail::checkReturn(library, library.getLicenseInfo(productIdIndices, licenseMethodIndices, info));

    auto productIds = getProductIds(library);
    auto licenseMethods = getLicenseMethods(library);

    char* pInfo = info; // Keep track of position in info
    for (int i = 0; productIdIndices[i] != -1; i++) {
        std::string thisInfo(pInfo);
        std::string key;
        std::string expiry;

        if (thisInfo.size() >= 5ULL + 16 + 2 + 9) { // "Key: <16 character key>  <9-10 char expiry>"
            key = thisInfo.substr(5, 16);
            expiry = thisInfo.substr(5ULL + 16 + 2);
        }

        ret.emplace_back(productIds[productIdIndices[i]], licenseMethods[licenseMethodIndices[i]], key, expiry);
        pInfo += strlen(pInfo) + 1;
    }

    return ret;
}

inline void activateProduct(cvie::CVIEDynamicLibrary& library, const std::string& key)
{
    detail::checkReturn(library, library.activateProduct(-1, key));
}

inline void activateProduct(cvie::CVIEDynamicLibrary& library, const std::string& productId, const std::string& key)
{
    int productIdIndex = detail::getIndex(getProductIds(library), productId);
    detail::checkReturn(library, library.activateProduct(productIdIndex, key));
}

inline void activateProduct(cvie::CVIEDynamicLibrary& library, int productIdOrIndex, const std::string& key)
{
    int productIdIndex = detail::toProductIndex(library, productIdOrIndex);
    detail::checkReturn(library, library.activateProduct(productIdIndex, key));
}

inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, const std::string& productId)
{
    int productIdIndex = detail::getIndex(getProductIds(library), productId);
    detail::checkReturn(library, library.deactivateProduct(productIdIndex, -1));
}

inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, const std::string& productId, const std::string& licenseMethod)
{
    int productIdIndex = detail::getIndex(getProductIds(library), productId);
    int licenseMethodIndex = detail::getIndex(getLicenseMethods(library), licenseMethod);
    detail::checkReturn(library, library.deactivateProduct(productIdIndex, licenseMethodIndex));
}

inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, int productIdOrIndex)
{
    int productIdIndex = detail::toProductIndex(library, productIdOrIndex);
    detail::checkReturn(library, library.deactivateProduct(productIdIndex, -1));
}

inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, int productIdOrIndex, const std::string& licenseMethod)
{
    int productIdIndex = detail::toProductIndex(library, productIdOrIndex);
    int licenseMethodIndex = detail::getIndex(getLicenseMethods(library), licenseMethod);
    detail::checkReturn(library, library.deactivateProduct(productIdIndex, licenseMethodIndex));
}

inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, const std::string& productId, int licenseMethodIndex)
{
    int productIdIndex = detail::getIndex(getProductIds(library), productId);
    detail::checkReturn(library, library.deactivateProduct(productIdIndex, licenseMethodIndex));
}

inline void deactivateProduct(cvie::CVIEDynamicLibrary& library, int productIdOrIndex, int licenseMethodIndex)
{
    int productIdIndex = detail::toProductIndex(library, productIdOrIndex);
    detail::checkReturn(library, library.deactivateProduct(productIdIndex, licenseMethodIndex));
}

inline std::uint32_t getDeviceId(const cvie::CVIEDynamicLibrary& library, const std::string& licenseMethod)
{
    int licenseMethodIndex = detail::getIndex(getLicenseMethods(library), licenseMethod);
    return getDeviceId(library, licenseMethodIndex);
}

inline std::uint32_t getDeviceId(const cvie::CVIEDynamicLibrary& library, int licenseMethodIndex)
{
    unsigned long deviceId;
    detail::checkReturn(library, library.getDeviceId(licenseMethodIndex, deviceId));
    return std::uint32_t(deviceId);
}

inline std::string getDeviceIdString(const cvie::CVIEDynamicLibrary& library, const std::string& licenseMethod)
{
    int licenseMethodIndex = detail::getIndex(getLicenseMethods(library), licenseMethod);
    return getDeviceIdString(library, licenseMethodIndex);
}

inline std::string getDeviceIdString(const cvie::CVIEDynamicLibrary& library, int licenseMethodIndex)
{
    uint32_t deviceId = getDeviceId(library, licenseMethodIndex);
    if (deviceId == 0)
        return "";      // No device ID available

    std::stringstream stream;
    stream.imbue(std::locale("C")); // Ensure that we don't get separators in the string
    stream << std::hex << std::setw(8) << std::setfill('0') << deviceId;
    return stream.str();
}

inline void setParameter(cvie::CVIEDynamicLibrary& library, int parameter, const std::string& value)
{
    detail::checkReturn(library, library.setParameterValue(parameter, value));
}

inline void setParameter(cvie::CVIEDynamicLibrary& library, int parameter, std::nullptr_t)
{
    detail::checkReturn(library, library.setParameterValue(parameter, nullptr));
}

} // namespace cvlm

#endif // Include guard
