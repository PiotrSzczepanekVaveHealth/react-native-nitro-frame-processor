package se.contextvision.cvie;

import com.sun.jna.Callback;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Platform;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;
import java.nio.IntBuffer;
import java.nio.ByteBuffer;

/**
 * JNA Wrapper for <b>CVIE2</b><br>
 *
 * This is a simple Java native wrapper of the \ref CVIE2 C API. It is possible, but not recommended,
 * to use this class directly to interact with the CVIE library.  Instead, the Cvie2 class,
 * which provides a higher-level Java API is recommended.
 *
 * \ingroup CVIE2_java
 *
 * \see CVIE2
 *
 */
public class Cvie2Library implements Library {
    /// \cond
    public static final String JNA_LIBRARY_NAME = Platform.is64Bit() ? "cvie64" : "cvie";
    public static final NativeLibrary JNA_NATIVE_LIB = NativeLibrary.getInstance(Cvie2Library.JNA_LIBRARY_NAME);
    static {
        Native.register(Cvie2Library.class, Cvie2Library.JNA_NATIVE_LIB);
    }
    public static final int CVIE_RESULT_OK = (int)0;
    public static final int CVIE_RESULT_BAD_HANDLE = (int)3;
    public static final int CVIE_RESULT_USER_ABORTED = (int)4;
    public static final int CVIE_RESULT_FILEIO_ERROR = (int)6;
    public static final int CVIE_RESULT_ILLEGAL_COMMAND = (int)7;
    public static final int CVIE_RESULT_INVALID_INPUT = (int)8;
    public static final int CVIE_RESULT_INVALID_PARAMETER_FILE = (int)9;
    public static final int CVIE_RESULT_LICENSE_ERROR = (int)10;
    public static final int CVIE_RESULT_NOT_SUPPORTED = (int)11;
    public static final int CVIE_RESULT_OUT_OF_MEMORY = (int)16;
    public static final int CVIE_RESULT_FATAL_ERROR = (int)5;
    public static final int CVIE_RESULT_UNKNOWN_ERROR = (int)15;
    public static final int CVIE_RESULT_GPU_ERROR = (int)17;
    public static final int CVIE_PIXEL_FORMAT_U8 = (int)1;
    public static final int CVIE_PIXEL_FORMAT_U16 = (int)2;
    public static final int CVIE_PIXEL_FORMAT_S16 = (int)3;
    public static final int CVIE_PIXEL_FORMAT_F32 = (int)4;
    public static final int CVIE_OPTION_TYPE_INT32 = (int)1;
    public static final int CVIE_OPTION_TYPE_FLOAT = (int)2;
    public static final int CVIE_OPTION_TYPE_CSTR = (int)3;
    public static final int CVIE_OPTION_TYPE_IMAGE_DESCRIPTOR = (int)7;
    public static final int CVIE_OPTION_CSTR_LICENSE_ID = (int)2;
    public static final int CVIE_OPTION_INT32_TRACE_ENABLE = (int)5;
    public static final int CVIE_INSTANCE_OPTION_CSTR_PARAMETER_DESCRIPTION = (int)101;
    public static final int CVIE_INSTANCE_OPTION_INT32_NUM_THREADS = (int)210;
    public static final int CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_X = (int)208;
    public static final int CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_Y = (int)209;
    public static final int CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START = (int)214;
    public static final int CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END = (int)215;
    public static final int CVIE_SETTING_OPTION_INT32_NUM_THREADS = (int)210;

    public static final int CVIE_SETTING_OPTION_IMAGE_DESCRIPTOR_MASK = (int)213;

    public static final int CVIE_API_VERSION = (int)2007;
    public static final int CVIE_INSTANCE_OPTION_CSTR_PARAMETER_FILE = (int)116;
    public static final int CVIE_INSTANCE_OPTION_CSTR_ORIGINAL_PARAMETER_FILE = (int)117;
    public static final int CVIE_INSTANCE_OPTION_CSTR_FLATTENED_PARAMETER_FILE = (int)118;
    public static final int CVIE_INSTANCE_OPTION_CSTR_RSS_IDENTIFICATION_ANATOMY_LEVEL = (int)2000;
    public static final int CVIE_INSTANCE_OPTION_CSTR_RSS_IDENTIFICATION_FILENAME_SETTING = (int)2001;

