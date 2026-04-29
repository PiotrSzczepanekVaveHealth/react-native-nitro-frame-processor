package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Global options for CVIE2.
 *
 * \see GlobalOptions
 *
 * \ingroup CVIE2_java
 */
public enum CvieOption {
    /**
     * This option sets the Device ID used for licensing. (read-write)
     * \see ::CVIE_OPTION_CSTR_LICENSE_ID
     */
    CSTR_LICENSE_ID(Cvie2Library.CVIE_OPTION_CSTR_LICENSE_ID),


    /**
     * Set this option to a non-zero value to enable trace log output.
     * This generates an encrypted log file which can be used by
     * ContextVision engineers to diagnose problems with the processing. (read-write)
     * \see \ref TraceLog
     * \see ::CVIE_OPTION_INT32_TRACE_ENABLE
     */
    INT32_TRACE_ENABLE(Cvie2Library.CVIE_OPTION_INT32_TRACE_ENABLE);

    private static SparseArray<CvieOption> map = new SparseArray<>();

    /// \cond
    static {
        for (CvieOption obj : CvieOption.values()) {
            map.put(obj.code, obj);
        }
    }
    /// \endcond

    private int code;

    private CvieOption(final int code) {
        this.code = code;
    }

    /**
     * @param code integer option code
     * @return the enum value corresponding to the code
     */
    public static CvieOption valueOf(int code) {
        CvieOption obj = map.get(code);
        if (obj == null)
            throw new RuntimeException("CVIE2 option code " + code + " unknown!");
        return obj;
    }

    /**
     * @return the integer code corresponding to this enum value
     */
    public int getCode() {
        return code;
    }

}
