package cc.ioctl.nfcncihost.daemon;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;

import java.io.File;
import java.io.IOException;

import cc.ioctl.nfcncihost.daemon.internal.NciHostDaemonProxy;
import cc.ioctl.nfcncihost.procedure.BaseApplicationImpl;

public class IpcNativeHandler {

    private static final String TAG = "IpcNativeHandler";
    private final long mNativeHandler;
    private static volatile boolean sInit = false;
    private static volatile String sDirPath = null;
    static NciHostDaemonProxy mProxy = null;
    static Thread sEventHandlerThread = null;

    private IpcNativeHandler(final long h) {
        mNativeHandler = h;
    }

    static class RemoteEventHandler implements Runnable {
        @Override
        public void run() {
            try {
                while (mProxy != null) {
                    NciHostDaemonProxy.NativeEventPacket packet = mProxy.waitForEvent();
                    if (packet != null) {
                        mProxy.dispatchRemoteEvent(packet);
                    } else {
                        throw new IOException("waitForEvent() returned null");
                    }
                }
            } catch (IOException e) {
                Log.e(TAG, "RemoteEventHandler: IOException: " + e.getMessage());
            }
        }
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
        startEventHandler();
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
        if (ntWaitForConnection(timeout)) {
            startEventHandler();
            return mProxy;
        } else {
            return null;
        }
    }

    static void startEventHandler() {
        synchronized (IpcNativeHandler.class) {
            if (sEventHandlerThread == null || !sEventHandlerThread.isAlive()) {
                sEventHandlerThread = new Thread(new RemoteEventHandler());
                sEventHandlerThread.start();
            }
        }
    }

    public interface IpcConnectionListener {
        void onConnect(IpcNativeHandler h);

        void onDisconnect(IpcNativeHandler h);
    }
}
