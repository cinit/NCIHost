package cc.ioctl.nfcncihost.procedure;

import android.util.Log;

public class DaemonApplicationImpl extends BaseApplicationImpl {
    private static final String TAG = "DaemonApplicationImpl";

    @Override
    public void doOnCreate() {
        super.doOnCreate();
        Log.d(TAG, "doOnCreate");
    }

}
