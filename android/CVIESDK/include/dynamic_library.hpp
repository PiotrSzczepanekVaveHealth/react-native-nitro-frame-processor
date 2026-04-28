//=============================================================================
///
/// @file dynamic_library.hpp
///
/// @brief Functionality for explicitly loading CVIE dynamic library from C++.
///
/// Copyright (c) 2023-2024 ContextVision AB.
///
//=============================================================================
#ifndef DYNAMIC_LIBRARY_HPP_HEADER_INCLUDED
#define DYNAMIC_LIBRARY_HPP_HEADER_INCLUDED
#pragma once

#include "dynamic_library.h"

namespace cvie {

/// \defgroup CVIEDynamicLibrary CVIE Dynamic Library class
/// A CVIEDynamicLibrary is a C++ wrapper of a TCVIEDynamicLibrary struct with shallow copy semantics. It only does minimal type
/// conversions for functions handling strings.
/// \{

class CVIEDynamicLibrary {
public:
    CVIEDynamicLibrary() = default;
    /// @name: Constructors
    /// Construct a CVIEDynamicLibrary using the default fileanme or a specific filename.
    /// The forWorker parameter can be set to false to be able to dynamically load an older CVIE library than 7.0
    /// without error. However, any call to worker related methods may then cause an error.
    /// @{
    ///
#ifdef CVIE_LOAD_EXPLICIT
    /// Always loads the library explicitly. Use "" as the libraryName to load the library using its default name.
    /// \note This constructor is only available if CVIE_LOAD_EXPLICIT is defined by the build system, see \ref ExplicitLibraryLoading.
    explicit CVIEDynamicLibrary(const std::string& libraryName, bool forWorker = true);

#endif // CVIE_LOAD_EXPLICIT
    ///@}

    /// Check if the dynamic library was loaded and all its mandatory functions found.
    bool isOk() const { return m_impl != nullptr && m_impl->isOk(); }

    /// @{
    /// Forwarding methods which call the corresponding CVIE C functions in the loaded library.
    /// string values are accepted as std::string in addition to const char*. For functions involving filenames there are
    /// also std::wstring based overloads.
    ECVIE create(HCVIE& handle, int flags) const { checkOk(); return m_impl->Create(&handle, flags); }

    ECVIE destroy(HCVIE& handle) const { checkOk(); return m_impl->Destroy(&handle); }

    ECVIE createWorker(CVIEWorker& worker, HCVIE handle, const std::string& filename, int setting) const { checkOk(); checkOptional(m_impl->CreateWorkerVer); return m_impl->CreateWorkerVer(&worker, handle, filename.c_str(), setting, CVIE_WORKER_VERSION); }
    ECVIE createWorker(CVIEWorker& worker, HCVIE handle, const std::wstring& filename, int setting) const { checkOk(); checkOptional(m_impl->CreateWorkerVerW); return m_impl->CreateWorkerVerW(&worker, handle, filename.c_str(), setting, CVIE_WORKER_VERSION); }
    ECVIE createWorkerFromBuffer(CVIEWorker& worker, HCVIE handle, const char* buffer, int length, int setting) const { checkOk(); checkOptional(m_impl->CreateWorkerFromBufferVer); return m_impl->CreateWorkerFromBufferVer(&worker, handle, buffer, length, setting, CVIE_WORKER_VERSION); }
    ECVIE destroyWorker(CVIEWorker& worker) const { checkOk(); checkOptional(m_impl->DestroyWorker); return m_impl->DestroyWorker(&worker); }
    ECVIE saveWorkerParameterFile(CVIEWorker worker, const std::string& filename) const { checkOk(); checkOptional(m_impl->SaveWorkerParameterFile); return m_impl->SaveWorkerParameterFile(worker, filename.c_str()); }
    ECVIE saveWorkerParameterFile(CVIEWorker worker, const std::wstring& filename) const { checkOk(); checkOptional(m_impl->SaveWorkerParameterFileW); return m_impl->SaveWorkerParameterFileW(worker, filename.c_str()); }
    ECVIE setup(CVIEWorker worker) const { checkOk(); checkOptional(m_impl->Setup); return m_impl->Setup(worker); }
    ECVIE run(CVIEWorker worker) const { checkOk(); checkOptional(m_impl->Run); return m_impl->Run(worker); }
    ECVIE resetTemporalState(CVIEWorker worker) const { checkOk(); checkOptional(m_impl->ResetTemporalState); return m_impl->ResetTemporalState(worker); }

