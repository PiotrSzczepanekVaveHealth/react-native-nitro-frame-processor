package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Flags for CVIEEnhanceSetup data format.
 *
 * \see CVIE_DataTypes
 *
 * \ingroup CVIE_java
 */
public enum CvieDataFlag {
    /**
     * Image data is stored as 16-bit unsigned integers.
     */
    U16(CvieLibrary.CVIE_DATA_U16, 2),

    /**
     * Image data is stored as 16-bit signed integers.
     */
    S16(CvieLibrary.CVIE_DATA_S16, 2),

    /**
     * Image data is stored as 8-bit unsigned integers.
     */
    U8(CvieLibrary.CVIE_DATA_U8, 1),

    /**
     * Image data is stored as 32-bit floating point values.
     */
    F32(CvieLibrary.CVIE_DATA_F32, 4);

    private static SparseArray<CvieDataFlag> map = new SparseArray<>();

    /// \cond
    static {
        for (CvieDataFlag flag : CvieDataFlag.values()) {
            map.put(flag.code, flag);
        }
    }
    /// \endcond

    private final int pixelSize;

    private final int code;

    private CvieDataFlag(final int code, final int pixelSize) {
        this.code = code;
        this.pixelSize = pixelSize;
    }

    /**
     * @param code integer data flag value
     * @return the CvieDataFlag enum value corresponding to the integer value
     */
    public static CvieDataFlag valueOf(int code) {
        CvieDataFlag flag = map.get(code);
        if (flag == null)
            throw new RuntimeException("CVIE data flag " + code + " unknown!");
        return flag;
    }

    /**
     * @return the integer data flag value corresponding to this enum value
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