    public static native int CvieEnter(int apiVersion);
    public static native int CvieExit();
    public static native String CvieGetLastError();
    public static native int CvieGetOption(int option, ByteBuffer valueBuffer, int valueBufferSize, IntByReference valueSize);
    public static native int CvieGetOptionInfo(int option, IntByReference valueType, IntByReference valueSize);
    public static native int CvieSetOption(int option, ByteBuffer valueBuffer, int valueBufferSize);
    public static native int CvieInstance_Create(PointerByReference instance);
    public static native int CvieInstance_Destroy(PointerByReference instance);
    public static native int CvieInstance_LoadParameterFile(Cvie2Library.CvieInstance instance, String fileName);
    public static native int CvieInstance_LoadParameterBuffer(Cvie2Library.CvieInstance instance, ByteBuffer data, int dataSize);
    public static native int CvieInstance_GetNumberOfSettings(Cvie2Library.CvieInstance instance, IntByReference nSettings);
    public static native int CvieInstance_GetParameterSetting(Cvie2Library.CvieInstance instance, int settingIndex, PointerByReference parameterSetting);
    public static native int CvieInstance_GetOption(Cvie2Library.CvieInstance instance, int option, ByteBuffer valueBuffer, int valueBufferSize, IntByReference valueSize);
    public static native int CvieInstance_GetOptionInfo(Cvie2Library.CvieInstance instance, int option, IntByReference valueType, IntByReference valueSize);
    public static native int CvieInstance_SetOption(Cvie2Library.CvieInstance instance, int option, ByteBuffer valueBuffer, int valueBufferSize);
    public static native int CvieInstance_ReleaseMemory(Cvie2Library.CvieInstance instance);
    public static native int CvieParameterSetting_Setup(Cvie2Library.CvieParameterSetting setting, Cvie2Library.CvieImageDescriptor input, Cvie2Library.CvieImageDescriptor output);
    public static native int CvieParameterSetting_Enhance(Cvie2Library.CvieParameterSetting setting);
    public static native int CvieParameterSetting_GetOption(Cvie2Library.CvieParameterSetting setting, int option, ByteBuffer valueBuffer, int valueBufferSize, IntByReference valueSize);
    public static native int CvieParameterSetting_GetOptionInfo(Cvie2Library.CvieParameterSetting setting, int option, IntByReference valueType, IntByReference valueSize);
    public static native int CvieParameterSetting_SetOption(Cvie2Library.CvieParameterSetting setting, int option, ByteBuffer valueBuffer, int valueBufferSize);
    public static native int CvieImageDescriptor_CreateForBuffer2D(PointerByReference desc, int width, int height, int format);
    public static native int CvieImageDescriptor_AttachBuffer2D(Cvie2Library.CvieImageDescriptor desc, ByteBuffer data, int rowStrideInBytes);
    public static native int CvieImageDescriptor_CreateForBuffer3D(PointerByReference desc, int width, int height, int depth, int format);
    public static native int CvieImageDescriptor_AttachBuffer3D(Cvie2Library.CvieImageDescriptor desc, ByteBuffer data, int rowStrideInBytes, int sliceStrideInBytes);
    public static native int CvieImageDescriptor_Detach(Cvie2Library.CvieImageDescriptor desc);
    public static native int CvieImageDescriptor_Destroy(PointerByReference desc);
    public static native int CvieInstance_SaveParameterFile(Cvie2Library.CvieInstance instance, String fileName);
    public static native int CvieParameterSetting_GetDIInfo(Cvie2Library.CvieParameterSetting setting, int option, CvieTCVIEDIInfo.ByReference info);

    public static class CvieParameterSetting extends PointerType {
        public CvieParameterSetting(Pointer address) {
            super(address);
        }
        public CvieParameterSetting() {
            super();
        }
    };

    public static class CvieImageDescriptor extends PointerType {
        public CvieImageDescriptor(Pointer address) {
            super(address);
        }
        public CvieImageDescriptor() {
            super();
        }
    };

    public static class CvieInstance extends PointerType {
        public CvieInstance(Pointer address) {
            super(address);
        }
        public CvieInstance() {
            super();
        }
    };
    /// \endcond
}
