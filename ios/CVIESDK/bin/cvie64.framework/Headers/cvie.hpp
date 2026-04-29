#ifndef CVIE_HPP_HEADER_INCLUDED
#define CVIE_HPP_HEADER_INCLUDED
#pragma once

/** \defgroup CVIE_CPP CVIE++ API Reference
 *
 * \brief Modern C++ API to the CVIE library.
 *
 * This file contains C++ classes to implement an easy to use CVIE API in C++. The main classes are:
 *
 *   Worker - A Worker performs imge enhancement for one setting of one parameter file.
 *
 *   Instance - An Instance represents one hardware resource where image enhancement can be performed, typically CPU or GPU.
 *
 *   Library - A Library connects to the functions exported from one CVIE dynamic library.
 *
 * Most applications only create Worker objects as there is always a default Instance (depending on the CVIE variant this can be CPU
 * or GPU). Explicitly creating Instances is only necessary to select which GPU to use (if the computer has more than one), to run CPU mode in 
 * a GPU SDK and to limit the number of threads for CPU operation.
 *
 * Explicitly creating Library objects is only necessary when the dynamic library file has been renamed compared to the SDK zip
 * file or has been moved to another directory than the application or in PATH.
 *
 * This C++ API is implemented as a header only library which means that there are no .cpp source code files that need to be
 * compiled, all functions are inline. To keep class definitions readable the implementation of each member function is in a
 * separate section at the end of the file.
 *
 * \note All names are declared in the cvie namespace. This is distinct from the Cvie namespace used by the old CVIE2 C++ wrapper
 * classes. CVIE2 is not recommended for new integration projects.
 *
 * CONFIDENTIAL
 * \copyright Copyright (c) ContextVision AB.
 *
 \{
 \cond
*/

#include "cvie.h"
#include "dynamic_library.h"

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <cstring>
#include <array>
#include <cassert>



///\endcond

/// All of the Functionality of the new C++ API are in the cvie namespace.
namespace cvie {


/// cvie::ParameterInfo is a C++ friendly name for TCVIEDIInfo
using ParameterInfo = ::TCVIEDIInfo;

/// Element types define the data types on Worker ports.
enum class ElementType : uint32_t {
    UNDEFINED = 0,
    U8 = CVIE_DATA_U8,
    U16 = CVIE_DATA_U16,
    S16 = CVIE_DATA_S16,
    F32 = CVIE_DATA_F32,
    F64 = CVIE_DATA_F64,                ///< Used for scalar EI outputs
};

/// ColorConfiguration defines which color channels a color image has.
/// Data is always stored in pixel interleave, with all values constituting one pixel stored together before the first value of
/// the next pixel.
enum class ColorConfiguration : uint32_t {
    GREYSCALE = CVIE_GREYSCALE_CONFIGURATION,   ///< Single channel greyscale data
    RGB = CVIE_RGB_CONFIGURATION,               ///< Three values per pixel, red, green, blue in this order.
    RGBA = CVIE_RGBA_CONFIGURATION              ///< Four values per pixel, red, green, blue, alpha in this order.
};
    

/// For inputs channel defines which color channel of a color image to process. 
/// For outputs channel defines which image channel(s) to affect. If LUMINANCE is selected for an output
/// the difference betwen unprocessed and processed input data is applied to the output, while for INTENSITY the RGB output channels
/// are set to the processed result (giving a black and white image stored in a RGB buffer). The Alpha value is never changed in an
/// RGBA output image.
enum class Channel : uint32_t {
    RED = CVIE_RED_CHANNEL,
    GREEN = CVIE_GREEN_CHANNEL,
    BLUE= CVIE_BLUE_CHANNEL,
    ALPHA = CVIE_ALPHA_CHANNEL,
    LUMINANCE = CVIE_LUMINANCE_CHANNEL,     ///< RGB channels are weighted together to luminance before processing.
    INTENSITY = CVIE_INTENSITY_CHANNEL      ///< RGB channels are weighted together equally before processing.
};

/// Enum defining which type of memory CPU/GPU port data is located in.
/// Only 2D and 3D ports support GPU memory.
enum class MemoryType : uint32_t {
    CPU = 0,
};


/// Port objects describe algorithm ports and is returned by the const versions of the Worker::input and Worker::output methods
/// of the Worker class. Ports inspect the state of the port and check its capabilities.
class Port {
public:
    std::string getName() const { return m_port.name; }             ///< Name of port, set by the CVIE library.
    bool isOutput() const { return m_port.flags & CVIE_DATA_OUTPUT; }  ///< True for output ports.

    int getRank() const { return m_port.rank; }                     ///< Rank of port (scalar, vector, image or volume).
    bool isScalar() const { return getRank() == 0; }                ///< Check if the port is scalar
    bool isVector() const { return getRank() == 1; }                ///< Check if the port is one-dimensional
    bool isImage() const { return getRank() == 2; }                 ///< Check if the port is two-dimensional, representing an image or matrix.
    bool isVolume() const { return getRank() == 3; }                ///< Check if the port is three-dimensional, representing a volume.

    int getWidth() const { return getSize(0); }                     ///< Get width. For one dimensional ports this is the declared maximum count. Throws std::domain_error if port is scalar.
    int getHeight() const { return getSize(1); }                    ///< Get height. Throws std::domain_error if port is scalar or one dimensional.
    int getDepth() const { return getSize(2); }                     ///< Get depth of a volume. Throws std::domain_error if port is not a volume.
    int getSize(int dim) const;                                     ///< Get size in any dimension (Where width is #0).
    size_t getElementCount() const;                                 ///< Get number of elements in total for this port.

    size_t getElementBytes() const;                                 ///< Get the number of bytes per pixel, for all channels.
    size_t getRowBytes() const { return m_port.row_bytes; }         ///< Get the number of bytes per row. If not explicitly set data is assumed to be packed.
    size_t getPlaneBytes() const { return m_port.plane_bytes; }     ///< Get the number of bytes per plane (aka slice) of a volume. If not explicitly set data is assumed to be packed.
    size_t getTotalBytes() const;                                   ///< Get of bytes for entire data set (including any extra bytes after last row of the last plane).

