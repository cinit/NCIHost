package cc.ioctl.nfcncihost.daemon;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;

import com.topjohnwu.superuser.Shell;

import java.io.File;
import java.io.IOException;
import java.util.HashSet;

import cc.ioctl.nfcncihost.NativeInterface;
import cc.ioctl.nfcncihost.daemon.internal.NciHostDaemonProxy;
import cc.ioctl.nfcncihost.procedure.BaseApplicationImpl;
import cc.ioctl.nfcncihost.util.NativeUtils;
import cc.ioctl.nfcncihost.util.RootShell;
import cc.ioctl.nfcncihost.util.ThreadManager;

public class IpcNativeHandler {

    private static final String TAG = "IpcNativeHandler";
    private static volatile boolean sInit = false;
    private static volatile String sDirPath = null;
    static NciHostDaemonProxy mProxy = null;
    static Thread sEventHandlerThread = null;
    static final HashSet<IpcConnectionListener> sConnectionListeners = new HashSet<>();
    static boolean sIsConnected = false;

    static class RemoteEventHandler implements Runnable {
        @Override
        public void run() {
            try {
                while (mProxy != null) {
                    NciHostDaemonProxy.NativeEventPacket packet = mProxy.waitForEvent();
                    if (packet != null) {
                        mProxy.dispatchRemoteEvent(packet);
                    } else {
                        // null packet means a daemon connected or disconnected event
                        HashSet<IpcConnectionListener> listeners = null;
                        final boolean isConnected = ntPeekConnection();
                        synchronized (sConnectionListeners) {
                            if (isConnected != sIsConnected) {
                                sIsConnected = isConnected;
                                // notify the listeners
                                listeners = new HashSet<>(sConnectionListeners);
                            }
                        }
                        if (listeners != null) {
                            HashSet<IpcConnectionListener> finalListeners = listeners;
                            ThreadManager.async(() -> {
                                for (IpcConnectionListener listener : finalListeners) {
                                    if (isConnected) {
                                        listener.onConnect(mProxy);
                                    } else {
                                        listener.onDisconnect(mProxy);
                                    }
                                }
                            });
                        }
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

    private static native int getIpcPidFileFD();

    /**
     * Get the architecture of the kernel by uname system call.
     * They are: "arm", "aarch64", "x86", "x86_64", "mips", etc.
     *
     * @return the architecture string
     */
    public static native String getKernelArchitecture();

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
        initForSocketDir();
        ntRequestConnection();
        long start = System.currentTimeMillis();
        boolean connected = ntWaitForConnection(20);
        if (!connected) {
            ntRequestConnection();
            connected = ntWaitForConnection(Math.max(timeout - 20, 20));
        }
        long delta = System.currentTimeMillis() - start;
        Log.d(TAG, "connect: " + (connected ? "success" : "fail") + " in " + delta + "ms");
        if (connected) {
            startEventHandler();
            return mProxy;
        } else {
            return null;
        }
    }

    private static File extractDaemonExecutable() {
        Context ctx = BaseApplicationImpl.getInstance();
        String kernelArch = getKernelArchitecture();
        // check the Android native library directory
        File appLib = new File(ctx.getApplicationInfo().nativeLibraryDir, "libncihostd.so");
        if (appLib.exists()) {
            // check architecture
            try {
                int elfArch = NativeUtils.getElfArch(appLib.getAbsoluteFile());
                if (elfArch != NativeUtils.archStringToInt(kernelArch)) {
                    Log.e(TAG, "libncihostd.so is not for arch " + kernelArch + ", got " + elfArch);
                }
                // check executable bit
                if (appLib.canExecute()) {
                    return appLib;
                } else {
                    Log.e(TAG, "libncihostd.so is not executable");
                }
            } catch (IOException e) {
                Log.e(TAG, "get app lib ELF arch: IOException: " + e.getMessage());
            }
        }
        // we don't have the right libncihostd.so, so try to extract it
        String abiName = NativeUtils.archIntToAndroidLibArch(NativeUtils.archStringToInt(kernelArch));
        return NativeInterface.extractNativeLibFromApk("libncihostd.so", abiName, "ncihostd", true);
    }

    /**
     * Start the daemon with root privileges and connect to it.
     *
     * @return the daemon instance
     * @throws RootShell.NoRootShellException if required root shell is not available
     * @throws IOException                    if any other error occurs
     */
    public static INciHostDaemon startDaemonWithRootShell() throws RootShell.NoRootShellException, IOException {
        if (!sInit) {
            throw new IllegalStateException("attempt to connect before init");
        }
        initForSocketDir();
        // if the daemon is already running, return it
        INciHostDaemon daemon = connect(300);
        if (daemon != null) {
            return daemon;
        }
        File daemonExecutable = extractDaemonExecutable();
        int myPid = android.os.Process.myPid();
        int myUid = android.os.Process.myUid();
        int pidFileFd = getIpcPidFileFD();
        // test /proc/<pid>/fd/<fd>
        if (!new File("/proc/" + myPid + "/fd/" + pidFileFd).exists()) {
            throw new IllegalStateException("unable to access /proc/" + myPid + "/fd/" + pidFileFd);
        }
        // start the daemon with root shell, exec("ncihostd", uid, pid, fd)
        Shell.Result result = RootShell.executeWithRoot(daemonExecutable.getAbsolutePath(),
                String.valueOf(myUid), String.valueOf(myPid), String.valueOf(pidFileFd));
        if (!result.isSuccess()) {
            StringBuilder sb = new StringBuilder();
            sb.append("failed to start daemon with root shell, exit code: ").append(result.getCode()).append("\n");
            sb.append("stdout: ");
            for (String line : result.getOut()) {
                sb.append(line).append("\n");
            }
            sb.append("stderr: ");
            for (String line : result.getErr()) {
                sb.append(line).append("\n");
            }
            throw new IOException(sb.toString());
        }
        // connect to the daemon
        daemon = connect(1000);
        if (daemon == null) {
            throw new IOException("failed to connect to daemon after starting with root shell");
        }
        return daemon;
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
        void onConnect(INciHostDaemon daemon);

        void onDisconnect(INciHostDaemon daemon);
    }

    public static void registerConnectionListener(IpcConnectionListener listener) {
        synchronized (sConnectionListeners) {
            sConnectionListeners.add(listener);
        }
    }

    public static boolean unregisterConnectionListener(IpcConnectionListener listener) {
        synchronized (sConnectionListeners) {
            return sConnectionListeners.remove(listener);
        }
    }
}
