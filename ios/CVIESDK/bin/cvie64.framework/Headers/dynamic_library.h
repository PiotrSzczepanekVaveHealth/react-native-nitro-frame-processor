#ifndef CVIE_DYNAMIC_LIBRARY_HEADER
#define CVIE_DYNAMIC_LIBRARY_HEADER
#pragma once


/** \cond */

#include "cvie.h"


#ifdef __cplusplus

#include <string>
#include <memory>
#include <stdexcept>

#endif

/* These includes are used for the Dynamic Library loading functionality */
extern "C" {

#if defined(WIN32)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif

/* Must get rid of min/max macros as we may use the min and max functions in std. */
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>

#else

#include <dlfcn.h>
#include <string.h>

#endif

#if defined(__APPLE__)
#    include <CoreFoundation/CoreFoundation.h>
#    include <TargetConditionals.h>
#endif

}

/** \endcond */

/** \defgroup CVIEDynamicLibrary CVIE Dynamic Library Loader
 *
 * \brief Functionality to simplify explicit loading of the CVIE dynamic library.
 *
 * This file contains a struct of function pointer, one each for the non-deprecated functions in the CVIE library. A function
 * is included which loads the dynamic library given the filename and sets up the function pointers. Another function unloads the
 * library.
 *
 * __Implicit__ loading is when the corresponding .lib file (cvie.lib) on Windows or the .so file on Linux is provided on the linker command
 * line. Implicit loading is simpler as yhe program can just call the functions as usual, but the drawback is that the library is
 * loaded on program start rather than when it is needed, and that the filename of the dynamic library must never be changed.
 *
 * __Explicit__ loading of a dynamic library is when the application loads the library using operating system functions. This
 * offers more control but is harder to implement.
 *
 * \note For new projects we recommend using the C++ API definied in cvie.hpp instead as it provides type safe operation and
 * automatic resource management. The C++ API always uses the functionality here and by default loads the dynamic library when the
 * first Worker object is created.
 *
 * CONFIDENTIAL
 * \copyright Copyright (c) ContextVision AB.
 *
 \{
*/


/** \anchor TCVIEDynamicLibrary Helper struct and functions to simplify using explicitly loaded libraries from C. */
typedef struct TCVIEDynamicLibrary {
    /**@{*/
    /** Function pointers for each CVIE function, which are set to point to the entry points of the loaded dynamic library by
     ** ::CVIEOpenLibrary function. */
    TCVIECreate Create;
    TCVIEDestroy Destroy;

    TCVIESetThreads SetThreads;

    TCVIECreateWorkerVer CreateWorkerVer;
    TCVIECreateWorkerVerW CreateWorkerVerW;
    TCVIECreateWorkerFromBufferVer CreateWorkerFromBufferVer;
    TCVIEDestroyWorker DestroyWorker;
    TCVIESaveWorkerParameterFile SaveWorkerParameterFile;
    TCVIESaveWorkerParameterFileW SaveWorkerParameterFileW;
    TCVIESetup Setup;
    TCVIERun Run;

    TCVIESetParameterFile SetParameterFile;
    TCVIESetParameterFileW SetParameterFileW;
    TCVIESetParameterBuffer SetParameterBuffer;
    TCVIESaveParameterFile SaveParameterFile;
    TCVIESaveParameterFileW SaveParameterFileW;

    TCVIEEnhanceSetup EnhanceSetup;
    TCVIEEnhanceNext EnhanceNext;

    TCVIEGetParameterInfo GetParameterInfo;
    TCVIEGetDIInfo GetDIInfo;
    TCVIEGetParameterValue GetParameterValue;
    TCVIESetParameterValue SetParameterValue;

    TCVEMGetLastError GetLastError;
    /**@}*/

    /** Handle to loaded dynamic library. NULL if CVIELoadLibraryImplicit was called. */
#ifdef WIN32
    HMODULE handle;
#else
    void* handle;
#endif
    ECVIE errorCode;        /**< Saved error code from CVIEOpenLibrary */
} TCVIEDynamicLibrary;


