package cc.ioctl.nfcncihost.procedure;

import android.app.Application;
import android.util.Log;

import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;
import cc.ioctl.nfcncihost.util.ThreadManager;

public class DaemonApplicationImpl extends BaseApplicationImpl {
    private static final String TAG = "DaemonApplicationImpl";
    final Object mNativeLock = new Object();

    @Override
    public void doOnCreate() {
        super.doOnCreate();
        Log.d(TAG, "doOnCreate");
    }

    public void initIpcSocketAsync() {
        if (!IpcNativeHandler.isInit()) {
            ThreadManager.execute(() -> {
                IpcNativeHandler.init(this);
                IpcNativeHandler.initForSocketDir();
            });
        }
    }

    public static DaemonApplicationImpl getInstance() {
        Application app = BaseApplicationImpl.sApplication;
        if (app == null) {
            throw new IllegalStateException("calling " + TAG + ".getInstance() before init");
        }
        if (app instanceof DaemonApplicationImpl) {
            return (DaemonApplicationImpl) app;
        } else {
            throw new IllegalStateException("calling " + TAG + ".getInstance() in wrong process");
        }
    }
}
