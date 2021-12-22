package cc.ioctl.nfcdevicehost.util.config;

import androidx.annotation.NonNull;

public class NullableMmkvConfigItem<T> extends BaseMmkvConfigItem<T> implements NullableConfigItem<T> {
    public NullableMmkvConfigItem(@NonNull ConfigManager cfg, @NonNull String key, T defaultValue) {
        super(cfg, key, defaultValue, false);
    }
}
