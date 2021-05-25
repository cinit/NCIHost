package cc.ioctl.nfcncihost;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import android.os.Build;
import android.util.Log;

import org.lsposed.hiddenapibypass.HiddenApiBypass;

import cc.ioctl.nfcncihost.procedure.StartupDirector;

public class BaseApplication extends Application {

    public static BaseApplication sApplication;
    public static StartupDirector sDirector;

    @Override
    public void onCreate() {
        super.onCreate();
        sApplication = this;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            try {
                HiddenApiBypass.setHiddenApiExemptions("L");
            } catch (NoClassDefFoundError e) {
                Log.e("HiddenApiBypass", "failed, SDK=" + Build.VERSION.SDK_INT, e);
            }
        }
        if (sDirector == null) {
            sDirector = StartupDirector.onAppStart();
        }
    }

    public boolean onActivityCreate(Activity activity, Intent intent) {
        if (sDirector == null) {
            return false;
        }
        return sDirector.onActivityCreate(activity, intent);
    }

}
