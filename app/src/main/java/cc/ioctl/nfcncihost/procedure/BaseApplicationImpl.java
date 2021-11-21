package cc.ioctl.nfcncihost.procedure;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.Application;
import android.content.Context;
import android.content.Intent;

import java.util.List;

public abstract class BaseApplicationImpl extends Application {

    protected static BaseApplicationImpl sApplication;
    private static volatile String sProcName = null;
    private static final String PROC_SUFFIX_DAEMON = ":service";
    private static final String TAG = "BaseApplicationImpl";

    public void attachToContextImpl(Context base) {
        if (base instanceof BaseApplicationDelegate) {
            throw new StackOverflowError("BaseApplicationImpl.attachToContextImpl(Context)V");
        }
        super.attachBaseContext(base);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        doOnCreate();
    }

    public void doOnCreate() {
        // override this method in implement
    }

    @SuppressWarnings("unused")
    public boolean onActivityCreate(Activity activity, Intent intent) {
        return false;
    }

    public static boolean isServiceProcess() {
        return BaseApplicationImpl.sApplication instanceof WorkerApplicationImpl;
    }

    public static boolean isMainProcess() {
        return !getProcessName().contains(":");
    }

    public static String getProcessName() {
        if (sProcName != null) {
            return sProcName;
        }
        List<ActivityManager.RunningAppProcessInfo> runningAppProcesses =
                ((ActivityManager) sApplication.getSystemService(Context.ACTIVITY_SERVICE))
                        .getRunningAppProcesses();
        if (runningAppProcesses != null) {
            for (ActivityManager.RunningAppProcessInfo runningAppProcessInfo : runningAppProcesses) {
                if (runningAppProcessInfo != null
                        && runningAppProcessInfo.pid == android.os.Process.myPid()) {
                    sProcName = runningAppProcessInfo.processName;
                    return runningAppProcessInfo.processName;
                }
            }
        }
        return sProcName;
    }

    public static BaseApplicationImpl getInstance() {
        BaseApplicationImpl app = BaseApplicationImpl.sApplication;
        if (app == null) {
            throw new IllegalStateException("calling " + TAG + ".getInstance() before init");
        }
        return app;
    }
}