    ECVIE setThreads(HCVIE handle, int threads) const { checkOk(); return m_impl->SetThreads(handle, threads); }
    ECVIE releaseMemory(HCVIE handle) const { checkOk(); checkOptional(m_impl->ReleaseMemory); return m_impl->ReleaseMemory(handle); }
    ECVIE exit() const {
        if (isOk() && m_impl->Exit != nullptr)  // Don't try to do this if the library was never loaded ok or if it was too old to have a CVIEExit function.
            return m_impl->Exit();
        else
            return CVIE_E_OK;
    }
    ECVIE setParameterFile(HCVIE handle, const std::string& filename, int& settings) const { checkOk(); return m_impl->SetParameterFile(handle, filename.c_str(), &settings); }
    ECVIE setParameterFile(HCVIE handle, const std::wstring& filename, int& settings) const { checkOk(); checkOptional(m_impl->SetParameterFileW); return m_impl->SetParameterFileW(handle, filename.c_str(), &settings); }
    ECVIE setParameterBuffer(HCVIE handle, const char* buffer, int& settings) const { checkOk(); return m_impl->SetParameterBuffer(handle, buffer, &settings); }
    ECVIE saveParameterFile(HCVIE handle, const std::string& filename) const { checkOk(); return m_impl->SaveParameterFile(handle, filename.c_str()); }
    ECVIE saveParameterFile(HCVIE handle, const std::wstring& filename) const { checkOk(); checkOptional(m_impl->SaveParameterFileW); return m_impl->SaveParameterFileW(handle, filename.c_str()); }
    ECVIE enhanceSetup(HCVIE handle, int width, int height, int flags, int setting, const unsigned char* mask) const { checkOk(); return m_impl->EnhanceSetup(handle, width, height, flags, setting, mask); }
    ECVIE enhanceNext(HCVIE handle, const void* inImage, void* outImage, int setting) const { checkOk(); return m_impl->EnhanceNext(handle, inImage, outImage, setting); }

    ECVIE getParameterInfo(HCVIE handle, int setting, int parameter, int& type, int& length) const { checkOk(); return m_impl->GetParameterInfo(handle, setting, parameter, &type, &length); }
    ECVIE getDIInfo(HCVIE handle, int setting, int parameter, TCVIEDIInfo& info) const { checkOk(); checkOptional(m_impl->GetDIInfo); return m_impl->GetDIInfo(handle, setting, parameter, &info); }

    template<typename T> ECVIE getParameterValue(HCVIE handle, int setting, int parameter, T* value) const { throw std::logic_error("Use references when getting parameter values, not pointers."); }
    template<typename T> ECVIE getParameterValue(HCVIE handle, int setting, int parameter, T& value) const;

    ECVIE getParameterValue(HCVIE handle, int setting, int parameter, int& value) const;

    ECVIE getParameterValue(HCVIE handle, int setting, int parameter, float& value) const;

    ECVIE getParameterValue(HCVIE handle, int setting, int parameter, double& value) const;

    ECVIE getParameterValue(HCVIE handle, int setting, int parameter, std::string& value) const;
    ECVIE getParameterValue(HCVIE handle, int setting, int parameter, std::wstring& value) const;
    ECVIE getParameterValue(HCVIE handle, int setting, int parameter, char* value) const;
    ECVIE getParameterValue(HCVIE handle, int setting, int parameter, wchar_t* value) const;

    ECVIE getParameterValue(HCVIE handle, int setting, int parameter, void*& value) const;

