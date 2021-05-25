package cc.ioctl.nfcncihost.procedure;

import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

@RequiresApi(api = Build.VERSION_CODES.Q)
@SuppressWarnings("unused")
public class ZygotePreloadTest implements android.app.ZygotePreload {
    @Override
    public void doPreload(@NonNull ApplicationInfo appInfo) {
        Log.i("ZygotePreloadTest", "doPreload");
    }
}
