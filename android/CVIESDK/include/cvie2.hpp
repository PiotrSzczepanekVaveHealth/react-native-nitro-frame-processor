//=============================================================================
///
/// @file cvie2.hpp
///
/// @brief Depreacted CVIE2 C++ API header
///
/// Copyright (c) 2015-2024 ContextVision AB.
///
//=============================================================================
#ifndef CVIE2_HPP_INCLUDED
#define CVIE2_HPP_INCLUDED
#pragma once

/**
 * \defgroup CVIE2_CPP CVIE2 C++ Interface
 *
 * \warning This API is deprecated and its documentation is included only for existing customers using it. For new integration please use
 * the \ref CVIECPPOverview.
 *
 * This file contains C++ bindings for the \ref CVIE2
 * It can be used instead of the plain C API to get automatic resource management (RAII) of the CVIE handles.
 * CvieEnter() and CvieExit() must still be called before and after using the library.
 *
 * CONFIDENTIAL
 * Copyright (c) ContextVision AB.
 *
 * \{
 * \cond
 **/

#include "cvie2.h"
#include "cvlm.hpp"

#include <exception>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

#ifdef CHECK_CVIE_CALL
#error CHECK_CVIE_CALL already defined.
#endif

#define CHECK_CVIE_CALL(x)            \
    do {                              \
        CvieResult res = (x);         \
        if (!CVIE_SUCCESS(res))       \
            throw CvieException(res); \
    } while (false)

// Is noexcept supported?
#if defined(__clang__)
#if __has_feature(cxx_noexcept)
#define CVIE_HAS_NOEXCEPT
#endif
#else
#if defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ * 10 + __GNUC_MINOR__ >= 46 || (__cplusplus >= 201103L) || \
        defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023026
#define CVIE_HAS_NOEXCEPT
#endif
#endif

#ifdef CVIE_HAS_NOEXCEPT
#define CVIE_NOEXCEPT noexcept
#else
#define CVIE_NOEXCEPT
#endif


/// \endcond

namespace Cvie {


namespace detail {
    // Traits class to map T to corresponding CvieOptionType value.
    template<typename T> struct TypeTraits {
    };

    template<> struct TypeTraits<float> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_FLOAT;
    };
    template<> struct TypeTraits<int> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_INT32;
    };
    template<> struct TypeTraits<std::string> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_CSTR;
    };
    template<> struct TypeTraits<std::wstring> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_WCS;
    };
    template<size_t N> struct TypeTraits<char[N]> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_CSTR;
    };
    template<size_t N> struct TypeTraits<wchar_t[N]> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_WCS;
    };
    template<> struct TypeTraits<const char*> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_CSTR;
    };
    template<> struct TypeTraits<const wchar_t*> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_WCS;
    };

    template<> struct TypeTraits<CvieImageDescriptor> {
        static const CvieOptionType type = CVIE_OPTION_TYPE_IMAGE_DESCRIPTOR;
    };
    template<typename T> CvieOptionType OptionType() { return TypeTraits<T>::type; }
}    // namespace detail

/** Exception class which is thrown for all errors, including option type errors detected by this wrapper. */
class CvieException : public std::exception {
public:
    /**
     * Creates a new exception from a result code.
     * \param res [in] Result code
     */
    CvieException(CvieResult res) : m_res(res), m_msg(CvieGetLastError()) {}
    /**
     * Creates a new exception from an error message.
     * \param msg [in] Error message
     */
    CvieException(const std::string& msg) : m_res(CVIE_RESULT_UNKNOWN_ERROR), m_msg(msg) {}
    CvieResult result() const { return m_res; } /**< \returns the ::CvieResult associated with the exception */
    const char* what() const CVIE_NOEXCEPT override
    {
        return m_msg.c_str();
    } /**< \returns the error message associated with the exception. */

private:
    CvieResult m_res;
    std::string m_msg;
};

/** Base class for all image descriptors. */
class ImageDescriptor {
public:
    CvieImageDescriptor GetObj() const { return m_desc; } /**< \returns the wrapped CvieImageDescriptor handle */

