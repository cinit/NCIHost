package cc.ioctl.nfcncihost.procedure.step;

import android.content.Context;

import com.tencent.mmkv.MMKV;

import java.io.File;

import cc.ioctl.nfcncihost.BaseApplication;
import cc.ioctl.nfcncihost.ipc.IpcNativeHandle;
import cc.ioctl.nfcncihost.procedure.Step;

public class LoadNativeLibs extends Step {
    @Override
    public boolean doStep() {
        Context ctx = BaseApplication.sApplication;
        System.loadLibrary("mmkv");
        MMKV.initialize(ctx);
        File dir = ctx.getNoBackupFilesDir();
        if (!dir.exists()) {
            dir.mkdirs();
        }
        System.loadLibrary("nciclient");
        IpcNativeHandle.initForSocketDir(dir.getAbsolutePath());
//        try {
//            Thread.sleep(300);
//        } catch (InterruptedException e) {
//            e.printStackTrace();
//        }
        return true;
    }
}
