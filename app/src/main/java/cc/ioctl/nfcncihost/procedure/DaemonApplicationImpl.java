package cc.ioctl.nfcncihost.procedure;

import android.util.Log;

import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;
import cc.ioctl.nfcncihost.util.ThreadManager;

public class DaemonApplicationImpl extends BaseApplicationImpl {
    private static final String TAG = "DaemonApplicationImpl";
    final Object mNativeLock = new Object();

    public static DaemonApplicationImpl get() {
        return (DaemonApplicationImpl) BaseApplicationImpl.sApplication;
    }

    @Override
    public void doOnCreate() {
        super.doOnCreate();
        Log.d(TAG, "doOnCreate");
    }

    public void initIpcSocketAsync() {
        if (!IpcNativeHandler.isInit()) {
            ThreadManager.execute(() -> IpcNativeHandler.init(this));
        }
    }
}
