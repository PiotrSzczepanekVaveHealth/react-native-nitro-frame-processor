package se.contextvision.cvie;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import static se.contextvision.cvie.CvieLibrary.CVIE_E_OK;

/**
 * \defgroup CVIE_java CVIE Java API
 *
 * These classes give access to the CVIE C API.  The high level interface, which is recommended,
 * is available in the Cvie class, and the low level JNA wrapper is available in the CvieLibrary
 * class.
 *
 * \see CVIE
 */

/**
 * This class wraps the CVIE library. Use it to interact with the CVIE API (\ref
 * CVIE) in a high level manner.
 *
 * All functions in this class will throw CvieError() if there is any error
 * reported from the native call. These errors should be caught and handled
 * appropriately.
 *
 * For more in-depth documentation of the various functions, see the C API
 * documentation (\ref CVIE).
 *
 * \ingroup CVIE_java
 */
public class Cvie {
    private static void CheckResult(int result) {
        if (result != CVIE_E_OK) {
            throw new CvieError(result);
        }
    }
    private static void CheckResult(CvieLibrary.HCVIE handle, int result) {
        if (result != CVIE_E_OK) {
            throw new CvieError(result, CVEMGetLastError(handle));
        }
    }

    /**
     * Creates a new CVIE instance.
     *
     * \see ::CVIECreate()
     *
     * @param cvieCreateFlags flags for creation, should be a combination of values
     *                        from CvieCreateFlag
     * @return the newly created instance
     */
    public static CvieLibrary.HCVIE CVIECreate(int cvieCreateFlags) {
        PointerByReference handleRef = new PointerByReference();
        CheckResult(CvieLibrary.CVIECreate(handleRef, cvieCreateFlags));
        return new CvieLibrary.HCVIE(handleRef.getValue());
    }


    /**
     * Destroys a given CVIE instance.
     *
     * \see ::CVIEDestroy()
     *
     * @param handle the instance to destroy
     */
    public static void CVIEDestroy(CvieLibrary.HCVIE handle) {
        Pointer pointer = handle == null ? null : handle.getPointer();
        CheckResult(CvieLibrary.CVIEDestroy(new PointerByReference(pointer)));
    }

    /**
     * Sets a parameter file for an instance.
     *
     * \see ::CVIESetParameterFile()
     *
     * @param handle        the instance to set parameters for
     * @param parameterFile the path to the parameter file to set
     * @return the number of parameter settings in the loaded parameter file
     */
    public static int CVIESetParameterFile(CvieLibrary.HCVIE handle, String parameterFile) {
        IntByReference nSettingsRef = new IntByReference();
        CheckResult(CvieLibrary.CVIESetParameterFile(handle, parameterFile, nSettingsRef));
        return nSettingsRef.getValue();
    }

    /**
     * Sets a parameter buffer for an instance.
     *
     * \see ::CVIESetParameterBuffer()
     *
     * @param handle          the instance to set parameters for
     * @param parameterBuffer a buffer containing parameter XML data loaded from a
     *                        parameter file
     * @return the number of parameter settings in the loaded parameter file
     */
    public static int CVIESetParameterBuffer(CvieLibrary.HCVIE handle, ByteBuffer parameterBuffer) {
        IntByReference nSettingsRef = new IntByReference();
        CheckResult(CvieLibrary.CVIESetParameterBuffer(handle, parameterBuffer, nSettingsRef));
        return nSettingsRef.getValue();
    }

    /**
     * Setup a parameter setting for processing by specifying size and format of the
     * image buffers to process.
     *
     * \see ::CVIEEnhanceSetup()
     *
     * @param handle  the instance to operate on
     * @param width   the width of the image buffer
     * @param height  the height of the image buffer
     * @param flags   flags for processing, should be a value from \ref CVIE_DataTypes
     * @param setting the index of the setting to set up
     */
    public static void CVIEEnhanceSetup(CvieLibrary.HCVIE handle, int width, int height, int flags, int setting) {
        CheckResult(CvieLibrary.CVIEEnhanceSetup(handle, width, height, flags, setting, null));
    }

    /**
     * Enhance an image. The byte buffers shall be of the size specified by a
     * preceding call to CVIEEnhanceSetup() - height * width * bytesPerPixel.
     *
     * \note for optimal performance, the byte buffers should be Direct buffers,
     * created with ByteBuffer.allocateDirect().
     *
     * \warning if the byte buffers are too small, the results are undefined, and
     * the JVM may crash
     *
     * \see ::CVIEEnhanceNext()
     *
     * @param handle   the instance to operate on
     * @param inImage  the input image data to process
     * @param outImage the buffer to store output data in (may be the same buffer as
     *                 inImage for in-place processing)
     * @param setting  the index of the setting to use for enhancement
     */
    public static void CVIEEnhanceNext(CvieLibrary.HCVIE handle, ByteBuffer inImage, ByteBuffer outImage, int setting) {
        CheckResult(CvieLibrary.CVIEEnhanceNext(handle, inImage, outImage, setting));
    }

