package cc.ioctl.nfcncihost.procedure.step;

import android.content.Context;

import com.tencent.mmkv.MMKV;

import java.io.File;

import cc.ioctl.nfcncihost.procedure.BaseApplicationDelegate;
import cc.ioctl.nfcncihost.procedure.Step;

public class LoadNativeLibs extends Step {
    @Override
    public boolean doStep() {
        Context ctx = BaseApplicationDelegate.sApplication;
        System.loadLibrary("mmkv");
        MMKV.initialize(ctx);
        return true;
    }
}
