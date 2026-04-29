package se.contextvision.cvie;

import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import static se.contextvision.cvie.Cvie2.CheckResult;

/**
 * Class representing a CVIE2 ParameterSetting.
 *
 * \see ::CvieParameterSetting
 *
 * \ingroup CVIE2_java
 */
public class CvieParameterSetting {
    private Cvie2Library.CvieParameterSetting handle;
    private CvieInstance instance;

    CvieParameterSetting(CvieInstance instance, int i) {
        PointerByReference handleRef = new PointerByReference();
        CheckResult(Cvie2Library.CvieInstance_GetParameterSetting(instance.handle, i, handleRef));
        this.instance = instance; // To prevent the instance from being garbage collected
        this.handle = new Cvie2Library.CvieParameterSetting(handleRef.getValue());
    }

    /**
     * Sets up the parameter setting for processing by associating it with an input and output image
     * descriptor.
     *
     * \see CvieParameterSetting_Setup()
     *
     * @param in an image descriptor describing the input image buffer
     * @param out an image descriptor describing the output image buffer (can be the same as \c in)
     */
    public void setup(CvieImageDescriptor in, CvieImageDescriptor out) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        if (in == null || out == null)
            throw new Cvie2Error(Cvie2Result.INVALID_INPUT, "Input and output descriptors must not be null");

        CheckResult(Cvie2Library.CvieParameterSetting_Setup(handle, in.handle, out.handle));
    }

    /**
     * Enhances an image using this parameter setting.  The input and output image will be the
     * image descriptor(s) previously set with setup().
     *
     * \see CvieParameterSetting_Enhance()
     */
    public void enhance() {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        CheckResult(Cvie2Library.CvieParameterSetting_Enhance(handle));
    }

    /**
     * Sets the value of an option with an integer value.
     *
     * \see CvieParameterSetting_SetOption()
     *
     * @param option the option to modify
     * @param value the value to set
     */
    public void setOption(CvieSettingOption option, int value) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        ByteBuffer bytes = ByteBuffer.allocate(4).order(ByteOrder.nativeOrder()).putInt(0, value);
        CheckResult(Cvie2Library.CvieParameterSetting_SetOption(handle, option.getCode(), bytes, 4));
    }

    /**
     * Gets the value of an option with an integer value.
     *
     * \see CvieParameterSetting_GetOption()
     *
     * @param option the option to read
     * @return the current value of the option
     */
    public int getIntOption(CvieSettingOption option) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        ByteBuffer value = ByteBuffer.allocate(4);
        CheckResult(Cvie2Library.CvieParameterSetting_GetOption(handle, option.getCode(), value, value.capacity(), null));
        return value.order(ByteOrder.nativeOrder()).getInt(0);
    }

    /**
     * Sets the value of an option with a float value.
     *
     * \see CvieParameterSetting_SetOption()
     *
     * @param option the option to modify
     * @param value the value to set
     */
    public void setOption(CvieSettingOption option, float value) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        ByteBuffer bytes = ByteBuffer.allocate(4).order(ByteOrder.nativeOrder()).putFloat(0, value);
        CheckResult(Cvie2Library.CvieParameterSetting_SetOption(handle, option.getCode(), bytes, 4));
    }

    /**
     * Sets the value of an option with a string value.
     *
     * \see CvieParameterSetting_SetOption()
     *
     * @param option the option to modify
     * @param value the value to set
     */
    public void setOption(CvieSettingOption option, String value) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        byte[] bytes = value.getBytes();        // Do this first as encoding may not be one byte per character in the string.
        ByteBuffer valueBytes = ByteBuffer.allocate(bytes.length + 1).put(bytes);
        valueBytes.put((byte)0);               // Zero terminate
        valueBytes.rewind();
        CheckResult(Cvie2Library.CvieParameterSetting_SetOption(handle, option.getCode(), valueBytes, valueBytes.capacity()));
    }

    /**
     * Gets the value of an option with a float value.
     *
     * \see CvieParameterSetting_GetOption()
     *
     * @param option the option to read
     * @return the current value of the option
     */
    public float getFloatOption(CvieSettingOption option) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        ByteBuffer value = ByteBuffer.allocate(4);
        CheckResult(Cvie2Library.CvieParameterSetting_GetOption(handle, option.getCode(), value, value.capacity(), null));
        return value.order(ByteOrder.nativeOrder()).getFloat(0);
    }

    /**
     * Gets the DIInfo of an option.
     *
     * \see CvieParameterSetting_GetDIInfo()
     *
     * @param option the option to get
     * @return the current value of the DIInfo
     */
    public CvieTCVIEDIInfo getDIInfo(int option) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }
        CvieTCVIEDIInfo.ByReference info = new CvieTCVIEDIInfo.ByReference();
        CheckResult(Cvie2Library.CvieParameterSetting_GetDIInfo(handle, option, info));
        return info;
    }

    /**
     * Gets the OptionInfo of an option.
     *
     * \see CvieParameterSetting_GetOptionInfo()
     *
     * @param option the option to get
     * @return the current value of the OptionInfo
     */
    public CvieOptionInfo getOptionInfo(int option) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        IntByReference nTypeRef = new IntByReference();
        IntByReference nLengthRef = new IntByReference();
        CheckResult(Cvie2Library.CvieParameterSetting_GetOptionInfo(handle, option, nTypeRef, nLengthRef));
        return new CvieOptionInfo(nTypeRef.getValue(), nLengthRef.getValue());
    }

    /**
     * Destroys the setting.  Further operations on it will fail.
     * This function can be called when the object is not needed anymore to allow the associated
     * instance to be garbage collected.
     */
    public void destroy() {
        handle = null;
        instance = null;
    }
}