    bool isAllowed(ElementType type) const;                         ///< Check if the element type is allowed on this port. Image and Volume ports often allow more than one pixel type to be used.
    ElementType getElementType() const;                             ///< Get the data type for the port. If more than one data type is allowed and none has been set by the application returns UNDEFINED.

    bool isAllowed(ColorConfiguration cc) const;                    ///< Check if the ColorConfiguration is allowed on this port.
    ColorConfiguration getColorConfiguration() const;               ///< Get the color configuration selected for this port.
    int getChannelCount() const;                                    ///< Get the number of color channels per pixel for the set ColorConfiguration.

    Channel getChannel() const { return Channel(m_port.channel); }  ///< Get the color channel selected for this port.

    float getMinValue() const { return m_port.min_value; }          ///< Get the current minimum value allowed on the port. This is only valid on some ports, see product documentation.
    float getMaxValue() const { return m_port.max_value; }          ///< Get the current maximum value allowed on the port. This is only valid on some ports, see product documentation.

    bool isPartialOutput() const;                                   ///< Return true if the output can be partially filled by running the algorithm.
    int resultCount() const { return m_port.result_count; }         ///< Get the number of valid elements produced by the last enhancement of the worker. Cleared if a new buffer is attached to the port.
 
    static int getChannelCount(ColorConfiguration cc);              ///< Get the number of color channels stored for the ColorConfiguration
    static size_t getElementTypeBytes(ElementType type);            ///< Get the number of bytes per ElementType channel.
protected:
    Port(const TCVIEPort& descr) : m_port(const_cast<TCVIEPort&>(descr)) {} ///< Only cvie::Worker can create ports.
    friend class Worker;

    TCVIEPort& m_port;  ///< internal
};


class Worker;

/// Intermediate class implementing the member functions available on non-const inputs and outputs.
class SettablePort : public Port {
public:
    /// Set pixel value range. This function is only valid on some ports, see product documentation.
    ///
    /// When used on an input the range from min to max is rescaled to the internally used 0 - 1 float range
    /// before processing starts. Values outside the range are normally clamped to 0 and 1 respectively.
    ///
    /// When set on an output the 0 - 1 result range of the processing is rescaled to the set range. Values outside the set range
    /// are normally clipped.
    ///
    /// Clipping can be disabled in the \ref termParameterSetting but for outputs clipping to the representable range for integer
    /// types is always performed, there is no wrap around.
    ///
    /// A reversed range can be used to invert the image before or after processing (or both).
    ///
    /// When a Worker is created the min and max values from the parameter setting are returned by getMinValue and getMaxValue. These values
    /// are adapted to the pixel type agreed upon in the contract. If set these values replace the values in the Setting.

    /// \note All values of all available integer types can be represented exactly in float, so no precision is lost by always
    /// setting the min and max values as floats.
    void setValueRange(float min, float max);

    /// Set the color configuration selected for this port. This controls which color channels each pixel consists of. Each color
    /// channel has a data type controlled by the attach function called.
    void setColorConfiguration(ColorConfiguration configuration);

    /// Sets channel to process for color inputs. Not applicable to single channel inputs.
    /// Sets channel to affect for color outputs. Not applicable to single channel outputs.
    /// See: \ref CVIE_ChannelSelection
    void setChannel(Channel channel) { m_port.channel = int(channel); }

    /// Attach to null pointer to indicate that no buffer is attached. As some inputs are optional this can be useful to indicate
    /// that no data is available. For Outputs it indicates that the data no longer has to be produced. For mandatory ports this 
    /// sets the Worker in a non-ready state.
    void attach(nullptr_t);

protected:
    SettablePort(TCVIEPort& descr, Worker& worker, int index = 0) : Port(descr), m_worker(worker), m_index(index) {}  ///< Only cvie::Worker can create ports.

    bool updateValue(int& var, int value);  ///< internal
    bool updateSize(int dim, int value);  ///< internal
    int toFlags(ElementType elementType, MemoryType memoryType, ColorConfiguration colorConfiguration);  ///< internal

    void attachCPU(void* data, ElementType format, int rowBytes, int planeBytes);  ///< Helper for subclasses.

    void doSetup();  ///< internal
    void checkAttach();  ///< internal

    void clearVec();  ///< internal
    template<typename T> void setVec(std::vector<T>& vec);  ///< internal

private:
    Worker& m_worker;
    int m_index;
};

///@{
/// Template function which returns the corresponding ElementType enum value for a type T given as an explicit template parameter at
/// the call site. This has the natural definition for char, unsigned char, uint8_t, unsigned short, uint16_t, short, signed short,
/// int16_t, float, double, TCVIEPoint.
/// \note While C/C++ does not define the signedness of the char type it has traditionally been associated with unsigned 8 bit data
/// in Computer Graphics and Image Processing. Therefore this function returns ElementType::U8 for the char type.
/// Compile time error for unusable types.
template <typename T, std::enable_if_t<std::is_same<std::decay_t<T>, char>::value
                                    || std::is_same<std::decay_t<T>, unsigned char>::value
                                    || std::is_same<std::decay_t<T>, uint8_t>::value, bool> = true>
constexpr ElementType ElementTypeOf() { return ElementType::U8; }

template <typename T, std::enable_if_t<std::is_same<std::decay_t<T>, unsigned short>::value
                                    || std::is_same<std::decay_t<T>, uint16_t>::value, bool> = true>
constexpr ElementType ElementTypeOf() { return ElementType::U16; }

template <typename T, std::enable_if_t<std::is_same<std::decay_t<T>, short>::value
                                    || std::is_same<std::decay_t<T>, signed short>::value
                                    || std::is_same<std::decay_t<T>, int16_t>::value, bool> = true>
constexpr ElementType ElementTypeOf() { return ElementType::S16; }

template <typename T, std::enable_if_t<std::is_same<std::decay_t<T>, float>::value, bool> = true>
constexpr ElementType ElementTypeOf() { return ElementType::F32; }

template <typename T, std::enable_if_t<std::is_same<std::decay_t<T>, double>::value, bool> = true>
constexpr ElementType ElementTypeOf() { return ElementType::F64; }
///@}

/// Inputs are used to modify Worker inputs. Inputs accept const pointers when attaching a CPU buffer.
class Input : public SettablePort {
public:
    using SettablePort::attach;

