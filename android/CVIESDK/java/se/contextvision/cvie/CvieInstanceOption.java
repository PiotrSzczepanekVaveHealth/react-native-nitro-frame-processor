package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Instance options for CVIE.
 *
 * \see InstanceOptions
 *
 * \ingroup CVIE2_java
 */
public enum CvieInstanceOption {

    /// \see ::CVIE_INSTANCE_OPTION_CSTR_PARAMETER_DESCRIPTION
    CSTR_PARAMETER_DESCRIPTION(Cvie2Library.CVIE_INSTANCE_OPTION_CSTR_PARAMETER_DESCRIPTION);

    private static SparseArray<CvieInstanceOption> map = new SparseArray<>();

    /// \cond
    static {
        for (CvieInstanceOption obj : CvieInstanceOption.values()) {
            map.put(obj.code, obj);
        }
    }
    /// \endcond

    private int code;

    private CvieInstanceOption(final int code) {
        this.code = code;
    }

    /**
     * @param code integer option code
     * @return the enum value corresponding to the code
     */
    public static CvieInstanceOption valueOf(int code) {
        CvieInstanceOption obj = map.get(code);
        if (obj == null)
            throw new RuntimeException("CVIE instance option code " + code + " unknown!");
        return obj;
    }

    /**
     * @return the integer code corresponding to this enum value
     */
    public int getCode() {
        return code;
    }

}