    /** Detaches the image descriptor from its data buffer.
     * \see CvieImageDescriptor_Detach()
     */
    void Detach() { CvieImageDescriptor_Detach(m_desc); }

protected:
    /// \cond
    CvieImageDescriptor m_desc;
    ImageDescriptor() : m_desc(0) {}
    virtual ~ImageDescriptor() { CvieImageDescriptor_Destroy(&m_desc); }
    /// \endcond

private:
    // Non-copyable
    ImageDescriptor(const ImageDescriptor& rhs);
    ImageDescriptor& operator=(const ImageDescriptor& rhs);
};



/** Image descriptor to which a 2D host memory buffer can be attached. */
class ImageDescriptor2D : public ImageDescriptor {
public:
    /** Creates a 2D image descriptor.
     * \param width [in] the image width
     * \param height [in] the image height
     * \param format [in] the image pixel format
     * \see CvieImageDescriptor_CreateForBuffer2D()
     */
    ImageDescriptor2D(int width, int height, CviePixelFormat format)
    {
        CHECK_CVIE_CALL(CvieImageDescriptor_CreateForBuffer2D(&m_desc, width, height, format));
    }

    /** Attaches a data buffer to the descriptor.
     * \param data [in] pointer to the start of the pixel data
     * \param rowStrideInBytes [in] the number of bytes between the start of each row in the image.
     * \see CvieImageDescriptor_AttachBuffer2D()
     */
    void Attach(void* data, int rowStrideInBytes)
    {
        CHECK_CVIE_CALL(CvieImageDescriptor_AttachBuffer2D(m_desc, data, rowStrideInBytes));
    }
};




/** Image descriptor to which a 3D host memory buffer can be attached. */
class ImageDescriptor3D : public ImageDescriptor {
public:
    /** Creates a 3D image descriptor.
     * \param width [in] the volume width
     * \param height [in] the volume height
     * \param depth [in] the volume depth
     * \param format [in] the volume pixel format
     * \see CvieImageDescriptor_CreateForBuffer3D()
     */
    ImageDescriptor3D(int width, int height, int depth, CviePixelFormat format)
    {
        CHECK_CVIE_CALL(CvieImageDescriptor_CreateForBuffer3D(&m_desc, width, height, depth, format));
    }

    /** Attaches a data buffer to the descriptor.
     * \param data [in] pointer to the start of the pixel data
     * \param rowStrideInBytes [in] the number of bytes between the start of each row in the volume.
     * \param sliceStrideInBytes [in] the number of bytes between the start of each slice in the volume.
     * \see CvieImageDescriptor_AttachBuffer3D()
     */
    void Attach(void* data, int rowStrideInBytes, int sliceStrideInBytes)
    {
        CHECK_CVIE_CALL(CvieImageDescriptor_AttachBuffer3D(m_desc, data, rowStrideInBytes, sliceStrideInBytes));
    }
};




/** Class representing one ParameterSetting retrieved from a Cvie instance. */
class ParameterSetting {
public:
    /** Creates a new ParameterSetting wrapper object.
     *  Should not be called directly, use Instance::GetParameterSetting() instead.
     *  \param s [in] The CvieParameterSetting handle to wrap
     */
    ParameterSetting(CvieParameterSetting s) : m_setting(s) {}

    /** Connects image descriptors for input and output data and prepare for enhancement.
     * \param input [in] input image descriptor
     * \param output [in] output image descriptor
     * \see CvieParameterSetting_Setup()
     */
    void Setup(const ImageDescriptor& input, const ImageDescriptor& output)
    {
        CHECK_CVIE_CALL(CvieParameterSetting_Setup(m_setting, input.GetObj(), output.GetObj()));
    }

    /** Runs enhancement on the data in the buffer attached to the input image descriptor and store
     *  the result in the buffer attached to the output descriptor.
     *  \see CvieParameterSetting_Enhance()
     */
    void Enhance() { CHECK_CVIE_CALL(CvieParameterSetting_Enhance(m_setting)); }

