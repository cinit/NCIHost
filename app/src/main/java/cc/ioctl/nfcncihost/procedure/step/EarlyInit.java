package cc.ioctl.nfcncihost.procedure.step;

import android.content.Context;

import com.tencent.mmkv.MMKV;

import cc.ioctl.nfcncihost.activity.config.ConfigManager;
import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;
import cc.ioctl.nfcncihost.procedure.BaseApplicationImpl;
import cc.ioctl.nfcncihost.procedure.Step;

public class EarlyInit extends Step {
    @Override
    public boolean doStep() {
        // init mmkv config
        Context ctx = BaseApplicationImpl.getInstance();
        System.loadLibrary("mmkv");
        MMKV.initialize(ctx);
        MMKV def = MMKV.mmkvWithID("default_config");
        ConfigManager.setDefaultConfig(def);
        // if in service process, init the NCI daemon IPC
        if (BaseApplicationImpl.isServiceProcess()) {
            IpcNativeHandler.init(ctx);
            IpcNativeHandler.initForSocketDir();
        }
        return true;
    }
}