    /// Return true if the input is optional. Optional inputs that are not attached have a specific default behaviour specified by
    /// the algorithm.
    bool isOptional() const { return m_port.allowed & CVIE_DATA_OPTIONAL_INPUT; }

    /// Some inputs are assumed to be unchanged between Run calls unless attach() is called to indicate that the data may have
    /// changed. For such inputs attach must be called with the same pointer to indicate that the application has changed the
    /// _contents_ of the attached buffer.
    bool isAttachRequiredAfterContentsChange() const { return m_port.allowed & CVIE_DATA_SETUP_REQUIRED; }
    
    /// Set input size. The rank of the port must match the count. All sizes must be > 0. 
    ///
    /// Throws std::logic_error if the count does not match rank of the port.
    /// Throws std::domain_error if any element of sizes < 1.
    void setSize(int* sizes, int count);

    /// Set input size. The rank of the port must match number of parameters. All sizes must be > 0. 
    ///
    /// Throws std::logic_error if the number of sizes does not match rank of the port.
    /// Throws std::domain_error if any size < 1.
    template<typename... Szs> void setSize(Szs... sizes);

    /// Attach CPU buffer to input. The ownership of the buffer memory is not transferred. Throws std::logic_error if rowBytes or planeBytes is smaller than minimally
    /// required. The ElementType is deduced from the type of the buffer pointer.
    template<typename T> void attach(const T* data, int rowBytes = 0, int planeBytes = 0) { attach(data, ElementTypeOf<T>(), rowBytes, planeBytes); }

    /// Attach CPU buffer to input. The ownership of the buffer memory is not transferred. Throws std::logic_error if rowBytes or planeBytes is smaller than minimally
    /// required. The ElementType is explicitly given as a parameter.
    void attach(const void* data, ElementType format, int rowBytes = 0, int planeBytes = 0) { attachCPU(const_cast<void*>(data), format, rowBytes, planeBytes); }

    /// Attach a std::vector to a 1D input port. The port's size is automatically set from vector size.
    template<typename T> void attach(const std::vector<T>& vec);

private:
    // Helpers for variadic setSize. Done recursively to cater for C++11.
    template<size_t IX, typename... Szs> bool setSizeHelper(int size, Szs... sizes);
    template<size_t IX> bool setSizeHelper();       // Sentinel

    using SettablePort::SettablePort;
    friend class Worker;
};


/// Outputs are used to modify Worker outputs. Outputs accept only non-const pointers when attaching a CPU buffer.
class Output : public SettablePort {
public:
    using SettablePort::attach;

    ///@{ 
    /// Attach CPU buffer to output. The ownership of the buffer memory is not transferred. Throws std::logic_error if rowBytes or planeBytes is smaller than minimally
    /// required. The ElementType is deduced from the type of the buffer pointer.
    template<typename T> void attach(T* data, int rowBytes = 0, int planeBytes = 0) { attachCPU(data, ElementTypeOf<T>(), rowBytes, planeBytes); }
    ///@}

    ///@{ 
    /// Attach CPU buffer to output. The ownership of the buffer memory is not transferred. Throws std::logic_error if rowBytes or planeBytes is smaller than minimally
    /// required. The ElementType is explicitly given as a parameter. This overload must be used for color data as C++ does not
    /// have a type naturally corresponding to RGB o RGBA pixels.
    void attach(void* data, ElementType format, int rowBytes = 0, int planeBytes = 0) { SettablePort::attachCPU(data, format, rowBytes, planeBytes); }
    ///@}

    /// Attach a std::vector to a 1D output port. The vector is resized to the result size afer each run. As for other buffers the
    /// ownership of the vector is not transferred, the application must not destroy the vector until after the last run call on the
    /// Worker.
    template<typename T> void attach(std::vector<T>& vec);

private:
    using SettablePort::SettablePort;
    friend class Worker;
};


/// Library objects load a specified shared library file and allows setting and getting of parameters which are global to the
/// library. The Library class also holds a few flags which are used when Instances are created from it.
/// \note Applications that use implicit linking of the CVIE library don't need to create Library objects. To set flags just call
/// Library::getDefault() and set the flags on the returned object. These flags take effect when Instance objects without
/// _library_ constructor parameter are subsequently created.
/// \note Libraries have shallow copy semantics. This means that the objects can be copied freely but they all refer to the same
/// Library that was initially constructed. However, if more than one Library object is created from the same filename that is also
/// reference counted by the operating system.
class Library : private CVIEDynamicLibrary {
public:
    Library() = default;        ///< Construct an empty library object

    ///@{ 
    /// Create a library given the filename of the dynamic library (.dll or .so file). This ensures _explicit_ library loading,
    /// i.e. that the dynamic library is not loaded until this constructor is called.
    explicit Library(const std::string& libraryName) : CVIEDynamicLibrary(libraryName) {}
#ifdef WIN32
    explicit Library(const std::wstring& libraryName) : CVIEDynamicLibrary(libraryName) {}
#endif
    ///@}

    bool isOk() const { return CVIEDynamicLibrary::isOk(); }        ///< Check that the Library file was loaded and contained the entry points of the CVIE library. 

    void enableCovTrace(bool value = true) { setFlag(CVIE_CREATE_TRACE, value); }               ///< Enable encrypted Full ContextVision tracing.
    void enableCustomerTrace(bool value = true) { setFlag(CVIE_CREATE_CUSTOMER_TRACE, value); } ///< Enable clear text tracing of function calls and errors.
    void clearTraceFiles(bool value = true) { setFlag(CVIE_CREATE_CLEAR_TRACE, value); }        ///< Clear trace files before starting tracing of each program run.

    std::string getVersionInfo() const; ///< Get version information as a string

