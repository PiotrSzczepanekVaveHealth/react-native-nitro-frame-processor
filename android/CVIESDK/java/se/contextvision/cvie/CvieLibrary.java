package se.contextvision.cvie;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Platform;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;

/**
 * JNA Wrapper for <b>CVIE</b><br>
 *
 * This is a simple Java native wrapper of the \ref CVIE C API. It is possible, but not recommended,
 * to use this class directly to interact with the CVIE library.  Instead, the Cvie class,
 * which provides a higher-level Java API is recommended.
 *
 * \see ImageEnhancement
 *
 * \ingroup CVIE_java
 *
 */
public class CvieLibrary implements Library {
    /// \cond
    public static final String JNA_LIBRARY_NAME = Platform.is64Bit() ? "cvie64" : "cvie";
    public static final NativeLibrary JNA_NATIVE_LIB = NativeLibrary.getInstance(CvieLibrary.JNA_LIBRARY_NAME);
    static {
        Native.register(CvieLibrary.class, CvieLibrary.JNA_NATIVE_LIB);
    }
    public static final int CVIE_DATA_U16 = (int)0x00000001;
    public static final int CVIE_DATA_S16 = (int)0x00000002;
    public static final int CVIE_DATA_U8 = (int)0x00000004;
    public static final int CVIE_DATA_F32 = (int)0x00000008;

    public static final int CVIE_CREATE_DEFAULT = (int)0;
    public static final int CVIE_CREATE_TRACE = (int)16;
    public static final int CVIE_PARTYPE_INT = (int)1;
    public static final int CVIE_PARTYPE_FLOAT = (int)2;
    public static final int CVIE_PARTYPE_STRING = (int)3;
    public static final int CVIE_PARTYPE_STRUCT = (int)8;
    public static final int CVIE_PARTYPE_BOOL = (int)9;
    public static final int CVIE_PARTYPE_MASK = (int)10;
    public static final int CVIE_PARTYPE_DOUBLE = (int)11;

    public static final int RESERVED_LICERROR_HANDLE = (int)43;
    public static final int CVIE_E_OK = (int)0;
    public static final int CVIE_E_NOT_OK = (int)1;
    public static final int CVIE_E_CREATE_HANDLE = (int)2;
    public static final int CVIE_E_BAD_HANDLE = (int)3;
    public static final int CVIE_E_USER_ABORTED = (int)4;
    public static final int CVIE_E_FATAL_ERROR = (int)5;
    public static final int CVIE_E_FILEIO_ERROR = (int)6;
    public static final int CVIE_E_ILLEGAL_COMMAND = (int)7;
    public static final int CVIE_E_INVALID_INPUT = (int)8;
    public static final int CVIE_E_INVALID_PARAMETER_FILE = (int)9;
    public static final int CVIE_E_LICENSE_ERROR = (int)10;
    public static final int CVIE_E_NOT_SUPPORTED = (int)11;
    public static final int CVIE_E_SETTING_NOT_READY = (int)12;
    public static final int CVIE_E_UNKNOWN_ERROR = (int)15;
    public static final int CVIE_E_OUT_OF_MEMORY = (int)16;

    public static final int MAX_ERROR_MESSAGE_LENGTH = (int)512;
    public static final int MAX_DESCRIPTION_LENGTH = (int)512;

    public static final int PARAMETER_DESCRIPTION = (int)1;
    public static final int OPERATION_DESCRIPTION = (int)2;
    public static final int CVIE_CUSTOMER_SETTING_DESCRIPTION = (int)10;
    public static final int CVIE_SETTING_TYPE = (int)9;
    public static final int CVIE_TRACE_MODE = (int)11;

    public static final int RELATIVE_RESOLUTION_X = (int)12;
    public static final int RELATIVE_RESOLUTION_Y = (int)13;
    public static final int DEPTH_DEPENDENCE_TRANSITION_START = (int)23;
    public static final int DEPTH_DEPENDENCE_TRANSITION_END = (int)24;

    public static final int CVIE_PARAMETER_FILE = (int)30;
    public static final int CVIE_ORIGINAL_PARAMETER_FILE = (int)31;
    public static final int CVIE_FLATTENED_PARAMETER_FILE = (int)32;
    public static final int CVIE_TRACE_PATH = (int)33;
    public static final int CVIE_VERSION_INFO = (int)40;
    public static final int CVIE_MEMORY_LIMIT = (int)213;
    public static final int CVIE_MEMORY_USAGE = (int)212;
    public static final int CVIE_PARAMETER_FILE_NAME = (int)700;
    public static final int CVIE_PARAMETER_FILE_SETTING_INDEX = (int)701;


    public static final int CVIE_RSS_IDENTIFICATION_ANATOMY_LEVEL = (int)2000;
    public static final int CVIE_RSS_IDENTIFICATION_FILENAME_SETTING = (int)2001;

    public static native int CVIECreate(PointerByReference handle, int flags);
    public static native int CVIESetParameterFile(HCVIE handle, String fileName, IntByReference settings);
    public static native int CVIESetParameterBuffer(HCVIE handle, ByteBuffer buffer, IntByReference settings);
    public static native int CVIEEnhanceSetup(HCVIE handle, int width, int height, int flags, int setting, ByteBuffer mask);
    public static native int CVIEEnhanceNext(HCVIE handle, ByteBuffer inImage, ByteBuffer outImage, int setting);
    public static native int CVIEDestroy(PointerByReference handle);
    public static native int CVIESetThreads(HCVIE handle, int threads);
    public static native int CVIEGetParameterValue(HCVIE handle, int setting, int parameter, ByteBuffer value);
    public static native int CVIESetParameterValue(HCVIE handle, int setting, int parameter, ByteBuffer value);

    public static native String CVEMGetLastError(HCVIE handle, ByteBuffer errorString, int length);

    public static native int CVIESaveParameterFile(HCVIE handle, String filename);
    public static native int CVIEGetParameterInfo(HCVIE handle, int setting, int parameter, IntByReference type, IntByReference length);
    public static native int CVIEGetDIInfo(HCVIE handle, int setting, int parameter, CvieTCVIEDIInfo.ByReference info);

    public static class HCVIE extends PointerType {
        public HCVIE(Pointer address) {
            super(address);
        }
        public HCVIE() {
            super();
        }
    };

    /// \endcond
}
