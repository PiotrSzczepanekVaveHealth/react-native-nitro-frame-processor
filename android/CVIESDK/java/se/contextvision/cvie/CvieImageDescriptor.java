package se.contextvision.cvie;

import com.sun.jna.ptr.PointerByReference;
import java.nio.ByteBuffer;

import static se.contextvision.cvie.Cvie2.CheckResult;

/**
 * Class representing a CVIE2 Image Descriptor.
 *
 * \see ::CvieImageDescriptor
 *
 * \ingroup CVIE2_java
 */
public class CvieImageDescriptor {
    private Cvie2 lib;
    Cvie2Library.CvieImageDescriptor handle;
    private final int width;
    private final int height;
    private final CviePixelFormat format;
    private ByteBuffer buffer;
    private int rowStrideInBytes;

    CvieImageDescriptor(Cvie2 cvie2, int width, int height, CviePixelFormat format) {
        this.lib = cvie2;
        this.width = width;
        this.height = height;
        this.format = format;

        PointerByReference newHandle = new PointerByReference();
        CheckResult(Cvie2Library.CvieImageDescriptor_CreateForBuffer2D(newHandle, width, height, format.getCode()));
        handle = new Cvie2Library.CvieImageDescriptor(newHandle.getValue());
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
     * Attaches a byte buffer to this descriptor.  The buffer must be a Direct byte buffer (created
     * with ByteBuffer.allocateDirect()), otherwise an exception will be thrown.
     *
     * The buffer's size must be at least rowStrideInBytes * height.
     *
     * \see CvieImageDescriptor_AttachBuffer2D()
     *
     * @param buffer the buffer to attach
     * @param rowStrideInBytes the total length of each row in bytes, including any padding
     */
    public void attach(ByteBuffer buffer, int rowStrideInBytes) {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        if (buffer.remaining() < rowStrideInBytes * height)
            throw new Cvie2Error(Cvie2Result.INVALID_INPUT, "Buffer is too small!");

        if (!buffer.isDirect())
            throw new Cvie2Error(Cvie2Result.INVALID_INPUT, "Only direct buffers are supported");

        this.buffer = buffer;
        this.rowStrideInBytes = rowStrideInBytes;

        CheckResult(Cvie2Library.CvieImageDescriptor_AttachBuffer2D(handle, this.buffer, rowStrideInBytes));
    }

    /**
     * Detaches a previously attached buffer.
     *
     * \see CvieImageDescriptor_Detach()
     */
    public void detach() {
        if (handle == null) {
            throw new IllegalStateException("Object has been destroyed");
        }

        CheckResult(Cvie2Library.CvieImageDescriptor_Detach(handle));

        this.buffer = null;
        this.rowStrideInBytes = 0;
    }

    /**
     * Gets the width in pixels of the descriptor.
     *
     * @return the width in pixels
     */
    public int getWidth() {
        return width;
    }

    /**
     * Gets the height in pixels of the descriptor.
     *
     * @return the height in pixels
     */
    public int getHeight() {
        return height;
    }

    /**
     * Gets the pixel format of the descriptor.
     *
     * @return the pixel format
     */
    public CviePixelFormat getPixelFormat() {
        return format;
    }

    /**
     * Destroys the image descriptor.  Further operations on it will fail.
     * This function will be called automatically by the finalizer, but can also be called
     * explicitly when the object is not needed anymore.
     *
     * \see CvieImageDescriptor_Destroy()
     */
    public void destroy() {
        if (handle != null) {
            PointerByReference handleRef = new PointerByReference(handle.getPointer());
            try {
                CheckResult(Cvie2Library.CvieImageDescriptor_Destroy(handleRef));
            }
            finally {
                handle = null;
                buffer = null;
            }
        }
        if (lib != null) {
            lib.deleteImageDescriptor(this);
            lib = null;
        }
    }
}
