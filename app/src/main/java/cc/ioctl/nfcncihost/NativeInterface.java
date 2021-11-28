package cc.ioctl.nfcncihost;

import android.content.Context;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import cc.ioctl.nfcncihost.procedure.BaseApplicationImpl;

public class NativeInterface {

    public enum ABI {
        ABI_ARM_32,
        ABI_ARM_64,
        ABI_X86,
        ABI_X86_64,
    }

    public enum NfcHalServicePatch {
        NXP_PATCH,
        ST_PATCH,
    }

    private NativeInterface() {
        throw new AssertionError("No instance for you!");
    }

    private static void deleteDir(File dir) {
        if (dir.isDirectory()) {
            File[] files = dir.listFiles();
            for (File file : files) {
                deleteDir(file);
            }
        }
        dir.delete();
    }

    public static File getNfcHalServicePatchFile(NfcHalServicePatch patch, ABI abi) {
        String patchFileName;
        switch (patch) {
            case NXP_PATCH:
                patchFileName = "libnxphalpatch.so";
                break;
            case ST_PATCH:
                patchFileName = "libsthalpatch.so";
                break;
            default:
                throw new IllegalArgumentException("Unknown patch type");
        }
        String abiName;
        switch (abi) {
            case ABI_ARM_32:
                abiName = "armeabi-v7a";
                break;
            case ABI_ARM_64:
                abiName = "arm64-v8a";
                break;
            case ABI_X86:
                abiName = "x86";
                break;
            case ABI_X86_64:
                abiName = "x86_64";
                break;
            default:
                throw new IllegalArgumentException("Unknown ABI type");
        }
        Context context = BaseApplicationImpl.getInstance();
        File mainDir = new File(context.getFilesDir(), "hal_patch");
        if (!mainDir.exists() && !mainDir.mkdirs()) {
            throw new IllegalStateException("Failed to create hal_patch directory");
        }
        // clean up old patch files
        File[] files = mainDir.listFiles();
        final String patchVersion = BuildConfig.BUILD_UUID;
        if (files != null) {
            for (File file : files) {
                if (file.isDirectory() && !file.getName().equals(patchVersion)) {
                    deleteDir(file);
                }
            }
        }
        File patchDir = new File(mainDir, patchVersion + File.separator + abiName);
        if (!patchDir.exists() && !patchDir.mkdirs()) {
            throw new IllegalStateException("Failed to create patch directory");
        }
        File patchFile = new File(patchDir, patchFileName);
        if (!patchFile.exists()) {
            // extract patch file from apk lib directory
            try (FileOutputStream fos = new FileOutputStream(patchFile);
                 InputStream is = NativeInterface.class
                         .getResourceAsStream("/lib/" + abiName + "/" + patchFileName)) {
                if (is == null) {
                    throw new IllegalStateException("Failed to read patch file from apk");
                }
                byte[] buffer = new byte[4096];
                int len;
                while ((len = is.read(buffer)) > 0) {
                    fos.write(buffer, 0, len);
                }
                fos.flush();
            } catch (IOException e) {
                throw new IllegalStateException("Failed to extract patch file from apk", e);
            }
        }
        return patchFile;
    }

}
