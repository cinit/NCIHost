package cc.ioctl.nfcncihost.daemon;

import android.content.Context;

import androidx.annotation.NonNull;

import java.io.File;

import cc.ioctl.nfcncihost.procedure.BaseApplicationImpl;

public class IpcNativeHandler {

    private final long mNativeHandler;

    private static volatile boolean sInit = false;

    private IpcNativeHandler(final long h) {
        mNativeHandler = h;
    }

    private IpcNativeHandler() {
        throw new AssertionError("No instance for you!");
    }

    public static boolean isInit() {
        return sInit;
    }

    public static void init(@NonNull Context ctx) {
        if (sInit) {
            return;
        }
        synchronized (IpcNativeHandler.class) {
            if (!sInit) {
                System.loadLibrary("nciclient");
                File dir = ctx.getNoBackupFilesDir();
                if (!dir.exists()) {
                    dir.mkdirs();
                }
                sInit = true;
            }
        }
    }

    public static void initForSocketDir(String name) {
        if (!name.startsWith("/")) {
            throw new IllegalArgumentException("absolute path required");
        }
        checkProcess();
        ntInitForSocketDir(name);
    }

    private static void checkProcess() {
        if (!BaseApplicationImpl.isDaemonProcess()) {
            throw new IllegalStateException("only daemon process may access IpcNativeHandler");
        }
    }

    public static IpcNativeHandler peekConnection() {
        checkProcess();
        return ntPeekConnection();
    }

    private static native void ntInitForSocketDir(String name);

    private static native IpcNativeHandler ntPeekConnection();

    public static boolean isConnected() {
        return peekConnection() != null;
    }

    public interface IpcConnectionListener {
        void onConnect(IpcNativeHandler h);

        void onDisconnect(IpcNativeHandler h);
    }


}