    /** Gets an option value.
     * \tparam T the type of the option value
     * \param option [in] the option ID to get (see \ref SettingOptions)
     * \returns the value of the option
     * \exception CvieException if the type \p T doesn't match the type of the option specified by \p option
     * \see CvieParameterSetting_GetOption()
     */
    template<typename T> T GetOption(CvieOptionId option)
    {
        T value;
        GetOption(option, value);
        return value;
    }

    /** Gets an option value.
     * \tparam T the type of the option value
     * \param option [in] the option ID to get (see \ref SettingOptions)
     * \param value [out] the value of the option
     * \exception CvieException if the type \p T doesn't match the type of the option specified by \p option
     * \see CvieParameterSetting_GetOption()
     */
    template<typename T> void GetOption(CvieOptionId option, T& value)
    {
        CHECK_CVIE_CALL(CvieParameterSetting_GetOption(m_setting, option, &value, CheckType<T>(option), nullptr));
    }

    /** Gets an option value.
     * \param option [in] the option ID to get (see \ref SettingOptions)
     * \param value [out] the value of the option
     * \exception CvieException if the type of the option specified by \p option is not a C string
     * \see CvieParameterSetting_GetOption()
     */
    void GetOption(CvieOptionId option, std::string& value)
    {
        // Resize result to include room for the trailing nul that CvieInstance_GetOption will write.
        value.resize(CheckType<std::string>(option));   // This may be a longer max size.
        CHECK_CVIE_CALL(CvieParameterSetting_GetOption(m_setting, option, const_cast<char*>(value.data()), int(value.size()), nullptr));
        value.resize(std::strlen(value.c_str()));                 // Remove spare part of buffer
    }
    /// GetOption Overload when the value is a wide string path.
    void GetOption(CvieOptionId option, std::wstring& value)
    {
        // This includes room for the trailing nul that CvieInstance_GetOption will write.
        value.resize(CheckType<std::wstring>(option) / sizeof(wchar_t));   // This may be a longer max size.
        CHECK_CVIE_CALL(CvieParameterSetting_GetOption(m_setting, option | CVIE_WIDE_PATH, const_cast<wchar_t*>(value.data()), int(value.size()) * sizeof(wchar_t), nullptr));
        value.resize(std::wcslen(value.c_str()));                 // Remove spare part of buffer
    }

    /** Gets the data type of an option.
     * \param option [in] the option ID to get (see \ref SettingOptions)
     * \return the data type of the option (see ::CvieOptionType)
     * \see CvieParameterSetting_GetOptionInfo()
     */
    CvieOptionType GetOptionType(CvieOptionId option)
    {
        CvieOptionType t;
        CHECK_CVIE_CALL(CvieParameterSetting_GetOptionInfo(m_setting, option, &t, nullptr));

        return t;
    }

    /** Sets an option value.
     * \tparam T the type of the option value
     * \param option [in] the option ID to set (see \ref SettingOptions)
     * \param value [in] the value of the option
     * \exception CvieException if the type \p T doesn't match the type of the option specified by \p option
     * \see CvieParameterSetting_SetOption()
     */
    template<typename T> void SetOption(CvieOptionId option, const T& value)
    {
        CHECK_CVIE_CALL(CvieParameterSetting_SetOption(m_setting, option, &value, CheckType<T>(option)));
    }

    /** Sets an option value.
     * \param option [in] the option ID to set (see \ref SettingOptions)
     * \param value [in] the value of the option
     * \exception CvieException if the type of the option specified by \p option is not a C string
     * \see CvieParameterSetting_SetOption()
     */
    void SetOption(CvieOptionId option, const std::string& value)
    {
        CheckType<std::string>(option);
        CHECK_CVIE_CALL(CvieParameterSetting_SetOption(m_setting, option, value.c_str(), int(value.size()) + 1));    // + 1 for terminating null byte
    }
    /// SetOption Overload when the value is a wide string path.
    void SetOption(CvieOptionId option, const std::wstring& value)
    {
        CheckType<std::wstring>(option);
        CHECK_CVIE_CALL(CvieParameterSetting_SetOption(m_setting, option | CVIE_WIDE_PATH, value.c_str(), int(value.size()) + 1));    // + 1 for terminating null byte
    }

