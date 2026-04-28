//=============================================================================
///
/// @file dynamic_library.h
///
/// @brief Functionality for explicitly loading CVIE dynamic library from C.
///
/// Copyright (c) 2023-2024 ContextVision AB.
///
//=============================================================================
#ifndef DYNAMIC_LIBRARY_HEADER_INCLUDED
#define DYNAMIC_LIBRARY_HEADER_INCLUDED
#pragma once

#if !defined(CVIE_LOAD_IMPLICIT) && !defined(CVIE_LOAD_EXPLICIT)
#   define CVIE_LOAD_IMPLICIT /* Use implicit loading by default */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** \cond */

#ifdef CVIE_LOAD_EXPLICIT
#   if defined(_WIN32)
#       ifndef _WINDOWS_
#           ifndef WIN32_LEAN_AND_MEAN
#               define WIN32_LEAN_AND_MEAN
#           endif
/*          Must get rid of min / max macros as we may use the min and max functions in std. */
#           ifndef NOMINMAX
#               define NOMINMAX
#           endif
#           include <windows.h>
#       endif
#   else
#       if defined(_FEATURES_H) && !defined(__USE_GNU)
#           error "CVIEOpenLibrary uses the dladdr function which requires _GNU_SOURCE to be defined before any standard library header is included"
#       endif
#       ifndef _GNU_SOURCE
#           define _GNU_SOURCE
#       endif
#       include <dlfcn.h>
#   endif
#   if defined(__APPLE__)
#       include <CoreFoundation/CoreFoundation.h>
#       include <TargetConditionals.h>
#   endif
#endif /* CVIE_LOAD_EXPLICIT */

#include <string.h>

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <string>
#include <memory>
#include <stdexcept>
#include <vector>

#endif

#include "cvie.h"

/** \endcond */

/** \defgroup TCVIEDynamicLibrary CVIE Dynamic Library Loader
 *
 * \brief Functionality to simplify explicit loading of the CVIE dynamic library.
 *
 * This file contains a struct of function pointer, one each for the non-deprecated functions in the CVIE library. A function
 * is included which loads the dynamic library given the filename and sets up the function pointers. Another function unloads the
 * library.
 *
 * __Implicit__ loading is when the corresponding .lib file (cvie.lib) on Windows or the .so file on Linux is provided on the linker command
 * line. Implicit loading is simpler as the program can just call the functions as usual, but the drawback is that the library is
 * loaded on program start rather than when it is needed, and that the filename of the dynamic library must never be changed.
 *
 * __Explicit__ loading of a dynamic library is when the application loads the library using operating system functions. This
 * offers more control but is harder to implement.
 *
 * \note For new projects we recommend using the C++ API definied in cvie.hpp instead as it provides type safe operation and
 * automatic resource management. The C++ API always uses the functionality here and by default loads the dynamic library implicitly.
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
    TCVIEReleaseMemory ReleaseMemory;
    TCVIEExit Exit;

    TCVIECreateWorkerVer CreateWorkerVer;
    TCVIECreateWorkerVerW CreateWorkerVerW;
    TCVIECreateWorkerFromBufferVer CreateWorkerFromBufferVer;
    TCVIEDestroyWorker DestroyWorker;
    TCVIESaveWorkerParameterFile SaveWorkerParameterFile;
    TCVIESaveWorkerParameterFileW SaveWorkerParameterFileW;
    TCVIESetup Setup;
    TCVIERun Run;
    TCVIEResetTemporalState ResetTemporalState;

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
    /**@}*/

    /**@{*/
    /** Function pointers for each CVLM function, which are set to point to the entry points of the loaded dynamic library by
     ** ::CVIEOpenLibrary function. */
    TCVLMIsLicensed IsLicensed;
    TCVLMGetDeviceId GetDeviceId;
    TCVLMGetLicenseMethods GetLicenseMethods;
    TCVLMGetProductIds GetProductIds;
    TCVLMGetLicenseInfo GetLicenseInfo;
    TCVLMActivateProduct ActivateProduct;
    TCVLMSetParameterValue CVLMSetParameterValue;           /* Avoid clash with previous function pointer name by retaining. */
    TCVLMDeactivateProduct DeactivateProduct;
    /**@}*/

    TCVEMGetLastError GetLastError;

    /** Handle to loaded dynamic library. NULL if CVIELoadLibraryImplicit was called. */