/** Internal helper
 ** \cond
*/
inline void CVIE__setupFunctions(TCVIEDynamicLibrary* library, int forWorker)
{
   library->errorCode = CVIE_E_OK;

#ifdef WIN32
#define CVIE__SetFunOpt(NAME) library->NAME = (TCVIE##NAME)GetProcAddress(library->handle, "CVIE" #NAME)
#else
#define CVIE__SetFunOpt(NAME) library->NAME = (TCVIE##NAME)dlsym(library->handle, "CVIE" #NAME)
#endif
#define CVIE__SetFun(NAME)               \
    CVIE__SetFunOpt(NAME);               \
    if (library->NAME == NULL)           \
        library->errorCode = CVIE_E_FATAL_ERROR

#define CVIE__SetWorkerFun(NAME)            \
    CVIE__SetFunOpt(NAME);                  \
    if (library->NAME == NULL && forWorker) \
        library->errorCode = CVIE_E_FATAL_ERROR

    CVIE__SetFun(Create);
    CVIE__SetFun(Destroy);

    CVIE__SetFun(SetThreads);

    CVIE__SetWorkerFun(CreateWorkerVer);
    CVIE__SetWorkerFun(CreateWorkerVerW);
    CVIE__SetWorkerFun(CreateWorkerFromBufferVer);
    CVIE__SetWorkerFun(DestroyWorker);
    CVIE__SetWorkerFun(SaveWorkerParameterFile);
    CVIE__SetWorkerFun(SaveWorkerParameterFileW);

    CVIE__SetWorkerFun(Setup);
    CVIE__SetWorkerFun(Run);

    CVIE__SetFun(SetParameterFile);
    CVIE__SetFunOpt(SetParameterFileW);
    CVIE__SetFun(SetParameterBuffer);
    CVIE__SetFun(SaveParameterFile);
    CVIE__SetFunOpt(SaveParameterFileW);

    CVIE__SetFun(EnhanceSetup);
    CVIE__SetFun(EnhanceNext);

    CVIE__SetFun(GetParameterInfo);
    CVIE__SetFunOpt(GetDIInfo);
    CVIE__SetFun(GetParameterValue);
    CVIE__SetFun(SetParameterValue);

    /* Can't use macro as function prefix is CVEM */
#ifdef WIN32
    library->GetLastError = (TCVEMGetLastError)GetProcAddress(library->handle, "CVEMGetLastError");
#else
    library->GetLastError = (TCVEMGetLastError)dlsym(library->handle, "CVEMGetLastError");
#endif
    if (library->GetLastError == NULL)
        library->errorCode = CVIE_E_FATAL_ERROR;
        
#undef CVIE__SetFun    
#undef CVIE__SetFunOpt
#undef CVIE__SetWorkerFun
}
/** \endcond */

/**
 * The CVIEOpenLibrary function is used to load the CVIE dynamic library and set up the function pointers in a TCVIEDynamicLibrary
 * struct so that the functions in the CVIE library can easily be called. This function is defined as an inline function. As it
 * loads the dynamic library it can't be located in the library itself. By making the function inline compilation of separate .c
 * file containing the code is avoided.
 *
 * Calling CVIEOpenLibrary multiple times for the same filename is ok and not very costly as the operating system reference counts
 * the dynamic library loads of the same file. Setting up all the function pointers can take a little time though. HCVIE handles
 * created from one TCVIEDynamicLibrary can be used with other TCVIEDynamicLibrary instances loaded from the same dynamic library file.
 *
 * It is also possible to load more than one CVIE dynamic library side to side if separate TCVIEDynamicLibrary structs are used. In
 * this case the HCVIE handles are local to each library, and only functions in the same library where the handle was created may
 * be called.
 * 
 * \param [out] library  The address of a TCVIEDynamicLibrary struct to be filled in.
 *
 * \param [in] filename  Name of the library to load. This must be an absolute or relative path that points to the CVIE dynamic
 *                       library. The function will not search PATH.
 *
 * \param [in] forWorker The library will be used with the CVIE Worker functions. This requires a dynamic library version 7.0 or
 *                       newer. If Worker functions are not needed set this parameter to 0 and no error will result for older CVIE libraries.
 *
 * \return               If the function succeeds, the returned value is \ref CVIE_E_OK, else the returned value is an error code:
 *                       \ref CVIE_E_FILEIO_ERROR if the dynamic library could not be loaded.
 *                       \ref CVIE_E_FATAL_ERROR if one of the mandatory functions could not be found in the dynamic library.
 *                     
 * \note If the dynamic library does not contain some of the optional functions \ref CVIE_E_OK will still be returned. This should
 * only happen if the dynamic library is of an older version than this header file, or if different variants are mixed (GPU vs. CPU
 * SDKs). To see if the function exists compare the function pointer to NULL.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ECVIE cvie_err;
 *
 *     TCVIEDynamicLibrary library;
 *     cvie_err = CVIEOpenLibrary(&library, "cvie.dll", true);
 *     assert(cvie_err == CVIE_E_OK);
 *
 *     // Create an instance using the syntax library.Create
 *     HCVIE handle;
 *     cvie_err = library.Create(&handle, CVIE_CREATE_DEFAULT);
 *     assert(cvie_err == CVIE_E_OK);
 *
 *     // Continue using the library functions until all image processing is done...
 *     library.SetParameterFile(...);
 *
 *     // Close the library
 *     CVIECloseLibrary(&library);
 * }
 * \endcode
 */

