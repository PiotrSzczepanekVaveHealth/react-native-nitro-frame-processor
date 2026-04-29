package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Pixel Formats for CVIE2.
 *
 * \see PixelFormats
 *
 * \ingroup CVIE2_java
 */
public enum CviePixelFormat {
    /**
     * Image data is stored as 8-bit unsigned integers.
     */
    U8(Cvie2Library.CVIE_PIXEL_FORMAT_U8, 1),

    /**
     * Image data is stored as 16-bit unsigned integers.
     */
    U16(Cvie2Library.CVIE_PIXEL_FORMAT_U16, 2),

    /**
     * Image data is stored as 16-bit signed integers.
     */
    S16(Cvie2Library.CVIE_PIXEL_FORMAT_S16, 2),

    /**
     * Image data is stored as 32-bit floating point values.
     */
    F32(Cvie2Library.CVIE_PIXEL_FORMAT_F32, 4);

    private static SparseArray<CviePixelFormat> map = new SparseArray<>();

    /// \cond
    static {
        for (CviePixelFormat obj : CviePixelFormat.values()) {
            map.put(obj.code, obj);
        }
    }
    /// \endcond

    private final int pixelSize;
    private int code;

    private CviePixelFormat(final int code, final int pixelSize)
    {
        this.code = code;
        this.pixelSize = pixelSize;
    }

    /**
     * @param code integer pixel format value
     * @return the enum value corresponding to the integer value
     */
    public static CviePixelFormat valueOf(int code) {
        CviePixelFormat obj = map.get(code);
        if (obj == null)
            throw new RuntimeException("CVIE pixel format " + code + " unknown!");
        return obj;
    }

    /**
     * @return the integer pixel format value corresponding to this enum value
     */
    public int getCode() {
        return code;
    }

    /**
     * @return the pixel size in bytes for this enum value
     */
    public int getPixelSize() {
        return pixelSize;
    }
}
