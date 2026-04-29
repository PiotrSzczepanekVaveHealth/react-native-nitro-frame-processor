package se.contextvision.cvie;

/**
 * POJO Class To hold the OptionInfo as a Java type
 *
 */

public class CvieOptionInfo {
    private int type;
    private int size;

    /**
     * Constructor
     *
     * @param type    the type of the parameter
     * @param size    the size in bytes of the parameter value
     */
    public CvieOptionInfo(int type, int size) {
        this.type = type;
        this.size = size;
    }

    /**
     * @return a CvieLibrary.CVIE_PARTYPE_* constant corresponding to the type of the parameter
     */
    public int getType() {
        return type;
    }

    /**
     * @return the number of bytes required for the value. This only depends on the type except for strings where it depends on the current string
     */
    public int getSize() {
        return size;
    }
}