    /// Get a list of available processing devices (GPUs). For CPU only SDKs this list is empty.
    /// The returned list does not include the "CPU" alternative but it can be used for DirectX and OpenCL processing to enable CPU
    /// emulated operation. This is very slow but can be useful for testing if a GPU driver bug is suspected.
    std::vector<std::string> getDeviceList();

    /// Get the default library. If the application calls this function the implicit library loading mechanism is engaged which
    /// means that the dynamic library is loaded on program start (not when getDefault is called).
    /// \note If any of the tracing or server mode related functions needs to be called for the default Library instance just call
    /// cvie::Library::getDefault() and then call the methods on the returned object. These settings affect Instance objects
    /// subsequently created without an explicit Library constructor parameter.
    static Library& getDefault() {
        static Library library(true, 1);         // The default library loads implicitly.
        return library;
    }

private:
    int getCreateFlags() const { return m_createFlags; }

    // Note: The extra dummy parameter needed to prevent Library("quoted string") from selecting this contructor (a const char*
    // converts more easily to bool than to std::string).
    Library(bool forWorker, int dummy) : CVIEDynamicLibrary(true) {}

    friend class Instance;
    friend class Worker;

    void checkOk() const { CVIEDynamicLibrary::checkOk(); }

    void setFlag(int flag, bool value) { m_createFlags = value ? m_createFlags | flag : m_createFlags & ~flag; }
    int m_createFlags = 0;
};


/// An Instance is created for a specific GPU or for the CPU. By specifying GPU name, GPU device/command queue a certain GPU can be
/// specified. A CPU Instance using a certain number of threads is specified by giving the thread count as a constructor parameter.
/// \note Applications that only do processing on the default GPU or on the CPU do not need to create any Instance objects. Instead
/// just create Workers using constructors without any Instance parameter.
/// \note Instances have shallow copy semantics. This means that the objects can be copied freely but they all refer to the same
/// Instance that was initially constructed.
class Instance {
public:
    Instance() = default;
    
    /// Create a default processing Instance using the provided Library object to interface to the dynamic CVIE library.
    explicit Instance(const Library& library);

    ///@{
    /// Create an instance for doing CPU processing with given thread count. Use 0 to get one thread per logical CPU core.
    explicit Instance(int threads) : Instance(Library::getDefault(), threads) {}   
    Instance(const Library& library, int threads);
    ///@}

    bool isOk() const { return m_impl != nullptr && m_impl->m_handle != 0; }                          ///< Check if the Instance construction was successful.
    
    /// Get error code from Instance creation.
    ECVIE getErrorCode() const {
        if (m_impl == nullptr)
            return CVIE_E_BAD_HANDLE;
        else
            return m_impl->m_errorCode;
    }

    /// Get error after failed instance creation.
    std::string getErrorMessage() const {
        if (m_impl == nullptr)
            return "Empty cvie::Instance.";
        else
            return m_impl->m_errorMessage;
    }

    std::string getDeviceName() const;      ///< Get actual device name used for GPU Instances. Returns the empty string for CPU Instances.
    int getMemoryUsage() const;             ///< Get memory usage of this Instance in megabytes. For CPU processing this is however for all instances in this version.

    int getSettingsCount(const std::string& parFile) const;  ///< Get the number of settings available in parFile.
    int getSettingsCount(const std::wstring& parFile) const; ///< Get the number of settings available in parFile.

    const Library& getLibrary() const { checkOk(); return m_impl->m_library; }     ///< Get a reference to the Library this Instance was created for. \note The flags set in that library does not affect this instance.

    /// Return the default Instance. This instance uses GPU processing on the best GPU for GPU SDKs and CPU processing otherwise.
    static const Instance& getDefault() {
        static Instance Instance(Library::getDefault());
        return Instance;
    }

private:
    friend class Worker;         // To be able to get flags etc.
    void checkOk() const {
        if (!isOk())
            throw std::logic_error("Tried to use an Instance object which is not ok.");
    }
    void checkCreateError(ECVIE code);

    /// internal
    struct Impl {
        ~Impl() {
            if (m_handle != 0)
                m_library.destroy(m_handle);
        }

        ECVIE m_errorCode;          ///< internal
        std::string m_errorMessage; ///< internal
        HCVIE m_handle = nullptr;   ///< internal
        Library m_library;          ///< internal
    };

    std::shared_ptr<Impl> m_impl;
};


/// Worker objects perform processing with a specific \ref termParameterSetting on a specific Instannce. When a Worker has been
/// created the input and output counts are valid and the application can inquiry and set up the available inputs and outputs using
/// input() and output(). When the ports have been set up and suitable buffers have been attached to them the run() method is used
/// to process each frame.
/// \note Worker objects have move semantics. While it is possible to move a Worker object around it is not possible to get two
/// Worker objects referring to the same TCVIEWorker structure in the CVIE library. To move Worker objects the std::move() function
/// must be used, except when returning the Worker from a function.
class Worker {
public:
    Worker() = default;                 ///< Create an empty Worker which another Worker can be assigned to. A default constructed Worker is !ok.
    Worker(const Worker&) = delete;     ///< Workers can't be copied
    Worker(Worker&&);                   ///< Create a Worker by moving from another Worker, which is set to !ok.

    ///@{
    /// Create a Worker by specifying the name of a parameter file and the index of a setting in the parameter file.
    Worker(const std::string& parfile, int setting) : Worker(Instance::getDefault(), parfile, setting) {}
    Worker(const Instance& instance, const std::string& parfile, int setting);

    Worker(const std::wstring& parfile, int setting) : Worker(Instance::getDefault(), parfile, setting) {}
    Worker(const Instance& instance, const std::wstring& parfile, int setting);
    ///@}

    ///@{
    /// Create a Worker from a buffer containing an in memory image of a loaded parameter file and a setting index in that
    /// parameter file.
    Worker(const char* buffer, size_t length, int setting) : Worker(Instance::getDefault(), buffer, length, setting) {}
    Worker(const Instance& instance, const char* buffer, size_t length, int setting);
    ///@}
    

    ~Worker();      ///< Destroy the worker.

    Worker& operator=(Worker&) = delete;        ///< Workers can't be copied
    Worker& operator=(Worker&&);                ///< Assign the Worker to another Worker. If this was previously ok that worker is closed and replaced.

