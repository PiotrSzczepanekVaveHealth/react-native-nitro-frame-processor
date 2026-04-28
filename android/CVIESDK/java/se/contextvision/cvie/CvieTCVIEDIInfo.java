package se.contextvision.cvie;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import java.util.Arrays;
import java.util.List;

/**
 * CvieTCVIEDIInfo is the Java imlementation for the C struct TCVIEDIInfo extending the JNA
 * Structure class.
 *
 */
public class CvieTCVIEDIInfo extends Structure {
    /** Low end of scale for any setting. Lowest allowed value. */
    public float nominal_min;
    /** Min value affected by current setting. Can be higher than nominal_min. */
    public float current_min;
    /** The value that does not affect the tuned setting. */
    public float neutral_value;
    /** The value loaded from file (same as nominal_value unless the setting was saved with tuning parameter values). */
    public float loaded_value;
    /** The value that was most recently set. */
    public float set_value;
    /** The effective value, i.e. set_value clamped to the current min/max range at the time it was set. */
    public float effective_value;
    /** Max value affected by current setting. Can be lower than nominal_max. */
    public float current_max;
    /** High end of scale for any setting. Highest allowed value. */
    public float nominal_max;
    public CvieTCVIEDIInfo() {
        super();
    }
    /** Internal use. */
    protected List<String> getFieldOrder() {
        return Arrays.asList("nominal_min", "current_min", "neutral_value", "loaded_value", "set_value", "effective_value", "current_max", "nominal_max");
    }
    /**
     * @param nominal_min < Low end of scale for any setting. Lowest allowed value.<br>
     * @param current_min < Min value affected by current setting. Can be higher than nominal_min.<br>
     * @param neutral_value < The value that does not affect the tuned setting.<br>
     * @param loaded_value < The value loaded from file (same as nominal_value unless the setting was saved with tuning parameter values).<br>
     * @param set_value < The value that was most recently set.<br>
     * @param effective_value < The effective value, i.e. set_value clamped to the current min/max range at the time it was set.<br>
     * @param current_max < Max value affected by current setting. Can be lower than nominal_max.<br>
     * @param nominal_max < High end of scale for any setting. Highest allowed value.
     */
    public CvieTCVIEDIInfo(float nominal_min, float current_min, float neutral_value, float loaded_value, float set_value, float effective_value, float current_max, float nominal_max) {
        super();
        this.nominal_min = nominal_min;
        this.current_min = current_min;
        this.neutral_value = neutral_value;
        this.loaded_value = loaded_value;
        this.set_value = set_value;
        this.effective_value = effective_value;
        this.current_max = current_max;
        this.nominal_max = nominal_max;
    }
    /** Constructor
     *  @param peer   pointer to raw struct from C library
     */
    public CvieTCVIEDIInfo(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends CvieTCVIEDIInfo implements Structure.ByReference {
    };

    public static class ByValue extends CvieTCVIEDIInfo implements Structure.ByValue {
    };
}