#if defined(_WIN32) && defined(CVIE_LOAD_EXPLICIT)
    HMODULE handle;
#else
    void* handle;
#endif

    ECVIE errorCode;        /**< Saved error code from CVIEOpenLibrary */
} TCVIEDynamicLibrary;

#ifdef CVIE_LOAD_EXPLICIT
/** Internal helper
 ** \cond
 */
static inline void CVIE__setupFunctions(TCVIEDynamicLibrary* library, int forWorker)
{
   library->errorCode = CVIE_E_OK;

#ifdef _WIN32
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
    CVIE__SetFunOpt(ReleaseMemory);
    CVIE__SetFunOpt(Exit);

    CVIE__SetWorkerFun(CreateWorkerVer);
    CVIE__SetWorkerFun(CreateWorkerVerW);
    CVIE__SetWorkerFun(CreateWorkerFromBufferVer);
    CVIE__SetWorkerFun(DestroyWorker);
    CVIE__SetWorkerFun(SaveWorkerParameterFile);
    CVIE__SetWorkerFun(SaveWorkerParameterFileW);

    CVIE__SetWorkerFun(Setup);
    CVIE__SetWorkerFun(Run);
    CVIE__SetFunOpt(ResetTemporalState);

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


#ifdef _WIN32
#define CVLM__SetFun(NAME) library->NAME = (TCVLM##NAME)GetProcAddress(library->handle, "CVLM" #NAME);
#else
#define CVLM__SetFun(NAME) library->NAME = (TCVLM##NAME)dlsym(library->handle, "CVLM" #NAME);
#endif
    CVLM__SetFun(IsLicensed);
    CVLM__SetFun(GetDeviceId);
    CVLM__SetFun(GetLicenseMethods);
    CVLM__SetFun(GetProductIds);
    CVLM__SetFun(GetLicenseInfo);
    CVLM__SetFun(ActivateProduct);
    CVLM__SetFun(DeactivateProduct);

#ifdef _WIN32
    library->CVLMSetParameterValue = (TCVLMSetParameterValue)GetProcAddress(library->handle, "CVLMSetParameterValue");
#else
    library->CVLMSetParameterValue = (TCVLMSetParameterValue)dlsym(library->handle, "CVLMSetParameterValue");
#endif


    /* Can't use macro as function prefix is CVEM */
#ifdef _WIN32
    library->GetLastError = (TCVEMGetLastError)GetProcAddress(library->handle, "CVEMGetLastError");
#else
    library->GetLastError = (TCVEMGetLastError)dlsym(library->handle, "CVEMGetLastError");
#endif
    if (library->GetLastError == NULL)
        library->errorCode = CVIE_E_FATAL_ERROR;
        
#undef CVIE__SetFun    
#undef CVIE__SetFunOpt
#undef CVIE__SetWorkerFun
#undef CVLM__SetFun    

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
 * \note To use explicit loading, the CVIE_LOAD_EXPLICIT macro must be defined, see \ref ExplicitLibraryLoading for more details.
 * 
 * \param [out] library  The address of a TCVIEDynamicLibrary struct to be filled in.
 *
 * \param [in] filename  Name of the library to load. This must be an absolute or relative path that points to the CVIE dynamic
 *                       library. The function will not search PATH.
 *
 * \param [in] forWorker The library will be used with the CVIE Worker functions. This requires a dynamic library version 7.0 or
 *                       newer. If Worker functions are not needed set this parameter to 0 and no error will result for older CVIE libraries.
 *
 * \return               If the function succeeds, the returned value is ::CVIE_E_OK, else the returned value is an error code:
 *                       ::CVIE_E_FILEIO_ERROR if the dynamic library could not be loaded.
 *                       ::CVIE_E_FATAL_ERROR if one of the mandatory functions could not be found in the dynamic library.
 *                     
 * \note If the dynamic library does not contain some of the optional functions ::CVIE_E_OK will still be returned. This should
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

static inline ECVIE CVIEOpenLibrary(TCVIEDynamicLibrary* library, const char* filename, int forWorker)
{
#ifndef _WIN32
    char buffer[1024];      /* Buffer for complete name used by Linux. */
#endif

#ifdef _WIN32
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
    if (filename == NULL || *filename == 0)
        filename = "libcvie64.so";

    if (strchr(filename, '/') == NULL) {
        /* Obtain the full path of the executable (program or shared object) trying to load cvie library. Assume that the CVIE
           library is located in the same directory. However, if the filename is reported without its path we can't do more but have
           to hope that the CVIE library can be found in the standard locations. */
        Dl_info DlInfo;
        int nRet = dladdr((void*)CVIE__setupFunctions, &DlInfo);
        if (nRet != 0) {
            const char* end = strrchr(DlInfo.dli_fname, '/');
            if (end != NULL) {
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

#ifdef _WIN32
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
static inline void CVIECloseLibrary(TCVIEDynamicLibrary* library)
{
    if (library->handle != NULL) {
        library->ReleaseMemory(NULL);
        library->Exit();
#ifdef _WIN32
        FreeLibrary(library->handle);
#else
        dlclose(library->handle);
#endif
    }

    /* Make sure calling functions after unloading crashes directly. */
    memset(library, 0, sizeof(TCVIEDynamicLibrary));
    library->errorCode = CVIE_E_BAD_HANDLE;
}

#endif /* CVIE_LOAD_EXPLICIT */

/* This function is only included if implicit loading is enabled, see \ref ExplicitLibraryLoading.
 * This is as the references to the library functions are inclued even if this inline function is never called.
 */
 #ifdef CVIE_LOAD_IMPLICIT

/**
 * CVIELoadLibraryImplicit is used to fill the pointers in a TCVIEDynamicLibrary from
 * the actual functions. This causes the dynamic library to be implicitly loaded. This function
 * can only be used if implicit loading is enabled, see \ref ExplicitLibraryLoading.
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
static inline void CVIELoadLibraryImplicit(TCVIEDynamicLibrary* library)
{
    library->errorCode = CVIE_E_OK;
    library->handle = NULL;

/** \cond */
#define CVIE__SetFun(NAME) library->NAME = CVIE##NAME;
/** \endcond */
    CVIE__SetFun(Create);
    CVIE__SetFun(Destroy);

    CVIE__SetFun(SetThreads);
    CVIE__SetFun(ReleaseMemory);
    CVIE__SetFun(Exit);

    CVIE__SetFun(CreateWorkerVer);
    CVIE__SetFun(CreateWorkerVerW);
    CVIE__SetFun(CreateWorkerFromBufferVer);
    CVIE__SetFun(DestroyWorker);
    CVIE__SetFun(SaveWorkerParameterFile);
    CVIE__SetFun(SaveWorkerParameterFileW);

    CVIE__SetFun(Setup);
    CVIE__SetFun(Run);
    CVIE__SetFun(ResetTemporalState);

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

    /** \cond */
#define CVLM__SetFun(NAME) library->NAME = CVLM##NAME;
    /** \endcond */

    CVLM__SetFun(IsLicensed);
    CVLM__SetFun(GetDeviceId);
    CVLM__SetFun(GetLicenseMethods);
    CVLM__SetFun(GetProductIds);
    CVLM__SetFun(GetLicenseInfo);
    CVLM__SetFun(ActivateProduct);
    CVLM__SetFun(DeactivateProduct);

#undef CVLM__SetFun    

    library->CVLMSetParameterValue = CVLMSetParameterValue;
    library->GetLastError = CVEMGetLastError;
}

#endif /* CVIE_LOAD_IMPLICIT */

/**\} TCVIEDynamicLibrary */
#endif
