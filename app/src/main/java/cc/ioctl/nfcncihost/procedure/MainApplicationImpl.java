package cc.ioctl.nfcncihost.procedure;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import android.util.Log;

public class MainApplicationImpl extends WorkerApplicationImpl {

    public static StartupDirector sDirector;
    private static final String TAG = "MainApplicationImpl";

    @Override
    public void doOnCreate() {
        super.doOnCreate();
        sApplication = this;
        if (sDirector == null) {
            sDirector = StartupDirector.onAppStart();
        }
        Log.d(TAG, "onCreate: proc=" + getProcessName());
    }

    @Override
    public boolean onActivityCreate(Activity activity, Intent intent) {
        if (sDirector == null) {
            return false;
        }
        return sDirector.onActivityCreate(activity, intent);
    }

    public static MainApplicationImpl getInstance() {
        Application app = BaseApplicationImpl.sApplication;
        if (app == null) {
            throw new IllegalStateException("calling " + TAG + ".getInstance() before init");
        }
        if (app instanceof MainApplicationImpl) {
            return (MainApplicationImpl) app;
        } else {
            throw new IllegalStateException("calling " + TAG + ".getInstance() in wrong process");
        }
    }
}
