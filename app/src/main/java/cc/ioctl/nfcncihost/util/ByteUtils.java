package cc.ioctl.nfcncihost.util;

public class ByteUtils {

    public static int readInt16(byte[] data, int offset) {
        return ((data[offset + 1] & 0xFF) << 8) | (data[offset] & 0xFF);
    }

    public static int readInt32(byte[] data, int offset) {
        return ((data[offset + 3] & 0xFF) << 24) | ((data[offset + 2] & 0xFF) << 16)
                | ((data[offset + 1] & 0xFF) << 8) | (data[offset] & 0xFF);
    }

    public static long readInt64(byte[] data, int offset) {
        return ((long) readInt32(data, offset + 4) << 32) | (readInt32(data, offset) & 0xFFFFFFFFL);
    }

    public static String bytesToHexString(byte[] bytes) {
        if (bytes == null) {
            return "";
        }
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02X ", b));
        }
        return sb.toString();
    }

    public static boolean isPrintable(byte[] bytes) {
        if (bytes == null) {
            return false;
        }
        for (byte b : bytes) {
            if (b < 0x20 || b > 0x7E) {
                return false;
            }
        }
        return true;
    }
}
