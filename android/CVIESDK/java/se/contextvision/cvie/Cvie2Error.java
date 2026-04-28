package se.contextvision.cvie;

/**
 * CVIE2 Exception class.  This is thrown when the native C API reports an error.
 *
 * \ingroup CVIE2_java
 */
public class Cvie2Error extends RuntimeException {
    private final Cvie2Result result;
    private final String message;

    Cvie2Error(int code) {
        this(Cvie2Result.valueOf(code));
    }

    Cvie2Error(int code, String message) {
        this(Cvie2Result.valueOf(code), message);
    }

    Cvie2Error(Cvie2Result result) {
        this.result = result;
        message = "No detailed message available";
    }

    Cvie2Error(Cvie2Result result, String message) {
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
        return "CVIE2 Error " + result.name() + " (" + result.getCode() + "): " + this.message;
    }

    /**
     * Gets the result code of the error.
     *
     * @return the result code
     */
    public Cvie2Result getResult() {
        return result;
    }
}