    template<typename T> ECVIE setParameterValue(HCVIE handle, int setting, int parameter, T* value) const { throw std::logic_error("Use references when setting parameter values, not pointers."); }
    template<typename T> ECVIE setParameterValue(HCVIE handle, int setting, int parameter, const T& value) const;
    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, const int& value) const;

    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, const float& value) const;

    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, const double& value) const;

    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, const std::string& value) const { return setParameterValue(handle, setting, parameter, value.c_str()); }
    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, const std::wstring& value) const { return setParameterValue(handle, setting, parameter, value.c_str()); }
    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, const char* value) const;
    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, const wchar_t* value) const;

    // Make sure string version is used even if argument is not const char/wchar_t pointer
    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, char* value) const { return setParameterValue(handle, setting, parameter, const_cast<const char*>(value)); }
    ECVIE setParameterValue(HCVIE handle, int setting, int parameter, wchar_t* value) const { return setParameterValue(handle, setting, parameter, const_cast<const wchar_t*>(value)); }

    ///@{
    /// These functions are primarily intended to be be called through the \ref CVLMCpp "CVLM C++ API".
    ECVIE isLicensed(int productIdIndex) const { checkOk(); return m_impl->IsLicensed(productIdIndex); }
    ECVIE getDeviceId(int licenseMethodIndex, unsigned long& deviceId) const { checkOk(); return m_impl->GetDeviceId(licenseMethodIndex, &deviceId); }
    ECVIE getProductIds(std::vector<std::string>& productIdsVec) const { checkOk(); return getNameList(m_impl->GetProductIds, productIdsVec); }
    ECVIE getLicenseMethods(std::vector<std::string>& licenseMethodsVec) const { checkOk(); return getNameList(m_impl->GetLicenseMethods, licenseMethodsVec); }
    ECVIE activateProduct(int productIdIndex, const std::string& key) { checkOk(); return m_impl->ActivateProduct(productIdIndex, key.c_str()); }
    ECVIE deactivateProduct(int productIdIndex, int licenseMethodIndex) { checkOk(); return m_impl->DeactivateProduct(productIdIndex, licenseMethodIndex); }
    ECVIE setParameterValue(int parameter, const std::string& value) { checkOk(); return m_impl->CVLMSetParameterValue(parameter, value.c_str()); }
    ECVIE setParameterValue(int parameter, std::nullptr_t) { checkOk(); return m_impl->CVLMSetParameterValue(parameter, nullptr); }
    ECVIE getLicenseInfo(int* productIdIndices, int* licenseMethodIndices, char* info) const { checkOk(); return m_impl->GetLicenseInfo(productIdIndices, licenseMethodIndices, info); }
    ///@}

    std::string getLastError(HCVIE handle) const {
        char buffer[CVEM_MAX_LENGTH];
        return m_impl->GetLastError(handle, buffer, CVEM_MAX_LENGTH);
    }
    ///@}

    static CVIEDynamicLibrary& getDefault() {
        static CVIEDynamicLibrary library(0, true);
        return library;
    }

protected:
    void checkOk() const;

private:
    class Impl : public TCVIEDynamicLibrary {
    public:
#ifdef CVIE_LOAD_EXPLICIT
        Impl(const std::string& libraryName, bool forWorker) { CVIEOpenLibrary(this, libraryName.c_str(), forWorker); }

#endif // CVIE_LOAD_EXPLICIT

#ifdef CVIE_LOAD_IMPLICIT
        Impl(bool forWorker) { CVIELoadLibraryImplicit(this); }
#else
        Impl(bool forWorker) { CVIEOpenLibrary(this, "", forWorker); }
#endif
        ~Impl() { 
#ifdef CVIE_LOAD_EXPLICIT
            if (isOk())
                Exit();

            CVIECloseLibrary(this); 
#endif
        }

        bool isOk() const { ECVIE ec = errorCode;  return ec == CVIE_E_OK; }
    };

    CVIEDynamicLibrary(int dummy, bool forWorker);  // Must have two parameters to avoid it being selected by const char* literal filename strings.

    template<typename T> void checkOptional(T fun) const;

    static ECVIE getNameList(ECVIE(fun)(const char***), std::vector<std::string>& result) {
        const char** possible;
        result.clear();

        ECVIE ret = fun(&possible);
        if (ret != CVIE_E_OK)
            return ret;

        for (size_t i = 0; possible[i][0] != '\0'; ++i)
            result.emplace_back(possible[i]);
        
        return ret;
    }

    ECVIE checkParameterType(HCVIE handle, int setting, int parameter, int required, int* length = nullptr) const;

    std::shared_ptr<Impl> m_impl;
};


//////////////// CVIEDynamicLibrary methods ////////////////

inline CVIEDynamicLibrary::CVIEDynamicLibrary(int dummy, bool forWorker) : m_impl(std::make_unique<Impl>(forWorker))
{
}

#ifdef CVIE_LOAD_EXPLICIT

inline CVIEDynamicLibrary::CVIEDynamicLibrary(const std::string& libraryName, bool forWorker) : m_impl(std::make_unique<Impl>(libraryName, forWorker))
{
}


#endif // CVIE_LOAD_EXPLICIT

