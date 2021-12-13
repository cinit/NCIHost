package cc.ioctl.nfcdevicehost.procedure;

import android.app.Application;
import android.util.Log;

import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.util.ThreadManager;

public class WorkerApplicationImpl extends BaseApplicationImpl {
    private static final String TAG = "WorkerApplicationImpl";
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

    public static WorkerApplicationImpl getInstance() {
        Application app = BaseApplicationImpl.sApplication;
        if (app == null) {
            throw new IllegalStateException("calling WorkerApplicationImpl.getInstance() before init");
        }
        if (app instanceof WorkerApplicationImpl) {
            return (WorkerApplicationImpl) app;
        } else {
            throw new IllegalStateException("calling WorkerApplicationImpl.getInstance() in wrong process");
        }
    }

    public static boolean isInServiceProcess() {
        BaseApplicationImpl app = BaseApplicationImpl.sApplication;
        if (app == null) {
            throw new IllegalStateException("calling WorkerApplicationImpl.isInServiceProcess() before init");
        }
        return app instanceof WorkerApplicationImpl;
    }
}
