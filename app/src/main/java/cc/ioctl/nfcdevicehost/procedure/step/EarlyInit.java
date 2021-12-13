package cc.ioctl.nfcdevicehost.procedure.step;

import android.content.Context;

import com.tencent.mmkv.MMKV;

import cc.ioctl.nfcdevicehost.activity.config.ConfigManager;
import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.procedure.BaseApplicationImpl;
import cc.ioctl.nfcdevicehost.procedure.Step;

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