    /** Gets information about a DoctorsInterface parameter.
     * \param [in] option A value indicating which tuning or API parameter to get info for. Available API parameters are in \ref CVIE_APIPar.
     *                    Available tuning parameters are \ref tp_parameters.
     * \return the TCVIEDIInfo struct for the selected parameter.
     * \exception CvieException if the option ID is invalid.
     */
    TCvieDIInfo GetDIInfo(CvieOptionId option)
    {
        TCvieDIInfo ret;
        CHECK_CVIE_CALL(CvieParameterSetting_GetDIInfo(m_setting, option, &ret));
        return ret;
    }

private:
    // Helper method for parameter SetOption/GetOption
    template<typename T> int CheckType(CvieOptionId option) { return CheckType(option, detail::OptionType<T>()); }
    int CheckType(CvieOptionId option, CvieOptionType requiredType)
    {
        CvieOptionType valueType;
        int valueSize;

        CHECK_CVIE_CALL(CvieParameterSetting_GetOptionInfo(m_setting, option, &valueType, &valueSize));
        if (valueType != requiredType)
            // The runtime returned an option type that doesn't match the one declared in this header
            throw CvieException("Option type mismatch.");

        return valueSize;
    }

    CvieParameterSetting m_setting;
};    // Class ParameterSetting


/** Class representing a CVIE instance. */
class Instance {
public:
    /** Creates a new CVIE Instance handle.
     * \see CvieInstance_Create()
     */
    Instance() : m_instance(nullptr) { CHECK_CVIE_CALL(CvieInstance_Create(&m_instance)); }

    ~Instance() { CvieInstance_Destroy(&m_instance); }

    /** Loads parameters from a file.
     *  \param fileName [in] the full path of the file to load
     *                       The string shall use the character encoding defined by the current system code page for the narrow
     *                       sting version. For the wide string version any Unicode characters can be used.
     *  \see CvieInstance_LoadParameterFile()
     *  \see WideFilenames
     */
    void LoadParameterFile(const std::string& fileName)
    {
        CHECK_CVIE_CALL(CvieInstance_LoadParameterFile(m_instance, fileName.c_str()));
    }
    /// Wide filename version of \ref LoadParameterFile
    void LoadParameterFile(const std::wstring& fileName)
    {
        CHECK_CVIE_CALL(CvieInstance_LoadParameterFileW(m_instance, fileName.c_str()));
    }

    /** Loads parameters from a parameter buffer.
     *  \param data [in] the parameter data to load
     *  \see CvieInstance_LoadParameterBuffer()
     */
    void LoadParameterBuffer(const std::string& data)
    {
        CHECK_CVIE_CALL(CvieInstance_LoadParameterBuffer(m_instance, data.c_str(), int(data.size())));
    }

    /** Saves parameters to a file.
     *  \param fileName [in] the full path of the file to save to. The directory must exist.
     *                       The string shall use the character encoding defined by the current system code page for the narrow
     *                       sting version. For the wide string version any Unicode characters can be used.
     *  \see CvieInstance_SaveParameterFile()
     *  \see WideFilenames
     */
    void SaveParameterFile(const std::string& fileName)
    {
        CHECK_CVIE_CALL(CvieInstance_SaveParameterFile(m_instance, fileName.c_str()));
    }
    /// Wide filename version of \ref SaveParameterFile
    void SaveParameterFile(const std::wstring& fileName)
    {
        CHECK_CVIE_CALL(CvieInstance_SaveParameterFileW(m_instance, fileName.c_str()));
    }
    
    /** Gets the number of parameter settings available in this file.
     *  \returns the number of settings
     *  \see CvieInstance_GetNumberOfSettings()
     */
    int GetNumberOfSettings() const
    {
        int nSettings = 0;
        CHECK_CVIE_CALL(CvieInstance_GetNumberOfSettings(m_instance, &nSettings));

        return nSettings;
    }

    /** Return an object for one of the parameter settings.
     * \param settingIndex [in] the setting to get
     * \returns a ParameterSetting object
     * \see CvieInstance_GetParameterSetting()
     */
    ParameterSetting GetParameterSetting(int settingIndex)
    {
        CvieParameterSetting parSetting;
        CHECK_CVIE_CALL(CvieInstance_GetParameterSetting(m_instance, settingIndex, &parSetting));

        return ParameterSetting(parSetting);
    }

