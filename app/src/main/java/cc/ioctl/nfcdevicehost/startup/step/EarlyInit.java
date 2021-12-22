package cc.ioctl.nfcdevicehost.startup.step;

import android.content.Context;

import com.tencent.mmkv.MMKV;

import cc.ioctl.nfcdevicehost.util.config.ConfigManager;
import cc.ioctl.nfcdevicehost.ipc.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.startup.BaseApplicationImpl;
import cc.ioctl.nfcdevicehost.startup.Step;

public class EarlyInit extends Step {
    @Override
    public boolean doStep() {
        // init mmkv config
        Context ctx = BaseApplicationImpl.getInstance();
        System.loadLibrary("mmkv");
        MMKV.initialize(ctx);
        MMKV.mmkvWithID("default_config");
        ConfigManager.getDefaultConfig();
        ConfigManager.getCache();
        // if in service process, init the NCI daemon IPC
        if (BaseApplicationImpl.isServiceProcess()) {
            IpcNativeHandler.init(ctx);
            IpcNativeHandler.initForSocketDir();
        }
        return true;
    }
}
