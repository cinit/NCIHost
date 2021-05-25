package cc.ioctl.nfcncihost.activity.config;

import com.tencent.mmkv.MMKV;

public class ConfigManager {
    private static MMKV defaultConfig;

    public static MMKV getDefaultConfig() {
        return defaultConfig;
    }

    public static void setDefaultConfig(MMKV defaultConfig) {
        ConfigManager.defaultConfig = defaultConfig;
    }
}
