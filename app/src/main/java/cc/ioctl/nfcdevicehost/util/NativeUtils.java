package cc.ioctl.nfcdevicehost.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Objects;

/**
 * Utility class for native related operations.
 */
public class NativeUtils {
    /**
     * constants from ELF format
     */
    public static final int ARCH_X86 = 3;
    public static final int ARCH_X86_64 = 62;
    public static final int ARCH_ARM = 40;
    public static final int ARCH_AARCH64 = 183;

    public static int archStringToInt(String arch) {
        Objects.requireNonNull(arch, "arch cannot be null");
        switch (arch) {
            case "x86":
            case "i386":
            case "i486":
            case "i586":
            case "i686":
                return ARCH_X86;
            case "x86_64":
            case "amd64":
                return ARCH_X86_64;
            case "arm":
            case "armhf":
            case "armeabi":
            case "armeabi-v7a":
                return ARCH_ARM;
            case "aarch64":
            case "arm64":
            case "arm64-v8a":
            case "armv8l":
                return ARCH_AARCH64;
            default:
                throw new IllegalArgumentException("unsupported arch: " + arch);
        }
    }

    public static String archIntToAndroidLibArch(int arch) {
        switch (arch) {
            case ARCH_X86:
                return "x86";
            case ARCH_X86_64:
                return "x86_64";
            case ARCH_ARM:
                // Anyone use armeabi but not armeabi-v7a?
                return "armeabi-v7a";
            case ARCH_AARCH64:
                return "arm64-v8a";
            default:
                throw new IllegalArgumentException("unsupported arch: " + arch);
        }
    }

    /**
     * Get the architecture of the ELF file.
     * Currently only support little endian 32-bit and 64-bit.
     *
     * @param elfHeader the ELF header
     * @return the architecture of the ELF file
     */
    public static int getElfArch(byte[] elfHeader) {
        if (elfHeader[0] == 0x7F && elfHeader[1] == 'E' && elfHeader[2] == 'L' && elfHeader[3] == 'F') {
            byte type = elfHeader[4];
            if (type == 1 || type == 2) {
                // e_machine is at offset 18 in the ELF header
                return ByteUtils.readInt16(elfHeader, 18);
            } else {
                throw new IllegalArgumentException("ELF type is not supported");
            }
        } else {
            throw new IllegalArgumentException("Invalid ELF header");
        }
    }

    public static int getElfArch(File file) throws IOException {
        try (InputStream is = new FileInputStream(file)) {
            // read the ELF header, 64 bytes is enough
            byte[] elfHeader = new byte[64];
            int i;
            int size = 0;
            while ((i = is.read(elfHeader, size, elfHeader.length - size)) > 0) {
                size += i;
                if (size == elfHeader.length) {
                    break;
                }
            }
            return getElfArch(elfHeader);
        }
    }

    /**
     * Get the architecture of the current process.
     * This method is only available on Linux.
     *
     * @return the architecture of the current process
     * @throws RuntimeException if /proc/self/exe is not accessible
     */
    public static int getCurrentProcessArch() {
        try (InputStream is = new FileInputStream("/proc/self/exe")) {
            // read the ELF header, 64 bytes is enough
            byte[] elfHeader = new byte[64];
            int i;
            int size = 0;
            while ((i = is.read(elfHeader, size, elfHeader.length - size)) > 0) {
                size += i;
                if (size == elfHeader.length) {
                    break;
                }
            }
            return getElfArch(elfHeader);
        } catch (IOException e) {
            // this usually doesn't happen, since /proc/self/exe is usually app_process
            throw new RuntimeException("/proc/self/exe is not accessible", e);
        }
    }
}