    /** Gets an option value.
     * \tparam T the type of the option value
     * \param option [in] the option ID to get (see \ref InstanceOptions)
     * \returns the value of the option
     * \exception CvieException if the type \p T doesn't match the type of the option specified by \p option
     * \see CvieInstance_GetOption()
     */
    template<typename T> T GetOption(CvieOptionId option)
    {
        T value;
        GetOption(option, value);
        return value;
    }

    /** Gets an option value.
     * \tparam T the type of the option value
     * \param option [in] the option ID to get (see \ref InstanceOptions)
     * \param value [out] the value of the option
     * \exception CvieException if the type \p T doesn't match the type of the option specified by \p option
     * \see CvieInstance_GetOption()
     */
    template<typename T> void GetOption(CvieOptionId option, T& value)
    {
        CHECK_CVIE_CALL(CvieInstance_GetOption(m_instance, option, &value, CheckType<T>(option), nullptr));
    }

    /** Gets an option value.
     * \param option [in] the option ID to get (see \ref InstanceOptions)
     * \param value [out] the value of the option
     * \exception CvieException if the type of the option specified by \p option is not a C string
     * \see CvieInstance_GetOption()
     */
    void GetOption(CvieOptionId option, std::string& value)
    {
        value.resize(CheckType<std::string>(option));   // This may be a longer max size.
        CHECK_CVIE_CALL(CvieInstance_GetOption(m_instance, option, const_cast<char*>(value.data()), int(value.size()), nullptr));
        value.resize(std::strlen(value.c_str()));             // Remove spare part of buffer
    }
    /// GetOption Overload when the value is a wide string path.
    void GetOption(CvieOptionId option, std::wstring& value)
    {
        value.resize(CheckType<std::wstring>(option) / sizeof(wchar_t));  // This may be a longer max size.
        CHECK_CVIE_CALL(CvieInstance_GetOption(m_instance, option, const_cast<wchar_t*>(value.data()), int(value.size()) *  sizeof(wchar_t), nullptr));
        value.resize(std::wcslen(value.c_str()));             // Remove spare part of buffer
    }

    /** Gets the data type of an option.
     * \param option [in] the option ID to get (see \ref InstanceOptions)
     * \return the data type of the option (see ::CvieOptionType)
     * \see CvieInstance_GetOptionInfo()
     */
    CvieOptionType GetOptionType(CvieOptionId option)
    {
        CvieOptionType t;
        CHECK_CVIE_CALL(CvieInstance_GetOptionInfo(m_instance, option, &t, nullptr));

        return t;
    }

    /** Sets an option value.
     * \tparam T the type of the option value
     * \param option [in] the option ID to set (see \ref InstanceOptions)
     * \param value [in] the value of the option
     * \exception CvieException if the type \p T doesn't match the type of the option specified by \p option
     * \see CvieInstance_SetOption()
     */
    template<typename T> void SetOption(CvieOptionId option, const T& value)
    {
        CHECK_CVIE_CALL(CvieInstance_SetOption(m_instance, option, &value, CheckType<T>(option)));
    }

    /** Sets an option value.
     * \param option [in] the option ID to set (see \ref InstanceOptions)
     * \param value [in] the value of the option
     * \exception CvieException if the type of the option specified by \p option is not a C string
     * \see CvieInstance_SetOption()
     */
    void SetOption(CvieOptionId option, const std::string& value)
    {
        CheckType<std::string>(option);
        CHECK_CVIE_CALL(CvieInstance_SetOption(m_instance, option, value.c_str(), int(value.size()) + 1));    // +1 for terminating null byte
    }
    /// SetOption Overload when the value is a wide string path.
    void SetOption(CvieOptionId option, const std::wstring& value)
    {
        CheckType<std::wstring>(option);
        CHECK_CVIE_CALL(CvieInstance_SetOption(m_instance, option | CVIE_WIDE_PATH, value.c_str(), int(value.size()) + 1));    // + 1 for terminating null byte
    }


private:
    // Non-copyable
    Instance(const Instance& rhs);
    Instance& operator=(const Instance& rhs);