    bool isOk() const { return m_worker != nullptr; }   ///< Return true if the parameter setting was loaded by the constructor and the Worker has not been moved from since.

    int inputCount() const { checkOk(); return m_worker->input_count; }     ///< Return the number of Inputs supported by the loaded setting. 
    int outputCount() const { checkOk(); return m_worker->output_count; }   ///< Return the number of Outputs supported by the loaded setting. 

    ///@{
    /// Get object representing a worker input. The non-const versions returns an Input which can be used for specifying the size and
    /// other properties of the input, while the const overloads returns a Port object which can only be inquired.
    /// The input can be selected by index or by name. The name version throws an exception if the name is not found.
    /// The first input which typically contains the main input of the processing can be accessed without any parameter to input().
    Port input(int ix = 0) const { checkOk(); return { m_worker->inputs[ix] }; }
    Port input(const std::string& name) const { return input(findPort(m_worker->inputs, m_worker->input_count, name)); }
    Input input(int ix = 0) { checkOk(); return { m_worker->inputs[ix], *this }; }
    Input input(const std::string& name) { return input(findPort(m_worker->inputs, m_worker->input_count, name)); }
    ///@}

    ///@{
    /// Get object representing a worker output. The non-const versions returns an Output which can be used for attaching a buffer
    /// to the output, while the const overloads returns a Port object which can only be inquired.
    /// The output can be selected by index or by name. The name version throws an exception if the name is not found.
    /// The first output which typically contains the main result of the processing can be accessed without any parameter to output().
    Port output(int ix = 0) const { checkOk(); return { m_worker->outputs[ix] }; }
    Port output(const std::string& name) const { return output(findPort(m_worker->outputs, m_worker->output_count, name)); }
    Output output(int ix = 0) { checkOk(); return { m_worker->outputs[ix], *this, ix }; }
    Output output(const std::string& name) { return output(findPort(m_worker->outputs, m_worker->output_count, name)); }
    ///@}

    ///@name Tuning and API parameter functions
    /// More information about Tuning Parameters can be found here \ref CVIEDIParameters.
    ///@{
    ParameterInfo getParameterInfo(int parNumber) const;          ///< Get information about a Tuning or API parameter given its number.
    float getParameterValue(int parNumber) const;                 ///< Get the value of an API or Tuning parameter.
    void setParameterValue(int parNumber, float value);           ///< Set the value of an API or Tuning parameter.  
    ///@}
    bool isReady() const { return isOk() && m_errorCode == CVIE_E_OK; }     ///< Return true if all required setup is done.
    ECVIE getErrorCode() const { return m_errorCode; }                      ///< Return error code indicating the Worker status
    const std::string& getErrorMessage() const { return m_errorMessage; }   ///< Return a descriptive error messsage indicating why the Worker is not ready.

    /// Do synchronous processing. When Run returns all processing is done and the results are in the output buffers.
    ECVIE run();

    const Instance& getInstance() const { return m_instance; }              ///< Return the Instance from which this Worker was created. 
    const Library& getLibrary() const { return m_instance.getLibrary(); }   ///< Return the Library from which the Instance of this Worker was created.

    // All these functions throw std::logic_error if called when Worker is not ok.
    std::string getParameterFileDescription() const { return getStringValue(PARAMETER_DESCRIPTION); }   ///< Get metadata for the loaded parameter file
    std::string getSettingDescription() const { return getStringValue(OPERATION_DESCRIPTION); }         ///< Get metadata for the laoded parameter setting.

    ///@{
    /// SaveParameterFile creates a new parameter file which is a copy of the original parameter file apart from
    /// changes made to TP values made using setParameterValue in this worker. If other workers have changed tuning parameters of the same or
    /// other Settings in the same parameter file those changes are _not_ included in the saved parameter file. Note: When using the
    /// C function CVIESaveParameterFile all TP changes for all settings.
    ECVIE saveParameterFile(const std::string& filename);
    ECVIE saveParameterFile(const std::wstring& filename);
    ///@}

    /// Get the contents of the parameter file including tuning parameter values from the loaded parameter file or set in this
    /// Worker. Tuning parameter values set in other settings in the same parameter file are preserved as they were when the Worker
    /// was created even if changed in other Workers or by legacy functions. This requires some caution when working with multiple
    /// settings in the same parameter file simultaneously.
    std::string getParameterFileContents() const { return getStringValue(CVIE_PARAMETER_FILE); }
    /// Get the original parameter file contents. The Tuning Parameter values for _all_ settings in the parameter file are ignored,
    /// not only for the setting in this worker.
    std::string getOriginalParameterFileContents() const { return getStringValue(CVIE_ORIGINAL_PARAMETER_FILE); }
    /// Get the flattened parameter file contents. This means that the tuning parameters are applied to their respective settings
    /// in the file and then the result is returned as a fresh parameter file. This can be useful to reduce file size.
    /// \note After retrieving the flat contents further use of Tuning Parameters is not allowed.
    std::string getFlattenedParameterFileContents() const { return getStringValue(CVIE_FLATTENED_PARAMETER_FILE); }

    ///@{
    /// These functions can be periodically called to change the preset to allow it to be remote controlled from the Remote
    /// Service Tool GUI. This ensures that when reviewing settings for a certain anatomy in RST the ultrasound machine uses the
    /// appropriate scan conversion for that anatomy. When No override is in effect these functions return empty strings.
    std::string getRSSOverrideAnatomy() const;
    std::string getRSSOverrideLevel() const;
    std::string getRSSOverrideFilename() const;
    int getRSSOVerrideSettingIndex() const;
    ///@}


private:
    friend class SettablePort;   // For doSetup, checkAttach

    static int findPort(const TCVIEPort* ports, int count, const std::string& name);
    void checkOk() const;     // Throw std::logic_error if not ok.
    void checkError(ECVIE code) const;
    std::string getStringValue(int parNumber) const;
    void updateErrorMessage(ECVIE code);
    void updateCreateErrorMessage(ECVIE code);

    void doSetup();
    void checkAttach();

