package cc.ioctl.nfcncihost.ipc;

public class IpcNativeHandle {

    private final long mNativeHandle;

    private IpcNativeHandle(final long h) {
        mNativeHandle = h;
    }

    private IpcNativeHandle() {
        throw new AssertionError("No instance for you!");
    }

    public static native void initForSocketDir(String name);

    public static native IpcNativeHandle peekConnection();

    public static boolean isConnected() {
        return peekConnection() != null;
    }

    public interface IpcConnectionListener {
        void onConnect(IpcNativeHandle h);

        void onDisconnect(IpcNativeHandle h);
    }


}