    // Helper method for instance SetOption/GetOption
    template<typename T> int CheckType(CvieOptionId option) { return CheckType(option, detail::OptionType<T>()); }
    int CheckType(CvieOptionId option, CvieOptionType requiredType)
    {
        CvieOptionType valueType;
        int valueSize;

        CHECK_CVIE_CALL(CvieInstance_GetOptionInfo(m_instance, option, &valueType, &valueSize));
        if (valueType != requiredType)
            // The runtime returned an option type that doesn't match the one declared in this header
            throw CvieException("Option type mismatch.");

        return valueSize;
    }

    CvieInstance m_instance;
};    // class Instance


// Versions of CvieEnter/Exit that throw exceptions on error.
inline void Enter()
{
    CHECK_CVIE_CALL(::CvieEnter(CVIE_API_VERSION));
}

inline void Exit()
{
    CHECK_CVIE_CALL(::CvieExit());
}


namespace detail {
    // Helper function for global SetOption/GetOption
    inline int CheckType(CvieOptionId option, CvieOptionType requiredType)
    {
        CvieOptionType valueType;
        int valueSize;

        CHECK_CVIE_CALL(CvieGetOptionInfo(option, &valueType, &valueSize));
        if (valueType != requiredType)
            // The runtime returned an option type that doesn't match the one declared in this header
            throw CvieException("Option type mismatch.");

        return valueSize;
    }
    template<typename T> int CheckType(CvieOptionId option) { return CheckType(option, detail::OptionType<T>()); }
}    // namespace detail

// Get option value using GetOption(ID, value) syntax.
template<typename T> void GetOption(CvieOptionId option, T& value)
{
    CHECK_CVIE_CALL(CvieGetOption(option, &value, detail::CheckType<T>(option), nullptr));
}

inline void GetOption(CvieOptionId option, std::string& value)
{
    value.resize(detail::CheckType<std::string>(option));    // This includes room for the trailing nul that CvieInstance_GetOption will write.
    CHECK_CVIE_CALL(CvieGetOption(option, const_cast<char*>(value.data()), int(value.size()), nullptr));
    value.resize(std::strlen(value.c_str()));               // Remove spare part of buffer
}

inline void GetOption(CvieOptionId option, std::wstring& value)
{
    value.resize(detail::CheckType<std::wstring>(option) / sizeof(wchar_t));    // This includes room for the trailing nul that CvieInstance_GetOption will write.
    CHECK_CVIE_CALL(CvieGetOption(option, const_cast<wchar_t*>(value.data()), int(value.size()) * sizeof(wchar_t), nullptr));
    value.resize(std::wcslen(value.c_str()));                 // Remove spare part of buffer
}

// Get option value using GetOption<T>(ID) syntax.
template<typename T> T GetOption(CvieOptionId option)    // Only specializations exist
{
    T value;
    GetOption(option, value);
    return value;
}

inline CvieOptionType GetOptionType(CvieOptionId option)
{
    CvieOptionType t;
    CHECK_CVIE_CALL(CvieGetOptionInfo(option, &t, nullptr));

    return t;
}

// Set an option
template<typename T> void SetOption(CvieOptionId option, const T& value)
{
    CHECK_CVIE_CALL(CvieSetOption(option, &value, detail::CheckType<T>(option)));
}

inline void SetOption(CvieOptionId option, const std::string& value)
{
    detail::CheckType<std::string>(option);
    CHECK_CVIE_CALL(CvieSetOption(option, value.c_str(), int(value.size()) + 1));    // +1 for terminating null byte
}

inline void SetOption(CvieOptionId option, const std::wstring& value)
{
    detail::CheckType<std::wstring>(option);
    CHECK_CVIE_CALL(CvieSetOption(option | CVIE_WIDE_PATH, value.c_str(), int(value.size()) + 1));    // + 1 for terminating null byte
}

}    // namespace Cvie


#undef CHECK_CVIE_CALL
#endif

/** \} */
