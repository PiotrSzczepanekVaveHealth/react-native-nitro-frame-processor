package se.contextvision.cvie;

import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.NativeLongByReference;
import com.sun.jna.ptr.PointerByReference;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.List;

import static se.contextvision.cvie.CvieLibrary.CVIE_E_LICENSE_ERROR;
import static se.contextvision.cvie.CvieLibrary.CVIE_E_OK;

/**
 * \defgroup CVLM_java CVLM Java API
 *
 * These classes give access to the CVLM C API.  The high level interface, which is recommended,
 * is available in the Cvlm class, and the low level JNA wrapper is available in the CvlmLibrary
 * class.
 *
 * \see CVLM
 */

/**
 * High level Java interface for the CVLM API.
 *
 * For more details, see the C API reference (\ref CVLM).
 *
 * \ingroup CVLM_java
 */
public class Cvlm {
    /**
     * Initialization parameter for use with SetParameter().
     */
    public static final int LM_INIT = CvlmLibrary.LM_INIT;

    /**
     * Gets the list of supported Host Types for licensing.
     *
     * @see CVLMGetPossibleHostTypes()
     *
     * @return a list of possible Host Types (each Host Type's index is given by its position in
     * this list)
     */
    public static List<String> GetPossibleHostTypes() {
        PointerByReference hostTypesRef = new PointerByReference();
        CheckResult(CvlmLibrary.CVLMGetPossibleHostTypes(null, hostTypesRef));

        Pointer hostTypesArray = hostTypesRef.getValue();

        List<String> hostTypes = new ArrayList<>();

        for (int i=0; i < 16; i++) {
            String next = hostTypesArray.getPointer(i * Native.POINTER_SIZE).getString(0);

            if ("".equals(next))
                break;

            hostTypes.add(next);
        }

        return hostTypes;
    }

    /**
     * Gets the list of supported Product IDs for licensing.
     *
     * @see CVLMGetPossibleModules()
     *
     * @return a list of possible Product IDs (each Product ID's index is given by its position in
     * this list)
     */
    public static List<String> GetPossibleModules() {
        List<String> productIDs = new ArrayList<>();

        PointerByReference productIDsRef = new PointerByReference();
        CheckResult(CvlmLibrary.CVLMGetPossibleModules(null, productIDsRef));

        for (int i=0; i < 16; i++) {
            String next = productIDsRef.getValue().getPointer(i * Native.POINTER_SIZE).getString(0);

            if ("".equals(next))
                break;

            productIDs.add(next);
        }

        return productIDs;
    }

    /**
     * Sets a license parameter string value.
     *
     * \see CVLMSetParameterValue()
     *
     * @param parameter the parameter to modify
     * @param stringValue the string value to set
     */
    public static void SetParameterValue(int parameter, String stringValue) {
        if (stringValue != null)
            CheckResult(CvlmLibrary.CVLMSetParameterValue(parameter, stringToBytes(stringValue)));
        else
            CheckResult(CvlmLibrary.CVLMSetParameterValue(parameter, null));
    }

    /**
     * Gets the Host ID for a given Host Type.  The index is the position of the Host Type in the
     * list returned by GetPossibleHostTypes().
     *
     * \see CVLMGetHostId()
     *
     * @param hostTypeIndex the Host Type index
     * @return the Host ID
     */
    public static long GetHostId(int hostTypeIndex) {
        NativeLongByReference hostIdRef = new NativeLongByReference();
        CheckResult(CvlmLibrary.CVLMGetHostId(null, hostTypeIndex, hostIdRef));

        return hostIdRef.getValue().longValue();
    }

    /**
     * Checks the license activation status for a Product ID.  The index is the position of the
     * Product ID in the list returned by GetPossibleModules().
     *
     * \see CVLMCheckLicense()
     *
     * @param productIdIndex the index of the Product ID to check
     * @return true if the Product ID is activated, false otherwise
     */
    public static boolean CheckLicense(int productIdIndex) {
        int res = CvlmLibrary.CVLMCheckLicense(null, productIdIndex);

        if (res == CVIE_E_OK)
            return true;

        if (res == CVIE_E_LICENSE_ERROR)
            return false;

        // Otherwise, an unexpected error occurred, throw an exception
        CheckResult(res);
        return false;
    }