inline ECVIE CVIEOpenLibrary(TCVIEDynamicLibrary* library, const char* filename, int forWorker)
{
#ifndef WIN32
    char buffer[1024];      /* Buffer for complete name used by Linux. */
#endif

#ifdef WIN32
    if (filename == NULL || *filename == 0) {
        if (sizeof(void*) == 8)
            filename = "cvie64.dll";
        else
            filename = "cvie.dll";
#elif defined(__APPLE__)
    if (filename == NULL || *filename == 0) {
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);

        CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
        CFStringGetCString(cfStringRef, buffer, MAX_INPUT, kCFStringEncodingUTF8);

        CFRelease(mainBundleURL);
        CFRelease(cfStringRef);

        strcat(buffer, "/Frameworks/cvie64.framework/cvie64");
        filename = buffer;
#else /* Linux and Android */
    if (filename == NULL || *filename == 0) {
        if (sizeof(void*) == 8)
            filename = "libcvie64.so";
        else
            filename = "libcvie.so";
    }

    if (strchr(filename, '/') == NULL) {
        /* Obtain the full path of the executable (program or shared library) trying to load cvie library. Assume that the CVIE
           library is located in the same directory. However, if the filename is reported without its path we can't do more but have
           to hope that the CVIE library can be found in the standard locations. */
        Dl_info DlInfo;
        int nRet = dladdr((void*)CVIE__setupFunctions, &DlInfo);
        if (nRet != 0) {
            const char* end = strrchr(DlInfo.dli_fname, '/');
            if (end != nullptr) {
                size_t len = end - DlInfo.dli_fname;
                if (len < 1000) {
                    memcpy(buffer, DlInfo.dli_fname, len + 1);
                    strcpy(buffer + len + 1, filename);
                }
            }

            /* try this combined path as a first try. */
            library->handle = dlopen(buffer, RTLD_NOW);
            if (library->handle != NULL) {
                CVIE__setupFunctions(library, forWorker);
                return library->errorCode;
            }

            /* If this fails try the standard places using code below, see https://man7.org/linux/man-pages/man3/dlopen.3.html. */
        }

#endif
    }

#ifdef WIN32
    library->handle = LoadLibraryA(filename);
#else
    library->handle = dlopen(filename, RTLD_NOW);
#endif
    if (library->handle != NULL)
        CVIE__setupFunctions(library, forWorker);
    else
        library->errorCode = CVIE_E_FILEIO_ERROR;

    return library->errorCode;
}




/**
 * The CVIECloseLibrary function is used to unload a CVIE dynamic library previously loaded using CVIEOpenLibrary.
 *
 * \param [in] library The address of a TCVIEDynamicLibrary to be unloaded (unless loaded implicitly).
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ECVIE cvie_err;
 *
 *     TCVIEDynamicLibrary library;
 *     cvie_err = CVIEOpenLibrary(&library, "cvie.dll", true);
 *     assert(cvie_err == CVIE_E_OK);
 *     
 *     // Use the functions pointers in library here.
 *
 *     // Close the library
 *     CVIECloseLibrary(&library);
 * }
 * \endcode
 */
inline void CVIECloseLibrary(TCVIEDynamicLibrary* library)
{
    if (library->handle != NULL) {
#ifdef WIN32
        FreeLibrary(library->handle);
#else
        dlclose(library->handle);
#endif
    }

    /* Make sure calling functions after unloading crashes directly. */
    memset(library, 0, sizeof(TCVIEDynamicLibrary));
    library->errorCode = CVIE_E_BAD_HANDLE;
}


// This function is only included if CVIE_USE_IMPLICIT_LOADING is defined by the build system.
// This is as the references to the library functions are inclued even if this inline function is never called.
#ifdef CVIE_USE_IMPLICIT_LOADING

/**
 * The CVIELoadLibraryImplicit is used to fill the pointers in a TCVIEDynamicLibrary from
 * the actual functions. This causes the dynamic library to be implicitly loaded. to be able to use this function
 * the preprocessor variable CVIE_USE_IMPLICIT_LOADING must be defined in the build system.
 *
 * \param [in] library The address of a TCVIEDynamicLibrary to be filled in.
 *
 * \par Code example
 * \code{.c}
 * int main()
 * {
 *     ECVIE cvie_err;
 *
 *     TCVIEDynamicLibrary library;
 *     CVIELoadLibraryImplicit(&library);
 *     
 *     // Use the functions pointers in library here.
 *
 *     // Close the library
 *     CVIECloseLibrary(&library);
 * }
 * \endcode
 */
inline void CVIELoadLibraryImplicit(TCVIEDynamicLibrary* library)
{
    library->errorCode = CVIE_E_OK;
    library->handle = NULL;

/** \cond */
#define CVIE__SetFun(NAME) library->NAME = CVIE##NAME;
/** \endcond */
    CVIE__SetFun(Create);
    CVIE__SetFun(Destroy);

    CVIE__SetFun(SetThreads);

    CVIE__SetFun(CreateWorkerVer);
    CVIE__SetFun(CreateWorkerVerW);
    CVIE__SetFun(CreateWorkerFromBufferVer);
    CVIE__SetFun(DestroyWorker);
    CVIE__SetFun(SaveWorkerParameterFile);
    CVIE__SetFun(SaveWorkerParameterFileW);

    CVIE__SetFun(Setup);
    CVIE__SetFun(Run);

    CVIE__SetFun(SetParameterFile);
    CVIE__SetFun(SetParameterFileW);
    CVIE__SetFun(SetParameterBuffer);
    CVIE__SetFun(SaveParameterFile);
    CVIE__SetFun(SaveParameterFileW);

    CVIE__SetFun(EnhanceSetup);
    CVIE__SetFun(EnhanceNext);

    CVIE__SetFun(GetParameterInfo);
    CVIE__SetFun(GetDIInfo);
    CVIE__SetFun(GetParameterValue);
    CVIE__SetFun(SetParameterValue);

#undef CVIE__SetFun    

    library->GetLastError = CVEMGetLastError;
}

#endif

#ifdef __cplusplus

namespace cvie {

/** A CVIEDynamicLibrary is a C++ wrapper of a TCVIEDynamicLibrary struct with shallow copy semantics. It only does minimal type
conversions for functions handling strings. */
class CVIEDynamicLibrary {
public:
    CVIEDynamicLibrary() = default;
    /// @name: Constructors
    /// Construct a CVIEDynamicLibrary using the default fileanme or a specific filename.
    /// The forWorker parameter can be set to false to be able to dynamically load an older CVIE library than 7.0
    /// without error. However, any call to worker related methods may then cause an access violation error.
    /// @{
    ///
    /// If CVIE_USE_IMPLICIT_LOADING is defined by the builds system this constructor loads the library implicitly.
    CVIEDynamicLibrary(bool forWorker);
    /// Always loads the library explicitly. Use "" as the libraryName to load the library using its default name.
    CVIEDynamicLibrary(const std::string& libraryName, bool forWorker = true);

    ///@}

    /// Check if the dynamic library was loaded and all its mandatory functions found.
    bool isOk() const { return m_impl != nullptr && m_impl->isOk(); }

    /// @{
    /// Forwarding methods which call the corresponding CVIE C functions in the loaded library.
    /// string values are accepted as std::string in addition to const char*. For functions involving filenames there are
    /// also std::wstring based overloads.
    ECVIE create(HCVIE& handle, int flags) const { checkOk(); return m_impl->Create(&handle, flags); }

    ECVIE destroy(HCVIE& handle) const { checkOk(); return m_impl->Destroy(&handle); }

    ECVIE createWorker(CVIEWorker& worker, HCVIE handle, const std::string& filename, int setting) const { checkOk(); return m_impl->CreateWorkerVer(&worker, handle, filename.c_str(), setting, CVIE_WORKER_VERSION); }
    ECVIE createWorker(CVIEWorker& worker, HCVIE handle, const std::wstring& filename, int setting) const { checkOk(); return m_impl->CreateWorkerVerW(&worker, handle, filename.c_str(), setting, CVIE_WORKER_VERSION); }
    ECVIE createWorkerFromBuffer(CVIEWorker& worker, HCVIE handle, const char* buffer, int length, int setting) const { checkOk(); return m_impl->CreateWorkerFromBufferVer(&worker, handle, buffer, length, setting, CVIE_WORKER_VERSION); }
    ECVIE destroyWorker(CVIEWorker& worker) const { checkOk(); return m_impl->DestroyWorker(&worker); }
    ECVIE saveWorkerParameterFile(CVIEWorker worker, const std::string& filename) const { checkOk(); return m_impl->SaveWorkerParameterFile(worker, filename.c_str()); }
    ECVIE saveWorkerParameterFile(CVIEWorker worker, const std::wstring& filename) const { checkOk(); return m_impl->SaveWorkerParameterFileW(worker, filename.c_str()); }
    ECVIE setup(CVIEWorker worker) const { checkOk(); return m_impl->Setup(worker); }
    ECVIE run(CVIEWorker worker) const { checkOk(); return m_impl->Run(worker); }

    ECVIE setThreads(HCVIE handle, int threads) const { checkOk(); return m_impl->SetThreads(handle, threads); }
    ECVIE setParameterFile(HCVIE handle, const std::string& filename, int& settings) const { checkOk(); return m_impl->SetParameterFile(handle, filename.c_str(), &settings); }
    ECVIE setParameterFile(HCVIE handle, const std::wstring& filename, int& settings) const { checkOk(); return m_impl->SetParameterFileW(handle, filename.c_str(), &settings); }
    ECVIE setParameterBuffer(HCVIE handle, const char* buffer, int& settings) const { checkOk(); return m_impl->SetParameterBuffer(handle, buffer, &settings); }
    ECVIE saveParameterFile(HCVIE handle, const std::string& filename) const { checkOk(); return m_impl->SaveParameterFile(handle, filename.c_str()); }
    ECVIE saveParameterFile(HCVIE handle, const std::wstring& filename) const { checkOk(); return m_impl->SaveParameterFileW(handle, filename.c_str()); }
    ECVIE enhanceSetup(HCVIE handle, int width, int height, int flags, int setting, const unsigned char* mask) const { checkOk(); return m_impl->EnhanceSetup(handle, width, height, flags, setting, mask); }
    ECVIE enhanceNext(HCVIE handle, const void* inImage, void* outImage, int setting) const { checkOk(); return m_impl->EnhanceNext(handle, inImage, outImage, setting); }

    ECVIE getParameterInfo(HCVIE handle, int setting, int parameter, int& type, int& length) const { checkOk(); return m_impl->GetParameterInfo(handle, setting, parameter, &type, &length); }
    ECVIE getDIInfo(HCVIE handle, int setting, int parameter, TCVIEDIInfo& info) const { checkOk(); return m_impl->GetDIInfo(handle, setting, parameter, &info); }

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

    template<typename T> ECVIE setParameterValue(HCVIE handle, int setting, int parameter, T* value) const { throw std::logic_error("Use references when getting parameter values, not pointers."); }
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

    std::string getLastError(HCVIE handle) const {
        char buffer[MAX_ERROR_MESSAGE_LENGTH];
        return m_impl->GetLastError(handle, buffer, MAX_ERROR_MESSAGE_LENGTH);
    }
    ///@}

protected:
    void checkOk() const;

private:
    class Impl : public TCVIEDynamicLibrary {
    public:
        Impl(const std::string& libraryName, bool forWorker) { CVIEOpenLibrary(this, libraryName.c_str(), forWorker); }


#ifdef CVIE_USE_IMPLICIT_LOADING
        Impl(bool forWorker) { CVIELoadLibraryImplicit(this); }
#else
        Impl(bool forWorker) { CVIEOpenLibrary(this, "", forWorker); }
#endif
        ~Impl() { CVIECloseLibrary(this); }

        bool isOk() const { return errorCode == CVIE_E_OK; }
    };

    ECVIE checkParameterType(HCVIE handle, int setting, int parameter, int required, int* length = nullptr) const;

    std::shared_ptr<Impl> m_impl;
};


//////////////// CVIEDynamicLibrary methods ////////////////

inline CVIEDynamicLibrary::CVIEDynamicLibrary(bool forWorker) : m_impl(std::make_unique<Impl>(forWorker))
{
}


inline CVIEDynamicLibrary::CVIEDynamicLibrary(const std::string& libraryName, bool forWorker) : m_impl(std::make_unique<Impl>(libraryName, forWorker))
{
}


inline void CVIEDynamicLibrary::checkOk() const
{
    if (!isOk())
        throw std::logic_error("The dynamic library has not been loaded.");
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
        float tmp;
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
        float tmp;
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


}   // namespace cvie

#endif    /* cpluplus */

/**\} CVIEDynamicLibrary */

#endif
