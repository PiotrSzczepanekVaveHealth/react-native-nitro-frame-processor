package se.contextvision.cvie;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import com.sun.jna.ptr.PointerByReference;
import com.sun.jna.ptr.IntByReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import static se.contextvision.cvie.Cvie2.CheckResult;

/**
 * This class represents a CVIE2 Instance.
 *
 * \see ::CvieInstance
 *
 * \ingroup CVIE2_java
 */
public class CvieInstance {
    private Cvie2 lib;
    Cvie2Library.CvieInstance handle;

    CvieInstance(Cvie2 cvie2) {
        this.lib = cvie2;
        PointerByReference newHandle = new PointerByReference();
        CheckResult(Cvie2Library.CvieInstance_Create(newHandle));
        handle = new Cvie2Library.CvieInstance(newHandle.getValue());
    }

    /**
     * Called by garbage collector, destroys the native object.
     *
     * @throws Throwable
     */
    @Override
    protected void finalize() throws Throwable {
        destroy();
        super.finalize();
    }

    /**
     * Loads a parameter file from a path.
     *
     * \see CvieInstance_LoadParameterFile()
     *
     * @param fileName the full path to the parameter file to load
     */
    public void loadParameterFile(String fileName) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        CheckResult(Cvie2Library.CvieInstance_LoadParameterFile(handle, fileName));
    }

    /**
     * Loads a parameter file from an InputStream.
     *
     * \see CvieInstance_LoadParameterBuffer()
     *
     * @param buf the input stream to load parameters from
     */
    public void loadParameterBuffer(InputStream buf) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        if (buf == null)
            throw new Cvie2Error(Cvie2Result.INVALID_INPUT, "Buffer must not be null!");
        ByteArrayOutputStream result = new ByteArrayOutputStream();
        byte[] buffer = new byte[1024];
        int length;
        try {
            while ((length = buf.read(buffer)) != -1) {
                result.write(buffer, 0, length);
            }
        } catch (IOException e) {
            throw new Cvie2Error(Cvie2Result.FILEIO_ERROR, e.getMessage());
        }

        ByteBuffer bytes = ByteBuffer.wrap(result.toByteArray());
        loadParameterBuffer(bytes);
    }

    /**
     * Loads a parameter file from a ByteBuffer object.
     *
     * \see CvieInstance_LoadParameterBuffer()
     *
     * @param buf the buffer to load parameters from
     */
    public void loadParameterBuffer(ByteBuffer buf) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        if (buf == null)
            throw new Cvie2Error(Cvie2Result.INVALID_INPUT, "Buffer must not be null!");

        CheckResult(Cvie2Library.CvieInstance_LoadParameterBuffer(handle, buf, buf.remaining()));
    }

    /**
     * Returns the number of settings available in a previously loaded parameter file.
     *
     * \see CvieInstance_GetNumberOfSettings()
     *
     * @return the number of settings
     */
    public int getNumSettings() {
        IntByReference nSettingsRef = new IntByReference();
        CheckResult(Cvie2Library.CvieInstance_GetNumberOfSettings(handle, nSettingsRef));
        return nSettingsRef.getValue();
    }

    /**
     * Returns a setting from a previously loaded parameter file.
     *
     * \see CvieInstance_GetParameterSetting()
     *
     * @param i the index of the setting to return
     * @return the setting
     */
    public CvieParameterSetting getSetting(int i) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        return new CvieParameterSetting(this, i);
    }

    /**
     * Sets an option with an integer value.
     *
     * \see CvieInstance_SetOption()
     *
     * @param option the option to modify
     * @param value the value to set
     */
    public void setOption(CvieInstanceOption option, int value) {
        ByteBuffer bytes = ByteBuffer.allocate(4).order(ByteOrder.nativeOrder()).putInt(0, value);
        CheckResult(Cvie2Library.CvieInstance_SetOption(handle, option.getCode(), bytes, 4));
    }

    /**
     * Gets the value of an integer option.
     *
     * \see CvieInstance_GetOption()
     *
     * @param option the option to get
     * @return the current value of the option
     */
    public int getIntOption(CvieInstanceOption option) {
        ByteBuffer value = ByteBuffer.allocate(4);
        CheckResult(Cvie2Library.CvieInstance_GetOption(handle, option.getCode(), value, value.remaining(), null));
        return value.order(ByteOrder.nativeOrder()).getInt(0);
    }

    /**
     * Gets the OptionInfo of an option.
     *
     * \see CvieInstance_GetOptionInfo()
     *
     * @param option the option to get
     * @return the current value of the OptionInfo
     */
    public CvieOptionInfo getOptionInfo(int option) {
        IntByReference nTypeRef = new IntByReference();
        IntByReference nLengthRef = new IntByReference();
        CheckResult(Cvie2Library.CvieInstance_GetOptionInfo(handle, option, nTypeRef, nLengthRef));
        return new CvieOptionInfo(nTypeRef.getValue(), nLengthRef.getValue());
    }

    /**
     * Save the parameter file.
     *
     * \see CvieInstance_SaveParameterFile()
     *
     * @param filename the filename to save to
     */
    public void saveParameterFile(String filename) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        CheckResult(Cvie2Library.CvieInstance_SaveParameterFile(handle, filename));
    }

    /**
     * Release Memory.
     *
     * \see CvieInstance_ReleaseMemory()
     *
     */
    public void releaseMemory() {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        CheckResult(Cvie2Library.CvieInstance_ReleaseMemory(handle));
    }

    /**
     * Destroys the instance.  Further operations on it will fail.
     * This function will be called automatically by the finalizer, but can also be called
     * explicitly when the object is not needed anymore.
     *
     * \see CvieInstance_Destroy()
     */
    public void destroy() {
        if (handle != null) {
            PointerByReference handleRef = new PointerByReference(handle.getPointer());
            try {
                CheckResult(Cvie2Library.CvieInstance_Destroy(handleRef));
            }
            finally {
                handle = null;
            }
        }
        if (lib != null) {
            lib.deleteInstance(this);
            lib = null;
        }
    }
}
