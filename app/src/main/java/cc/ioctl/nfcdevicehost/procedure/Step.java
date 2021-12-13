package cc.ioctl.nfcdevicehost.procedure;

import android.util.Log;

public abstract class Step implements Runnable {

    private static final String TAG = "StartupSteps";

    public abstract boolean doStep();

    public boolean step() {
        try {
            return doStep();
        } catch (LinkageError e) {
            Log.e(TAG, "step: " + this.getClass().getSimpleName(), e);
            return false;
        }
    }

    @Override
    public void run() {
        step();
    }
}
