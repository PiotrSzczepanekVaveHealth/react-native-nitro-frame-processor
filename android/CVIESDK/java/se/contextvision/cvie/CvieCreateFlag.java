package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Flags for CVIECreate.
 *
 * \see CVIE_CreateFlags
 *
 * \ingroup CVIE_java
 */
public enum CvieCreateFlag {
    /// Default operation
    /// \see ::CVIE_CREATE_DEFAULT
    DEFAULT(CvieLibrary.CVIE_CREATE_DEFAULT),
    /// Enable trace logging
    /// \see ::CVIE_CREATE_TRACE
    TRACE(CvieLibrary.CVIE_CREATE_TRACE);

    private static SparseArray<CvieCreateFlag> map = new SparseArray<>();

    /// \cond
    static {
        for (CvieCreateFlag flag : CvieCreateFlag.values()) {
            map.put(flag.code, flag);
        }
    }
    /// \endcond

    private final int code;

    private CvieCreateFlag(final int code) {
        this.code = code;
    }

    /**
     * @param code integer flag value
     * @return the enum value corresponding to the integer value.
     */
    public static CvieCreateFlag valueOf(int code) {
        CvieCreateFlag flag = map.get(code);
        if (flag == null)
            throw new RuntimeException("CVIE create flag " + code + " unknown!");
        return flag;
    }

    /**
     * @return the integer flag value corresponding to this enum value
     */
    public int getCode() {
        return code;
    }
}
