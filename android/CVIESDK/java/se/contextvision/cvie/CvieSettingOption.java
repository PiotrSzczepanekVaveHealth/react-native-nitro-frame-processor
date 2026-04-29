package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Setting options for CVIE2.
 *
 * \see SettingOptions
 *
 * \ingroup CVIE2_java
 */
public enum CvieSettingOption {
    /// \see ::CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_X
    FLOAT_RELATIVE_RESOLUTION_X(Cvie2Library.CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_X),
    /// \see ::CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_Y
    FLOAT_RELATIVE_RESOLUTION_Y(Cvie2Library.CVIE_SETTING_OPTION_FLOAT_RELATIVE_RESOLUTION_Y),
    /// \see ::CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START
    FLOAT_DEPTH_DEPENDENCE_TRANSITION_START(Cvie2Library.CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_START),
    /// \see ::CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END
    FLOAT_DEPTH_DEPENDENCE_TRANSITION_END(Cvie2Library.CVIE_SETTING_OPTION_FLOAT_DEPTH_DEPENDENCE_TRANSITION_END),
    /// \see ::CVIE_SETTING_OPTION_IMAGE_DESCRIPTOR_MASK
    IMAGE_DESCRIPTOR_MASK(Cvie2Library.CVIE_SETTING_OPTION_IMAGE_DESCRIPTOR_MASK),

    /// \see ::CVIE_SETTING_OPTION_INT32_NUM_THREADS
    INT32_NUM_THREADS(Cvie2Library.CVIE_SETTING_OPTION_INT32_NUM_THREADS);

    private static SparseArray<CvieSettingOption> map = new SparseArray<>();

    /// \cond
    static {
        for (CvieSettingOption obj : CvieSettingOption.values()) {
            map.put(obj.code, obj);
        }
    }
    /// \endcond

    private int code;

    private CvieSettingOption(final int code) {
        this.code = code;
    }

    /**
     * @param code integer option code
     * @return the enum value corresponding to the code
     */
    public static CvieSettingOption valueOf(int code) {
        CvieSettingOption obj = map.get(code);
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
