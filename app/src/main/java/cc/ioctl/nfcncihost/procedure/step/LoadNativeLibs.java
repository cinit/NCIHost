package cc.ioctl.nfcncihost.procedure.step;

import android.content.Context;

import com.tencent.mmkv.MMKV;

import cc.ioctl.nfcncihost.procedure.BaseApplicationImpl;
import cc.ioctl.nfcncihost.procedure.Step;

public class LoadNativeLibs extends Step {
    @Override
    public boolean doStep() {
        Context ctx = BaseApplicationImpl.getInstance();
        System.loadLibrary("mmkv");
        MMKV.initialize(ctx);
        return true;
    }
}