inline void CVIEDynamicLibrary::checkOk() const
{
    if (!isOk())
        throw std::logic_error("The dynamic library has not been loaded.");
}

template<typename T> void CVIEDynamicLibrary::checkOptional(T fun) const
{
    if (fun == nullptr)
        throw std::logic_error("The loaded dynamic library does not contain this function. It may be too old or lack GPU support.");
}


inline ECVIE CVIEDynamicLibrary::checkParameterType(HCVIE handle, int setting, int parameter, int required, int* length) const
{
    checkOk(); 
    int type;
    ECVIE ret = m_impl->GetParameterInfo(handle, setting, parameter, &type, length);
    if (ret != CVIE_E_OK)
        return ret;

    if (type != required)
        throw std::logic_error("Parameter type mismatch");

    return ret;
}


template<typename T> inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, T& value) const 
{
    // This is an incomplete test. But as we don't have individual constants for each struct type its the best we can do.
    ECVIE ret = checkParameterType(handle, setting, parameter, CVIE_PARTYPE_STRUCT);
    if (ret != CVIE_E_OK)
        return ret;
    
    return m_impl->GetParameterValue(handle, setting, parameter, &value);
}


inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, int& value) const
{
    ECVIE ret = checkParameterType(handle, setting, parameter, CVIE_PARTYPE_INT);
    if (ret != CVIE_E_OK)
        return ret;

    return m_impl->GetParameterValue(handle, setting, parameter, &value);
}


inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, float& value) const
{
    int type, length;
    ECVIE ret = getParameterInfo(handle, setting, parameter, type, length);
    if (ret != CVIE_E_OK)
        return ret;

    if (type == CVIE_PARTYPE_FLOAT)
        return m_impl->GetParameterValue(handle, setting, parameter, &value);

    if (type == CVIE_PARTYPE_DOUBLE) {
        double tmp;
        ret = m_impl->GetParameterValue(handle, setting, parameter, &tmp);
        if (ret == CVIE_E_OK)
            value = float(tmp);

        return ret;
    }

    throw std::logic_error("Parameter type mismatch");
}


inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, double& value) const
{
    int type, length;
    ECVIE ret = getParameterInfo(handle, setting, parameter, type, length);
    if (ret != CVIE_E_OK)
        return ret;

    if (type == CVIE_PARTYPE_DOUBLE)
        return m_impl->GetParameterValue(handle, setting, parameter, &value);

    if (type == CVIE_PARTYPE_FLOAT) {
        float tmp;
        ret = m_impl->GetParameterValue(handle, setting, parameter, &tmp);
        if (ret == CVIE_E_OK)
            value = double(tmp);

        return ret;
    }

    throw std::logic_error("Parameter type mismatch");
}


inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, char* value) const
{
    int type, size;
    ECVIE ret = getParameterInfo(handle, setting, parameter, type, size);
    if (ret != CVIE_E_OK)
        return ret;

    if (type != CVIE_PARTYPE_STRING && type != CVIE_PARTYPE_MASK)
        throw std::logic_error("Parameter type mismatch");

    return m_impl->GetParameterValue(handle, setting, parameter, value);
}


inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, wchar_t* value) const
{
    ECVIE ret = checkParameterType(handle, setting, parameter | CVIE_WIDE_PATH, CVIE_PARTYPE_WIDE_STRING);
    if (ret != CVIE_E_OK)
        return ret;

    return m_impl->GetParameterValue(handle, setting, parameter | CVIE_WIDE_PATH, value);
}


inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, std::string& value) const
{
    int length;
    ECVIE ret = checkParameterType(handle, setting, parameter, CVIE_PARTYPE_STRING, &length);
    if (ret != CVIE_E_OK)
        return ret;

    value.resize(length);
    ret = m_impl->GetParameterValue(handle, setting, parameter, &value[0]);
    if (ret == CVIE_E_OK)
        value.resize(value.find('\0'));     // Remove trailing nul characters. std::string will add its own if c_str is called.
    else
        value.clear();

    return ret;
}


inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, std::wstring& value) const
{
    int length;
    ECVIE ret = checkParameterType(handle, setting, parameter | CVIE_WIDE_PATH, CVIE_PARTYPE_WIDE_STRING, &length);
    if (ret != CVIE_E_OK)
        return ret;

    value.resize(length / sizeof(wchar_t));
    ret = m_impl->GetParameterValue(handle, setting, parameter | CVIE_WIDE_PATH, &value[0]);
    if (ret == CVIE_E_OK)
        value.resize(value.find(L'\0'));     // Remove trailing nul character. std::wstring will add its own if c_str is called.
    else
        value.clear();

    return ret;
}