    Instance m_instance;                     // Shared ownership of Instance, in case Workers outlive the Instance object sent to ctor.

    CVIEWorker m_worker = nullptr;

    ECVIE m_errorCode;
    std::string m_errorMessage;

    struct VectorResizer {
        void* vec = nullptr;       ///< internal
        void(*func)(void*, int);   ///< internal
    };

    std::array<VectorResizer, CVIE_MAX_PORTS> m_resizers;    // Allocated statically even if seldom used.
};


//////////////// Port methods ////////////////

inline int Port::getSize(int dim) const 
{
    if (dim >= getRank())
        throw std::domain_error("Dimension out of range");

    return m_port.size[dim];
}


inline size_t Port::getElementCount() const 
{
    size_t ret = 1;
    for (int d = 0; d < m_port.rank; d++)
        ret *= m_port.size[d];

    return ret;
}


inline size_t Port::getElementBytes() const
{
    return getChannelCount() * getElementTypeBytes(getElementType());
}


inline size_t Port::getTotalBytes() const
{
    int from;
    size_t ret;
    if (m_port.plane_bytes > 0) {
        from = 2;
        ret = m_port.plane_bytes;
    }
    else if (m_port.row_bytes > 0) {
        from = 1;
        ret = m_port.row_bytes;
    }
    else {
        from = 0;
        ret = getElementBytes();
    }

    for (int d = from; d < m_port.rank; d++)
        ret *= m_port.size[d];

    return ret;
}

inline ElementType Port::getElementType() const
{
    return ElementType(m_port.flags & CVIE_ELEMENT_TYPE_MASK);
}


inline bool Port::isAllowed(ColorConfiguration cc) const
{
    return int(cc) <= (m_port.allowed & CVIE_COLOR_CONFIGURATION_MASK);
}    


inline ColorConfiguration Port::getColorConfiguration() const
{
    return ColorConfiguration(m_port.flags & CVIE_COLOR_CONFIGURATION_MASK);
}


inline int Port::getChannelCount() const
{
    return getChannelCount(getColorConfiguration());
}


inline bool Port::isAllowed(ElementType type) const
{
    int t = int(type);

    if ((t & CVIE_PIXEL_TYPE_MASK) != 0)
        return (t & m_port.allowed) != 0;
    else
        return (t & CVIE_DATA_TYPE_MASK) == (m_port.allowed & CVIE_DATA_TYPE_MASK);
}



inline bool Port::isPartialOutput() const
{
    return (m_port.allowed & CVIE_DATA_PARTIAL_OUTPUT) != 0;
}


inline int Port::getChannelCount(ColorConfiguration cc)
{
    switch (cc) {
    case ColorConfiguration::GREYSCALE:
        return 1;
    case ColorConfiguration::RGB:
        return 3;
    case ColorConfiguration::RGBA:
        return 4;
    default:
        assert(false);
        return 0;
    }
}


inline size_t Port::getElementTypeBytes(ElementType type)
{
    switch (type) {
        case ElementType::U8:
            return 1;
            
        case ElementType::U16:
        case ElementType::S16:
            return 2;

        case ElementType::F32:
            return 4;

        case ElementType::F64:
            return 8;

        default:
            return 0;
    }
}

//////////////// SettablePort methods ////////////////

inline int SettablePort::toFlags(ElementType elementType, MemoryType memoryType, ColorConfiguration colorConfiguration)
{
    return int(elementType) | int(memoryType) | int(colorConfiguration);
}


inline void cvie::SettablePort::checkAttach() 
{ 
    m_worker.checkAttach(); 
}


inline void SettablePort::setValueRange(float min, float max) 
{
    m_port.min_value = min;
    m_port.max_value = max;
    doSetup();
}


inline void SettablePort::setColorConfiguration(ColorConfiguration configuration)
{
    if (updateValue(m_port.flags, (m_port.flags & ~CVIE_COLOR_CONFIGURATION_MASK) | int(configuration)))
        doSetup();
}


inline void SettablePort::attach(nullptr_t)
{
    bool any = m_port.data != nullptr;
    m_port.data = nullptr; 
    if (any) {
        if ((m_port.allowed & CVIE_DATA_SETUP_REQUIRED) != 0)
            doSetup();
        else
            checkAttach();
    }
}


inline void SettablePort::doSetup() 
{
    m_worker.doSetup();
}


inline void SettablePort::clearVec() 
{
    m_worker.m_resizers[m_index].vec = nullptr;
}



inline bool SettablePort::updateValue(int& var, int value) 
{
    if (var != value) {
        var = value;
        return true;
    }

    return false;
}


inline bool SettablePort::updateSize(int dim, int value)
{
    if (value < 1)
        throw std::domain_error("Sizes must be positive");

    return updateValue(m_port.size[dim], value);
}




inline void SettablePort::attachCPU(void* data, ElementType format, int rowBytes, int planeBytes) 
{
    if (data == nullptr)
        throw std::logic_error("You can't use nulled pointer variable to undo attach, only nullptr");

    m_port.data = data;
    m_port.result_count = 0;                // No valid data available as buffer changed.

    clearVec();                             // Make sure to not try to resize any vector that was previously attached to this port.

    m_port.row_bytes = rowBytes;            // No need to redo setup when stride changes.
    m_port.plane_bytes = planeBytes;

    int flags = toFlags(format, MemoryType::CPU, getColorConfiguration());
    bool any = updateValue(m_port.flags, flags);
    if (any || (m_port.allowed & CVIE_DATA_SETUP_REQUIRED) != 0)
        doSetup();        // Always do Setup after attach if stride or pixel format changed, and when the input always requires this.
    else
        checkAttach();
}


template<typename T> void SettablePort::setVec(std::vector<T>& vec) 
{
    m_worker.m_resizers[m_index].vec = &vec;
    m_worker.m_resizers[m_index].func = [](void* vec, int size) {
        reinterpret_cast<std::vector<T>*>(vec)->resize(size);
    };
}

//////////////// Input methods ////////////////

inline void Input::setSize(int* sizes, int count)
{
    if (count != m_port.rank)
        throw std::logic_error("setSize count does not match input rank");

    bool any = false;
    for (int i = 0; i < count; i++)
        any |= updateSize(i, sizes[i]);

    if (any) {
        m_port.data = nullptr;
        m_port.flags = 0;   // forget the data type too
        clearVec();
        doSetup();
    }
}


template<size_t IX> bool Input::setSizeHelper() { return false; }
template<size_t IX, typename... Szs> bool Input::setSizeHelper(int size, Szs... sizes) 
{
    return updateSize(IX, size) | setSizeHelper<IX + 1>(sizes...);
}


template<typename... Szs> void Input::setSize(Szs... sizes) 
{
    if (sizeof...(sizes) != m_port.rank)
        throw std::logic_error("Wrong number of parameters to setSize");

    if (setSizeHelper<0>(sizes...)) {
        m_port.data = nullptr;
        m_port.flags = 0;   // forget the data type too
        doSetup();
    }
}


template<typename T> void Input::attach(const std::vector<T>& vec)
{
    if (getRank() != 1)
        throw std::logic_error("std::vector<T> can only be attached to one dimensional ports");

    bool any = updateValue(m_port.flags, toFlags(ElementTypeOf<T>(), MemoryType::CPU, getColorConfiguration()));
    any |= updateSize(0, vec.size() / getChannelCount());           // Set input size according to vector size
    m_port.data = vec.data();                   // Set data pointer

    if (any)
        doSetup();
    else
        checkAttach();
}


//////////////// Output methods ////////////////

template<typename T> void Output::attach(std::vector<T>& vec)
{
    if (getRank() != 1)
        throw std::logic_error("std::vector<T> can only be attached to one dimensional ports");

    bool any = updateValue(m_port.flags, toFlags(ElementTypeOf<T>(), MemoryType::CPU, getColorConfiguration()));
    vec.reserve(m_port.size[0] * getChannelCount());                // Setup size once to make sure memory is allocated
    vec.clear();
    m_port.data = vec.data();                   // Set data pointer after reserve
    m_port.result_count = 0;                    // No valid data available as buffer changed.

    // Install a function which can resize the vector back after run.
    setVec(vec);

    if (any)
        doSetup();
    else
        checkAttach();
}



//////////////// Library methods ////////////////

inline std::string Library::getVersionInfo() const
{
    checkOk();
    std::string val;
    ECVIE code = getParameterValue(nullptr, 0, CVIE_VERSION_INFO, val);
    if (code != CVIE_E_OK)
        throw std::logic_error("Error while getting version info: " + getLastError(nullptr));
    return val;
}

inline std::vector<std::string> Library::getDeviceList()
{
    std::vector<std::string> ret;

    return ret;
}


//////////////// Instance methods ////////////////

inline Instance::Instance(const Library& library) : m_impl(std::make_shared<Impl>())
{
    library.checkOk();
    m_impl->m_library = library;
    checkCreateError(library.create(m_impl->m_handle, library.getCreateFlags()));
}


inline Instance::Instance(const Library& library, int threads) : m_impl(std::make_shared<Impl>())
{
    library.checkOk();
    m_impl->m_library = library;
    checkCreateError(library.create(m_impl->m_handle, CVIE_CREATE_CPU | library.getCreateFlags()));
    if (isOk())
        checkCreateError(m_impl->m_library.setThreads(m_impl->m_handle, threads));
}





inline std::string Instance::getDeviceName() const
{
    std::string ret;
    return ret;
}

inline int Instance::getMemoryUsage() const
{
    int ret;
    if (getLibrary().getParameterValue(m_impl->m_handle, 0, CVIE_MEMORY_USAGE, ret) != CVIE_E_OK)
        throw std::logic_error("Error from CVIE: " + getLibrary().getLastError(m_impl->m_handle));

    return ret;
}

inline int Instance::getSettingsCount(const std::string& parFile) const
{
    checkOk();
    int numSettings;
    getLibrary().setParameterFile(m_impl->m_handle, parFile.c_str(), numSettings);
    return numSettings;
}

inline int Instance::getSettingsCount(const std::wstring& parFile) const
{
    checkOk();
    int numSettings;
    getLibrary().setParameterFile(m_impl->m_handle, parFile.c_str(), numSettings);
    return numSettings;
};


inline void Instance::checkCreateError(ECVIE error) 
{
    m_impl->m_errorCode = error;
    if (error != CVIE_E_OK)
        m_impl->m_errorMessage = m_impl->m_library.getLastError(m_impl->m_handle);
}

//////////////// Worker methods ////////////////

inline Worker::Worker(const Instance& instance, const std::string& parfile, int setting) : m_instance(instance)
{
    instance.checkOk();
    updateCreateErrorMessage(getLibrary().createWorker(m_worker, m_instance.m_impl->m_handle, parfile, setting));
}


inline Worker::Worker(const Instance& instance, const std::wstring& parfile, int setting) : m_instance(instance)
{
    instance.checkOk();
    updateCreateErrorMessage(getLibrary().createWorker(m_worker, m_instance.m_impl->m_handle, parfile, setting));
}


inline Worker::Worker(const Instance& instance, const char* buffer, size_t length, int setting) : m_instance(instance)
{
    instance.checkOk();
    updateCreateErrorMessage(getLibrary().createWorkerFromBuffer(m_worker, m_instance.m_impl->m_handle, buffer, int(length), setting));
}


inline Worker::Worker(Worker&& src) : m_instance(src.m_instance), m_worker(src.m_worker), m_errorCode(src.m_errorCode), m_errorMessage(std::move(src.m_errorMessage))
{
    src.m_worker = nullptr;
    src.m_errorCode = CVIE_E_BAD_HANDLE;        // Moved from state.
    src.m_errorMessage = "Worker has been moved from";
}


inline Worker::~Worker()
{
    if (m_worker != nullptr)
        getLibrary().destroyWorker(m_worker);
}


inline Worker& Worker::operator=(Worker&& src)
{
    if (this == &src)
        return *this;
    
    if (m_worker != nullptr)        // If I was active before, destroy that worker handle.
        getLibrary().destroyWorker(m_worker);
    
    m_instance = src.m_instance;
    m_worker = src.m_worker;
    m_errorCode = src.m_errorCode;
    m_errorMessage = std::move(src.m_errorMessage);
    m_resizers = src.m_resizers;

    src.m_worker = nullptr;
    src.m_errorCode = CVIE_E_BAD_HANDLE;        // Moved from state.
    src.m_errorMessage = "Worker has been moved from";
    return *this;
}


inline ParameterInfo Worker::getParameterInfo(int parNumber) const
{
    checkOk();

    ParameterInfo ret;
    checkError(getLibrary().getDIInfo(m_worker->handle, m_worker->id, parNumber, ret));
    return ret;
}


inline float Worker::getParameterValue(int parNumber) const
{
    checkOk();

    float ret;
    checkError(getLibrary().getParameterValue(m_worker->handle, m_worker->id, parNumber, ret));
    return ret;
}


inline void Worker::setParameterValue(int parNumber, float value)
{
    checkOk();
    checkError(getLibrary().setParameterValue(m_worker->handle, m_worker->id, parNumber, value));
}


inline ECVIE Worker::run()
{
    checkOk();

    if (!isReady())
        throw std::logic_error("Worker not ready when run was called.");

    // Resize output ports represented by vectors before running
    for (int o = 0; o < m_worker->output_count; o++) {
        if (m_resizers[o].vec != nullptr)
            m_resizers[o].func(m_resizers[o].vec, m_worker->outputs[o].size[0]);
    }

    updateErrorMessage(getLibrary().run(m_worker));

    if (m_errorCode == CVIE_E_OK) {
        // Update vector lengths for such outputs.
        for (int o = 0; o < m_worker->output_count; o++) {
            if (m_resizers[o].vec != nullptr)
                m_resizers[o].func(m_resizers[o].vec, m_worker->outputs[o].result_count);
        }
    }

    return m_errorCode;
}


inline ECVIE Worker::saveParameterFile(const std::string& filename)
{
    checkOk();

    return getLibrary().saveWorkerParameterFile(m_worker, filename);
}


inline ECVIE Worker::saveParameterFile(const std::wstring& filename)
{
    checkOk();

    return getLibrary().saveWorkerParameterFile(m_worker, filename);
}

inline std::string Worker::getRSSOverrideAnatomy() const
{
    std::string rss = getStringValue(CVIE_RSS_IDENTIFICATION_ANATOMY_LEVEL);
    size_t pos = rss.find(':');
    if (pos == std::string::npos)
        return "";

    return rss.substr(0, pos);
}


inline std::string Worker::getRSSOverrideLevel() const
{
    std::string rss = getStringValue(CVIE_RSS_IDENTIFICATION_ANATOMY_LEVEL);
    size_t pos = rss.find(':');
    if (pos == std::string::npos)
        return "";

    return rss.substr(pos + 1);
}


inline std::string Worker::getRSSOverrideFilename() const
{
    std::string rss = getStringValue(CVIE_RSS_IDENTIFICATION_FILENAME_SETTING);
    size_t pos = rss.find(':');
    if (pos == std::string::npos)
        return "";

    return rss.substr(0, pos);
}


inline int Worker::getRSSOVerrideSettingIndex() const
{
    std::string rss = getStringValue(CVIE_RSS_IDENTIFICATION_FILENAME_SETTING);
    size_t pos = rss.find(':');
    if (pos == std::string::npos)
        return -1;

    return std::atoi(rss.c_str() + pos + 1);
}


inline int Worker::findPort(const TCVIEPort* ports, int count, const std::string& name)
{
    for (int i = 0; i < count; i++)
        if (name.compare(ports[i].name) == 0)
            return i;

    throw std::logic_error("Port name not found: " + name);
}


inline void Worker::checkOk() const {     // Throw std::logic_error if not ok.
    if (!isOk())
        throw std::logic_error("Can't do this if Worker is not ok.");
}


inline void Worker::checkError(ECVIE code) const
{
    if (code != CVIE_E_OK)
        throw std::logic_error("Error from CVIE: " + getLibrary().getLastError(getInstance().m_impl->m_handle));
}


inline std::string Worker::getStringValue(int parNumber) const
{
    checkOk();

    std::string ret;
    checkError(getLibrary().getParameterValue(m_worker->handle, m_worker->id, parNumber, ret));
    return ret;
}


inline void Worker::updateErrorMessage(ECVIE code)
{
    m_errorCode = code;
    if (code != CVIE_E_OK)
        m_errorMessage = getLibrary().getLastError(getInstance().m_impl->m_handle);  // Note: for constructor problems m_worker can be nullptr.
    else
        checkAttach();
}


inline void Worker::updateCreateErrorMessage(ECVIE code)
{
    if (code != CVIE_E_OK)
        updateErrorMessage(code);
    else {
        m_errorCode = CVIE_E_SETTING_NOT_READY;
        m_errorMessage = "No size set";
    }        
}


inline void Worker::doSetup()
{
    checkOk();
    updateErrorMessage(getLibrary().setup(m_worker));
}

inline void Worker::checkAttach()
{
    static const char* errorMsg = "Not all mandatory ports are attached.";

    // Only do this if there were no more serious errors.
    if (m_errorCode == CVIE_E_OK || m_errorMessage == errorMsg) {
        // Check that all mandatory inputs are attached
        bool ok = true;
        for (int i = 0; i < m_worker->input_count; i++) {
            auto& in = m_worker->inputs[i];
            if (in.data == nullptr && (in.allowed & CVIE_DATA_OPTIONAL_INPUT) == 0) {
                ok = false;
                break;
            }
        }
        if (ok) {
            // Also check that at least one output is attached.
            for (int o = 0; o < m_worker->output_count; o++) {
                auto& out = m_worker->outputs[o];
                if (out.data != nullptr) {
                    m_errorCode = CVIE_E_OK;        // Found an attached output.
                    m_errorMessage.clear();
                    return;
                }
            }
        }

        m_errorCode = CVIE_E_INVALID_INPUT;
        m_errorMessage = errorMsg;
    }
}


}   // namespace cvie


/// \}

#endif // Include guard
