package cc.ioctl.nfcncihost.procedure;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.Application;
import android.content.Context;
import android.content.Intent;

import java.util.List;

public abstract class BaseApplicationImpl extends Application {

    public static BaseApplicationImpl sApplication;
    private static volatile String mProcName = null;
    private static final String PROC_SUFFIX_DAEMON = ":daemon";
    private static final String TAG = "BaseApplicationImpl";

    public void attachToContextImpl(Context base) {
        if (base instanceof BaseApplicationDelegate) {
            throw new IllegalArgumentException("recursion ahead");
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

    public static boolean isDaemonProcess() {
        return getProcessName().endsWith(PROC_SUFFIX_DAEMON);
    }

    public static boolean isMainProcess() {
        return !getProcessName().contains(":");
    }

    public static String getProcessName() {
        if (mProcName != null) {
            return mProcName;
        }
        List<ActivityManager.RunningAppProcessInfo> runningAppProcesses =
                ((ActivityManager) sApplication.getSystemService(Context.ACTIVITY_SERVICE))
                        .getRunningAppProcesses();
        if (runningAppProcesses != null) {
            for (ActivityManager.RunningAppProcessInfo runningAppProcessInfo : runningAppProcesses) {
                if (runningAppProcessInfo != null
                        && runningAppProcessInfo.pid == android.os.Process.myPid()) {
                    mProcName = runningAppProcessInfo.processName;
                    return runningAppProcessInfo.processName;
                }
            }
        }
        return mProcName;
    }
}