inline ECVIE CVIEDynamicLibrary::getParameterValue(HCVIE handle, int setting, int parameter, void*& value) const
{
    int length;
    ECVIE ret = checkParameterType(handle, setting, parameter, CVIE_PARTYPE_HANDLE, &length);
    if (ret != CVIE_E_OK)
        return ret;

    ret = m_impl->GetParameterValue(handle, setting, parameter, &value);
    return ret;
}


template<typename T> inline ECVIE CVIEDynamicLibrary::setParameterValue(HCVIE handle, int setting, int parameter, const T& value) const 
{
    // This is an incomplete test. But as we don't have individual constants for each struct type its the best we can do.
    ECVIE ret = checkParameterType(handle, setting, parameter, CVIE_PARTYPE_STRUCT);
    if (ret != CVIE_E_OK)
        return ret;

    return m_impl->SetParameterValue(handle, setting, parameter, &value);
}


inline ECVIE CVIEDynamicLibrary::setParameterValue(HCVIE handle, int setting, int parameter, const int& value) const
{
    int type, length;
    ECVIE ret = getParameterInfo(handle, setting, parameter, type, length);
    if (ret != CVIE_E_OK)
        return ret;

    if (type == CVIE_PARTYPE_INT) {
        return m_impl->SetParameterValue(handle, setting, parameter, &value);
    }

    if (type == CVIE_PARTYPE_DOUBLE) {
        double tmp = value;
        return m_impl->SetParameterValue(handle, setting, parameter, &tmp);
    }

    if (type == CVIE_PARTYPE_FLOAT) {
        float tmp = float(value);
        return m_impl->SetParameterValue(handle, setting, parameter, &tmp);
    }

    throw std::logic_error("Parameter type mismatch");
}

inline ECVIE CVIEDynamicLibrary::setParameterValue(HCVIE handle, int setting, int parameter, const float& value) const
{
    int type, length;
    ECVIE ret = getParameterInfo(handle, setting, parameter, type, length);
    if (ret != CVIE_E_OK)
        return ret;

    if (type == CVIE_PARTYPE_DOUBLE) {
        double tmp = value;
        return m_impl->SetParameterValue(handle, setting, parameter, &tmp);
    }

    if (type == CVIE_PARTYPE_FLOAT) {
        return m_impl->SetParameterValue(handle, setting, parameter, &value);
    }

    throw std::logic_error("Parameter type mismatch");
}

inline ECVIE CVIEDynamicLibrary::setParameterValue(HCVIE handle, int setting, int parameter, const double& value) const
{
    int type, length;
    ECVIE ret = getParameterInfo(handle, setting, parameter, type, length);
    if (ret != CVIE_E_OK)
        return ret;

    if (type == CVIE_PARTYPE_DOUBLE) {
        return m_impl->SetParameterValue(handle, setting, parameter, &value);
    }

    if (type == CVIE_PARTYPE_FLOAT) {
        float tmp = float(value);
        return m_impl->SetParameterValue(handle, setting, parameter, &tmp);
    }

    throw std::logic_error("Parameter type mismatch");
}


inline ECVIE CVIEDynamicLibrary::setParameterValue(HCVIE handle, int setting, int parameter, const char* value) const
{
    int type, size;
    ECVIE ret = getParameterInfo(handle, setting, parameter, type, size);
    if (ret != CVIE_E_OK)
        return ret;

    if (type != CVIE_PARTYPE_STRING && type != CVIE_PARTYPE_MASK)
        throw std::logic_error("Parameter type mismatch");

    return m_impl->SetParameterValue(handle, setting, parameter, value);
}


inline ECVIE CVIEDynamicLibrary::setParameterValue(HCVIE handle, int setting, int parameter, const wchar_t* value) const
{
    ECVIE ret = checkParameterType(handle, setting, parameter | CVIE_WIDE_PATH, CVIE_PARTYPE_WIDE_STRING);
    if (ret != CVIE_E_OK)
        return ret;

    return m_impl->SetParameterValue(handle, setting, parameter | CVIE_WIDE_PATH, value);
}
///\} CVIEDynamicLibrary
}   // namespace cvie

#endif    // Include guard

