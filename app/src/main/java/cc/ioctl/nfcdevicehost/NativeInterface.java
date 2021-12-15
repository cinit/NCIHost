package cc.ioctl.nfcdevicehost;

import android.content.Context;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import cc.ioctl.nfcdevicehost.procedure.BaseApplicationImpl;
import cc.ioctl.nfcdevicehost.util.NativeUtils;

public class NativeInterface {

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

    /**
     * Extracts the native library from the APK and returns the file.
     *
     * @param libName    the soname, eg. "libnfc-nci.so"
     * @param abiName    the ABI name, eg. "armeabi-v7a"
     * @param dirName    the internal directory name, eg. "hal_patch"
     * @param executable whether to chmod +x the file
     * @return the extracted file
     */
    public static File extractNativeLibFromApk(String libName, String abiName, String dirName, boolean executable) {
        Context context = BaseApplicationImpl.getInstance();
        File mainDir = new File(context.getFilesDir(), dirName);
        if (!mainDir.exists() && !mainDir.mkdirs()) {
            throw new IllegalStateException("Failed to create " + dirName + " directory");
        }
        // clean up old patch files
        File[] files = mainDir.listFiles();
        final String currentVersion = BuildConfig.BUILD_UUID;
        if (files != null) {
            for (File file : files) {
                if (file.isDirectory() && !file.getName().equals(currentVersion)) {
                    deleteDir(file);
                }
            }
        }
        File patchDir = new File(mainDir, currentVersion + File.separator + abiName);
        if (!patchDir.exists() && !patchDir.mkdirs()) {
            throw new IllegalStateException("Failed to create patch directory");
        }
        File patchFile = new File(patchDir, libName);
        if (!patchFile.exists()) {
            // extract patch file from apk lib directory
            try (FileOutputStream fos = new FileOutputStream(patchFile);
                 InputStream is = NativeInterface.class.getResourceAsStream("/lib/" + abiName + "/" + libName)) {
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
        if (executable) {
            if (!patchFile.setExecutable(true, false) && !patchFile.canExecute()) {
                throw new IllegalStateException("Failed to set executable flag on " + patchFile.getAbsolutePath());
            }
        }
        return patchFile;
    }

    public static File getNfcHalServicePatchFile(NfcHalServicePatch patch, int abi) {
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
        String abiName = NativeUtils.archIntToAndroidLibArch(abi);
        return extractNativeLibFromApk(patchFileName, abiName, "hal_patch", false);
    }

}
