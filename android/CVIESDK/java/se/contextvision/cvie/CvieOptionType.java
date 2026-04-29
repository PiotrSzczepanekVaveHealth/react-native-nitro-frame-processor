package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Option types for CVIE2.
 *
 * \see Options
 *
 * \ingroup CVIE2_java
 */

public enum CvieOptionType {
    /**
     * Image descriptor handle
     * \see ::CVIE_OPTION_TYPE_IMAGE_DESCRIPTOR
     */
    IMAGE_DESCRIPTOR(Cvie2Library.CVIE_OPTION_TYPE_IMAGE_DESCRIPTOR),

    /**
     * 32-bit signed integer
     * \see ::CVIE_OPTION_TYPE_INT32
     */
    INT32(Cvie2Library.CVIE_OPTION_TYPE_INT32),

    /**
     * 32-bit float.
     * \see ::CVIE_OPTION_TYPE_FLOAT
     */
    FLOAT(Cvie2Library.CVIE_OPTION_TYPE_FLOAT),

    /**
     * Null-terminated string
     * \see ::CVIE_OPTION_TYPE_CSTR
     */
    CSTR(Cvie2Library.CVIE_OPTION_TYPE_CSTR);

    private static SparseArray<CvieOptionType> map = new SparseArray<>();

    /// \cond
    static {
        for (CvieOptionType obj : CvieOptionType.values()) {
            map.put(obj.code, obj);
        }
    }
    /// \endcond

    private int code;

    private CvieOptionType(final int code)
    {
        this.code = code;
    }

    /**
     * @param code an integer option type code
     * @return the enum value corresponding to the integer type code
     */
    public static CvieOptionType valueOf(int code) {
        CvieOptionType obj = map.get(code);
        if (obj == null)
            throw new RuntimeException("CVIE2 option type " + code + " unknown!");
        return obj;
    }

    /**
     * @return the integer type code corresponding to this enum value
     */
    public int getCode() {
        return code;
    }
}