    /**
     * Sets the Activation Key for a Product ID.  The index is the position of the
     * Product ID in the list returned by GetPossibleModules().
     *
     * \see CVLMSetKey()
     *
     * @param productIdIndex the index of the Product ID to activate
     * @param activationKey the Activation Key to set
     * @return
     */
    public static boolean SetKey(int productIdIndex, String activationKey) {
        int res = CvlmLibrary.CVLMSetKey(null, productIdIndex, activationKey);

        if (res == CVIE_E_OK)
            return true;

        if (res == CVIE_E_LICENSE_ERROR)
            return false;

        // Otherwise, an unexpected error occurred, throw an exception
        CheckResult(res);
        return false;
    }

    /**
     * Gets a list of currently installed license information.
     *
     * @return a list of license information objects
     */
    public static List<LicenseInfo> GetRegisteredModules() {
        IntBuffer moduleIndexes = IntBuffer.allocate(17);
        IntBuffer hostTypeIndexes = IntBuffer.allocate(17);
        ByteBuffer info = ByteBuffer.allocate(1024);

        CheckResult(CvlmLibrary.CVLMGetRegisteredModules(null, moduleIndexes, hostTypeIndexes, info));

        List<LicenseInfo> licenseInfoList = new ArrayList<>();
        int infoPtr = 0;
        for (int i=0; moduleIndexes.get(i) != -1; i++) {
            int newInfoPtr = infoPtr;
            while (info.get(newInfoPtr) != '\0')
                newInfoPtr++;

            byte[] infoBytes = new byte[newInfoPtr - infoPtr];
            for (int ix = infoPtr; ix < newInfoPtr; ix++)
                infoBytes[ix - infoPtr] = info.get(ix);
            infoPtr = newInfoPtr + 1;

            licenseInfoList.add(new LicenseInfo(moduleIndexes.get(i), hostTypeIndexes.get(i), new String(infoBytes)));
        }

        return licenseInfoList;
    }

    /**
     * Uninstalls an activation key from a given Product ID and Host Type combination.
     *
     * \see CVLMUninstall()
     *
     * @param productIdIndex the index of the Product ID to uninstall
     * @param hostTypeIndex the index of the Host Type to uninstall
     */
    public static void Uninstall(int productIdIndex, int hostTypeIndex) {
        CheckResult(CvlmLibrary.CVLMUninstall(null, productIdIndex, hostTypeIndex));
    }

    /**
     * This class represents license information for a particular combination of Product ID and
     * Host Type.
     */
    public static class LicenseInfo {
        private final int productIdIndex;
        private final int hostTypeIndex;
        private final String info;

        LicenseInfo(int productIdIndex, int hostTypeIndex, String info) {
            this.productIdIndex = productIdIndex;
            this.hostTypeIndex = hostTypeIndex;
            this.info = info;
        }

        /**
         * @return the index of the Product ID which this object describes
         */
        public int getProductIdIndex() {
            return productIdIndex;
        }

        /**
         * @return the index of the Host Type which this object describes
         */
        public int getHostTypeIndex() {
            return hostTypeIndex;
        }

        /**
         * Returns information for the specific Product ID and Host Type combination.  The
         * information is a human-readable string, typically containing the Activation Key string
         * and expiry date.
         * @return the information string
         */
        public String getInfo() {
            return info;
        }
    }

    private static void CheckResult(int result) {
        if (result != CVIE_E_OK) {
            throw new CvieError(result);
        }
    }

    private static ByteBuffer stringToBytes(String s) {
        ByteBuffer bytes = ByteBuffer.allocate(s.getBytes().length + 1);
        bytes.put(s.getBytes());
        bytes.put((byte) 0);
        bytes.rewind();
        return bytes;
    }
}
