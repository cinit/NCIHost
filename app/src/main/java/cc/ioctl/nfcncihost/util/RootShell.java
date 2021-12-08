package cc.ioctl.nfcncihost.util;

import android.os.Looper;
import android.util.Log;

import com.topjohnwu.superuser.Shell;

import java.io.IOException;
import java.util.ArrayList;

public class RootShell {

    private static final String TAG = "RootShell";

    public static class NoRootShellException extends Exception {
        private static final long serialVersionUID = 1L;
    }

    /**
     * Execute a command in root shell.
     * This method will request a root shell and wait for the user to grant permission.
     * The root shell is not cached and each call will create a new root shell.
     * Note: This method MUST NOT be called from the main thread.
     *
     * @param path The executable path.
     * @param args The arguments to pass to the command.
     * @return Result of the command
     * @throws NoRootShellException if we cannot get root shell
     */
    public static Shell.Result executeWithRoot(String path, String... args) throws NoRootShellException {
        // Check if the path are absolute
        if (path.charAt(0) != '/') {
            throw new IllegalArgumentException("path must be absolute");
        }
        StringBuilder sb = new StringBuilder();
        sb.append('"');
        sb.append(path);
        sb.append('"');
        if (args != null && args.length > 0) {
            for (String arg : args) {
                sb.append(' ');
                sb.append('"');
                sb.append(arg);
                sb.append('"');
            }
        }
        // check if we are on the main thread
        if (Thread.currentThread() == Looper.getMainLooper().getThread()) {
            throw new IllegalStateException("RootShell.executeAbsWithRoot shall not be called from the main thread");
        }
        // request root shell
        Shell shell = Shell.Builder.create().build();
        if (!shell.isRoot()) {
            try {
                shell.close();
            } catch (IOException ignored) {
            }
            throw new NoRootShellException();
        }
        ArrayList<String> stdout = new ArrayList<>();
        ArrayList<String> stderr = new ArrayList<>();
        // wait for the result
        Shell.Result result = shell.newJob().add(sb.toString()).to(stdout, stderr).exec();
        try {
            shell.waitAndClose();
        } catch (IOException e) {
            Log.e(TAG, "Failed to wait for root shell", e);
        }
        return result;
    }

}
