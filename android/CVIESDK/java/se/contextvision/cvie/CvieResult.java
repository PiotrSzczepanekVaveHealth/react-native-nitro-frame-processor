package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Enum defining the CVIE result codes.
 *
 * \see CVIE_Results
 *
 * \ingroup CVIE_java
 */

public enum CvieResult {
    /**
     The call succeeded.
     */
    OK(0),

    /**
     The call failed.
     */
    NOT_OK(1),

    /**
     Failed to create handle.
     */
    CREATE_HANDLE(2),

    /**
     The supplied instance handle is invalid.
     */
    BAD_HANDLE(3),

    /**
     The operation was aborted by the user (by returning 0 from a progress callback function).
     */
    USER_ABORTED(4),

    /**
     A fatal error has occurred (e.g. out of memory). The library state is undefined and no further calls should be made.
     */
    FATAL_ERROR(5),

    /**
     The parameter file could not be opened or read.
     */
    FILEIO_ERROR(6),

    /**
     The prerequisites for calling this function has not been met.
     */
    ILLEGAL_COMMAND(7),

    /**
     One or more parameters are invalid or out of range.
     */
    INVALID_INPUT(8),

    /**
     The supplied parameter file is invalid or corrupted.
     */
    INVALID_PARAMETER_FILE(9),

    /**
     There is no valid license for the requested operation.
     */
    LICENSE_ERROR(10),

    /**
     The supplied parameters are not supported.
     */
    NOT_SUPPORTED(11),

    /**
     The requested setting has not been set up.  Call CVIEEnhanceSetup() first.
     */
    SETTING_NOT_READY(12),

    /**
     An unknown error has occured.
     */
    UNKNOWN_ERROR(15),

    /**
     There was an error during GPU processing.
     */
    GPU_ERROR(16);


    private static SparseArray<CvieResult> map = new SparseArray<>();

    /// \cond
    static {
        for (CvieResult result : CvieResult.values()) {
            map.put(result.code, result);
        }
    }
    /// \endcond

    private int code;

    private CvieResult(final int code) {
        this.code = code;
    }

    /**
     * @param code integer CVIE error code
     * @return the CvieResult enum value corresponding to the integer value.
     */
    public static CvieResult valueOf(int code) {
        CvieResult result = map.get(code);
        if (result == null)
            throw new RuntimeException("CVIE error code " + code + " unknown!");
        return result;
    }

    /**
     * @return the integer CVIE error code corresponding to this enum value
     */
    public int getCode() {
        return code;
    }
}