    /**
     * Set the number of CPU threads to use for processing. If not specified, or if
     * set to zero, will use a number of threads corresponding to the number of
     * logical cores in the CPU.
     *
     * \see ::CVIESetThreads()
     *
     * @param handle  the instance to operate on
     * @param threads the number of threads to use
     */
    public static void CVIESetThreads(CvieLibrary.HCVIE handle, int threads) {
        CheckResult(CvieLibrary.CVIESetThreads(handle, threads));
    }

    /**
     * Return a string describing the last error to occur for a given instance.
     *
     * \see ::CVEMGetLastError()
     *
     * @param handle the instance to operate on
     * @return a string describing the last error on the instance
     */
    public static String CVEMGetLastError(CvieLibrary.HCVIE handle) {
        ByteBuffer errorString = ByteBuffer.allocate(1024);

        return CvieLibrary.CVEMGetLastError(handle, errorString, errorString.capacity());
    }

    /**
     * Get a floating point parameter value.
     *
     * \see ::CVIEGetParameterValue()
     *
     * @param handle    the instance to operate on
     * @param setting   the setting index to operate on
     * @param parameter the id of the parameter to get
     * @return the value of the parameter
     */
    public static float CVIEGetParameterValueFloat(CvieLibrary.HCVIE handle, int setting, int parameter) {
        ByteBuffer value = ByteBuffer.allocate(4).order(ByteOrder.nativeOrder());
        CheckResult(CvieLibrary.CVIEGetParameterValue(handle, setting, parameter, value));

        return value.getFloat(0);
    }

    /**
     * Set a floating point parameter value.
     *
     * \see ::CVIESetParameterValue()
     *
     * @param handle    the instance to operate on
     * @param setting   the setting index to operate on
     * @param parameter the id of the parameter to set
     * @param value     the value to set
     */
    public static void CVIESetParameterValue(CvieLibrary.HCVIE handle, int setting, int parameter, float value) {
        // These should check type using CVIEGetParameterInfo
        ByteBuffer valueBytes = ByteBuffer.allocate(4).order(ByteOrder.nativeOrder()).putFloat(0, value);
        CheckResult(CvieLibrary.CVIESetParameterValue(handle, setting, parameter, valueBytes));
    }
    // For string parameters.
    public static void CVIESetParameterValue(CvieLibrary.HCVIE handle, int setting, int parameter, String value) {
        // These should check type using CVIEGetParameterInfo
        byte[] bytes = value.getBytes();        // Do this first as encoding may not be one byte per character in the string.
        ByteBuffer valueBytes = ByteBuffer.allocate(bytes.length + 1).put(bytes);
        valueBytes.put((byte)0);               // Zero terminate
        valueBytes.rewind();
        CheckResult(handle, CvieLibrary.CVIESetParameterValue(handle, setting, parameter, valueBytes));
    }

    /**
     * Save the parameter file.
     *
     * \see CVIESaveParameterFile()
     *
     * @param handle    the instance to save the loaded parameter file from
     * @param filename  the filename to save to
     */
    public static void CVIESaveParameterFile(CvieLibrary.HCVIE handle, String filename) {
        CheckResult(CvieLibrary.CVIESaveParameterFile(handle, filename));
    }

    /**
     * Gets the ParameterInfo of a setting and parameter
     *
     * \see CVIEGetParameterInfo()
     *
     * @param handle    the instance (not relevant for all parameters)
     * @param setting   the setting to get info for (not relevant for all parameters)
     * @param parameter the parameter to get info for
     * @return An OptionInfo containing information about the selected parameter.
     */
    public static CvieOptionInfo CVIEGetParameterInfo(CvieLibrary.HCVIE handle, int setting, int parameter) {
        IntByReference nTypeRef = new IntByReference();
        IntByReference nLengthRef = new IntByReference();
        CheckResult(CvieLibrary.CVIEGetParameterInfo(handle, setting, parameter, nTypeRef, nLengthRef));
        return new CvieOptionInfo(nTypeRef.getValue(), nLengthRef.getValue());
    }

    /**
     * Gets the DIInfo of a setting and parameter.
     *
     * \see CVIEGetDIInfo()
     *
     * @param handle    the instance
     * @param setting   the setting
     * @param parameter the TP parameter to get info for
     * @return a DIInfo struct for the parameter in the specified setting of the sepcified instance.
     */
    public static CvieTCVIEDIInfo CVIEGetDIInfo(CvieLibrary.HCVIE handle, int setting, int parameter) {
        CvieTCVIEDIInfo.ByReference info = new CvieTCVIEDIInfo.ByReference();
        CheckResult(handle, CvieLibrary.CVIEGetDIInfo(handle, setting, parameter, info));
        return info;
    }
}
