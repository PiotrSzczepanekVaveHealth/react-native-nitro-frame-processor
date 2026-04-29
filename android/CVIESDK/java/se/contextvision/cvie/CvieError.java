package se.contextvision.cvie;

/**
 * CVIE Exception class.
 *
 * \ingroup CVIE_java
 */

public class CvieError extends RuntimeException {
    private final CvieResult result;
    private final String message;

    CvieError(int code) {
        this(CvieResult.valueOf(code));
    }

    CvieError(int code, String message) {
        this(CvieResult.valueOf(code), message);
    }

    CvieError(CvieResult result) {
        this.result = result;
        message = result.name();
    }

    CvieError(CvieResult result, String message) {
        this.result = result;
        this.message = message;
    }

    /**
     * Gets a string message describing the error.
     *
     * @return the error message
     */
    @Override
    public String getMessage() {
        return "CVIE Error " + result.getCode() + " (" + this.message + ")";
    }

    /**
     * Gets the result code of the error.
     *
     * @return the result code
     */
    public CvieResult getResult() {
        return result;
    }
}
