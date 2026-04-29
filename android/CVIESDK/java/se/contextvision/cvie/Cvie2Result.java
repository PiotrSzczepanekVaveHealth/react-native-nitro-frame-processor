package se.contextvision.cvie;

import android.util.SparseArray;

/**
 * Enum defining the CVIE2 result codes.
 *
 * \see Results
 *
 * \ingroup CVIE2_java
 */
public enum Cvie2Result {
    /**
     * The call succeeded.
     */
    OK(0),

    /**
     * The handle is invalid.
     */
    BAD_HANDLE(3),

    /**
     * The operation was aborted by the user.
     */
    USER_ABORTED(4),

    /**
     * File IO error, for example file not found or not readable.
     */
    FILEIO_ERROR(6),

    /**
     * The call is made out of sequence and illegal in the current state.
     */
    ILLEGAL_COMMAND(7),

    /**
     * One of the actual parameters has an invalid value.
     */
    INVALID_INPUT(8),

    /**
     * The supplied parameter file or buffer is invalid.
     */
    INVALID_PARAMETER_FILE(9),

    /**
     * The requested action requires a license key that is not found.
     */
    LICENSE_ERROR(10),

    /**
     * The requested action, while recognized, is not supported in the current configuration.
     */
    NOT_SUPPORTED(11),

    /**
     * A memory allocation failed.
     * The library is in an invalid state and no further usage of the library API is possible.
     * <p>
     * \note This return code is possible from any API function and is not specifically declared
     * as a possible return value for individual functions.
     */
    OUT_OF_MEMORY(16),

    /**
     * An unexpected fatal error occurred, for example out of memory or memory corruption.
     * The library is in an invalid state and no further usage of the library API is possible.
     * <p>
     * \note This return code is possible from any API function and is not specifically declared
     * as a possible return value for individual functions.
     */
    FATAL_ERROR(5),

    /**
     * An unknown error has occurred. Continued use of the API is possible, but may lead to further errors.
     * <p>
     * \note This return code is possible from any API function and is not specifically declared
     * as a possible return value for individual functions.
     */
    UNKNOWN_ERROR(15),

    /**
     * A GPU error has occurred, for example out of memory, no supported GPU found or failure to
     * initialize the GPU platform driver.  Continued use of the API is possible, but may lead to further errors.
     */
    GPU_ERROR(17);

    private static SparseArray<Cvie2Result> map = new SparseArray<>();

    /// \cond
    static {
        for (Cvie2Result result : Cvie2Result.values()) {
            map.put(result.code, result);
        }
    }
    /// \endcond

    private int code;

    private Cvie2Result(final int code) {
        this.code = code;
    }

    /**
     * @param code integer CVIE2 error code
     * @return the Cvie2Result enum value corresponding to the integer value.
     */
    public static Cvie2Result valueOf(int code) {
        Cvie2Result result = map.get(code);
        if (result == null)
            throw new RuntimeException("CVIE2 error code " + code + " unknown!");
        return result;
    }

    /**
     * @return the integer CVIE2 error code corresponding to this enum value
     */
    public int getCode() {
        return code;
    }
}
