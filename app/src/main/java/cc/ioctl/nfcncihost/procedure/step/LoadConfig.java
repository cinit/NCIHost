package cc.ioctl.nfcncihost.procedure.step;

import com.tencent.mmkv.MMKV;

import cc.ioctl.nfcncihost.activity.config.ConfigManager;
import cc.ioctl.nfcncihost.procedure.Step;

public class LoadConfig extends Step {
    @Override
    public boolean doStep() {
        MMKV def = MMKV.mmkvWithID("default_config");
        ConfigManager.setDefaultConfig(def);
        return true;
    }
}
