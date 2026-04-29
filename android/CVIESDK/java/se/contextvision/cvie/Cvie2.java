package se.contextvision.cvie;

import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashMap;
import java.util.Map;
import java.lang.String;
import android.os.Environment;

import com.sun.jna.ptr.IntByReference;

import static se.contextvision.cvie.Cvie2Library.CVIE_RESULT_OK;

/**
 * \defgroup CVIE2_java CVIE2 Java API
 * 
 * These classes give access to the CVIE2 C API.  The high level interface, which is recommended,
 * is available in the Cvie2 class (and others), and the low level JNA wrapper is available in the
 * Cvie2Library class.
 * 
 * \warning The CVIE2 API is deprecated and its documentation is included only for existing customers using it.
 *
 * \see CVIE2
 *
 * Most functions in this package will throw Cvie2Error if there is any error reported from the native
 * call.  These errors should be caught and handled appropriately.
 */

/**
 * This class wraps the global CVIE2 library state.  Use it to initialize the library and to create
 * other objects.  It is implemented as a singleton - use the static Get function to get the
 * singleton instance.
 *
 * For more in-depth documentation of the various functions, see the C API documentation (\ref CVIE2).
 * 
 * \ingroup CVIE2_java
 */
public class Cvie2 {
    private static final Object mutex = 0;
    private static Cvie2 lib;
    private Map<Cvie2Library.CvieInstance, WeakReference<CvieInstance>> instances = new HashMap<>();
    private Map<Cvie2Library.CvieImageDescriptor, WeakReference<CvieImageDescriptor>> imageDescriptors = new HashMap<>();
    private boolean destroying = false;

    static void CheckResult(int cvieResult)
    {
        if (cvieResult != CVIE_RESULT_OK) {
            String message = Cvie2Library.CvieGetLastError();
            throw new Cvie2Error(cvieResult, message);
        }
    }

    private static ByteBuffer stringToBytes(String s) {
        ByteBuffer bytes = ByteBuffer.allocate(s.getBytes().length + 1);
        bytes.put(s.getBytes());
        bytes.put((byte) 0);
        bytes.rewind();
        return bytes;
    }

    private Cvie2() {
        CheckResult(Cvie2Library.CvieEnter(Cvie2Library.CVIE_API_VERSION));
    }

    /**
     * Initializes (calls CvieEnter()) and returns the CVIE2 library singleton object.
     * This is the starting point for all of the CVIE2 functionality.
     *
     * @return the CVIE2 library singleton
     */
    public static Cvie2 Get() {
        synchronized (mutex) {
            if (lib == null) {
                lib = new Cvie2();
            }
            return lib;
        }
    }

    /**
     * Prepares to unload the CVIE2 library.  It cleans up the native library
     * (calls CvieExit()) and clears the singleton reference.  Unless there are more references to the singleton,
     * it will eventually be garbage collected.
     */
    synchronized public static void unload() {
        synchronized (mutex) {
            if (lib == null)
                return;

            lib.unloadSelf();
            lib = null;
        }
    }

    private void unloadSelf() {
        if (destroying)
            return;

        destroying = true;
        try {
            for (WeakReference<CvieInstance> i : instances.values()) {
                CvieInstance cvieInstance = i.get();
                if (cvieInstance != null)
                    cvieInstance.destroy();
            }
            instances.clear();

            for (WeakReference<CvieImageDescriptor> i : imageDescriptors.values()) {
                CvieImageDescriptor descriptor = i.get();
                if (descriptor != null)
                    descriptor.destroy();
            }
            imageDescriptors.clear();
        }
        finally {
            Cvie2Library.CvieExit();
        }

    }

    /**
     * Creates a new \ref CvieInstance object.
     *
     * \see CvieInstance_Create()
     * 
     * @return a newly created CvieInstance object
     */
    public CvieInstance createInstance() {
        CvieInstance instance = new CvieInstance(this);
        instances.put(instance.handle, new WeakReference<>(instance));
        return instance;
    }

    /**
     * Creates a new \ref CvieImageDescriptor object with a 2D image geometry.
     * 
     * \see CvieImageDescriptor_CreateForBuffer2D()
     *
     * @param width the width of the image in pixels
     * @param height the height of the image in pixels
     * @param format the format of the image pixels
     * @return a newly created CvieImageDescriptor object
     */
    public CvieImageDescriptor createImageDescriptor(int width, int height, CviePixelFormat format) {
        CvieImageDescriptor imageDescriptor = new CvieImageDescriptor(this, width, height, format);
        imageDescriptors.put(imageDescriptor.handle, new WeakReference<>(imageDescriptor));
        return imageDescriptor;
    }

    /**
     * Called by garbage collector, unloads native library.
     *
     * @throws Throwable
     */
    @Override
    protected void finalize() throws Throwable {
        unloadSelf();

        super.finalize();
    }

    void deleteInstance(CvieInstance instance) {
        if (!destroying) {
            instances.remove(instance.handle);
        }
    }

    void deleteImageDescriptor(CvieImageDescriptor descriptor) {
        if (!destroying) {
            imageDescriptors.remove(descriptor.handle);
        }
    }

    /**
     * Sets a global option with an integer value.
     *
     * @param option the option to modify
     * @param value the value to set
     *
     * \see CvieSetOption()
     */
    public void setOption(CvieOption option, int value) {
        ByteBuffer bytes = ByteBuffer.allocate(4).order(ByteOrder.nativeOrder()).putInt(0, value);
        CheckResult(Cvie2Library.CvieSetOption(option.getCode(), bytes, bytes.capacity()));
    }
    
    /**
     * Sets a global option with a string value.
     *
     * @param option the option to modify
     * @param value the value to set
     *
     * \see CvieSetOption()
     */
    public void setOption(CvieOption option, String value) {
        ByteBuffer bytes = stringToBytes(value);
        CheckResult(Cvie2Library.CvieSetOption(option.getCode(), bytes, bytes.capacity()));
    }

    /**
     * Gets a global option with an integer value.
     * 
     * \see CvieGetOption()
     *
     * @param option the option to read
     * @return the value of the option
     */
    public int getIntOption(CvieOption option) {
        ByteBuffer value = ByteBuffer.allocate(4).order(ByteOrder.nativeOrder());
        IntByReference actualLengthRef = new IntByReference();
        CheckResult(Cvie2Library.CvieGetOption(option.getCode(), value, value.capacity(), actualLengthRef));

        return value.getInt(0);
    }

    /**
     * Gets the type of a global option.
     *
     * @param option the option to query
     * @return the type of the option
     *
     * \see CvieGetOptionInfo()
     */
    public CvieOptionType getOptionType(CvieOption option) {
        IntByReference valueRef = new IntByReference();
        CheckResult(Cvie2Library.CvieGetOptionInfo(option.getCode(), valueRef, null));
        return CvieOptionType.valueOf(valueRef.getValue());
    }

    /**
     * Gets the size of a global option.
     *
     * @param option the option to query
     * @return the size of the option value in bytes
     *
     * \see CvieGetOptionInfo()
     */
    public int getOptionSize(CvieOption option) {
        IntByReference valueRef = new IntByReference();
        CheckResult(Cvie2Library.CvieGetOptionInfo(option.getCode(), null, valueRef));
        return valueRef.getValue();
    }
}
