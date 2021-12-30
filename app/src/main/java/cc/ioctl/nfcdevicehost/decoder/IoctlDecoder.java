package cc.ioctl.nfcdevicehost.decoder;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class IoctlDecoder {

    @NonNull
    public static String requestToString(int request) {
        int dir = (request >> 30) & 0x3;
        int size = (request >> 16) & 0x3f;
        int type = (request >> 8) & 0xff;
        int nr = request & 0xff;
        String dirStr;
        switch (dir) {
            case 0:
                dirStr = "IO";
                break;
            case 1:
                dirStr = "IOW";
                break;
            case 2:
                dirStr = "IOR";
                break;
            case 3:
                dirStr = "IOWR";
                break;
            default:
                // should never happen
                throw new IllegalArgumentException("Invalid direction");
        }
        String nrStr = nr < 10 ? String.valueOf(nr) : String.format(Locale.ROOT, "0x%02X", nr);
        return (dir == 0 && size == 0) ? String.format(Locale.ROOT, "%s(0x%02X, %s)", dirStr, type, nrStr)
                : String.format(Locale.ROOT, "%s(0x%02X, %s, %d)", dirStr, type, nrStr, size);
    }

    @Nullable
    public static String getIoctlRequestName(int request) {
        return IOCTL_NAMES.get(request);
    }

    public static int IO(int type, int nr) {
        return (0 << 30) | (0 << 16) | (type << 8) | nr;
    }

    public static int IOW(int type, int nr, int size) {
        return (1 << 30) | (size << 16) | (type << 8) | nr;
    }

    public static int IOR(int type, int nr, int size) {
        return (2 << 30) | (size << 16) | (type << 8) | nr;
    }

    public static int IOWR(int type, int nr, int size) {
        return (3 << 30) | (size << 16) | (type << 8) | nr;
    }

    private static final Map<Integer, String> IOCTL_NAMES = new HashMap<>();

    static {
        // /dev/nq-nci
        IOCTL_NAMES.put(IOW(0xE9, 0x01, 0x04), "NFC_SET_PWR");
        IOCTL_NAMES.put(IOW(0xE9, 0x02, 0x04), "ESE_SET_PWR");
        IOCTL_NAMES.put(IOR(0xE9, 0x03, 0x04), "ESE_GET_PWR");
        IOCTL_NAMES.put(IO(0xE9, 0x04), "NFC_GET_PLATFORM_TYPE");
        IOCTL_NAMES.put(IOW(0xE9, 0x05, 4), "NFC_GET_IRQ_STATE");
        // /dev/assd
        IOCTL_NAMES.put(IO('A', 0x00), "ASSD_IOC_ENABLE");
        IOCTL_NAMES.put(IOWR('A', 0x01, 4), "ASSD_IOC_TRANSCEIVE");
        IOCTL_NAMES.put(IOWR('A', 0x01, 8), "ASSD_IOC_TRANSCEIVE");
        IOCTL_NAMES.put(IO('A', 0x02), "ASSD_IOC_PROBE");
        IOCTL_NAMES.put(IOW('A', 0x03, 4), "ASSD_IOC_WAIT");
        IOCTL_NAMES.put(IOWR('A', 0x04, 4), "ASSD_IOC_SET_TIMEOUT");
        IOCTL_NAMES.put(IOR('A', 0x05, 4), "ASSD_IOC_GET_VERSION");
        IOCTL_NAMES.put(IOR('A', 0x05, 8), "ASSD_IOC_GET_VERSION");
    }
}
