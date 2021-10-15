package cc.ioctl.nfcncihost.daemon;

import android.content.Context;

import androidx.annotation.NonNull;

import java.io.File;

import cc.ioctl.nfcncihost.daemon.internal.NciHostDaemonProxy;
import cc.ioctl.nfcncihost.procedure.BaseApplicationImpl;

public class IpcNativeHandler {

    private final long mNativeHandler;

    private static volatile boolean sInit = false;
    private static volatile String sDirPath = null;
    private static NciHostDaemonProxy mProxy = null;

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
        mProxy = new NciHostDaemonProxy();
        synchronized (IpcNativeHandler.class) {
            if (!sInit) {
                System.loadLibrary("nciclient");
                File dir = ctx.getNoBackupFilesDir();
                if (!dir.exists()) {
                    dir.mkdirs();
                }
                sDirPath = dir.getAbsolutePath();
                sInit = true;
            }
        }
    }

    public static void initForSocketDir() {
        checkProcess();
        ntInitForSocketDir(sDirPath);
    }

    private static void checkProcess() {
        if (!BaseApplicationImpl.isServiceProcess()) {
            throw new IllegalStateException("only daemon process may access IpcNativeHandler");
        }
    }

    public static INciHostDaemon peekConnection() {
        checkProcess();
        if (ntPeekConnection()) {
            return mProxy;
        }
        return null;
    }

    private static native void ntInitForSocketDir(String name);

    private static native boolean ntPeekConnection();

    private static native boolean ntRequestConnection();

    private static native boolean ntWaitForConnection(int timeout);

    public static boolean isConnected() {
        return ntPeekConnection();
    }

    public static INciHostDaemon connect(int timeout) {
        if (!sInit) {
            throw new IllegalStateException("attempt to connect before init");
        }
        if (ntPeekConnection()) {
            return mProxy;
        }
        ntRequestConnection();
        return ntWaitForConnection(timeout) ? mProxy : null;
    }

    public interface IpcConnectionListener {
        void onConnect(IpcNativeHandler h);

        void onDisconnect(IpcNativeHandler h);
    }
}
