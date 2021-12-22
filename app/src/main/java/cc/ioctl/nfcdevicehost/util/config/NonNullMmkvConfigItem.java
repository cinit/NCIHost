package cc.ioctl.nfcdevicehost.util.config;

import androidx.annotation.NonNull;

import java.util.Objects;

public class NonNullMmkvConfigItem<T> extends BaseMmkvConfigItem<T> implements NonNullConfigItem<T> {
    public NonNullMmkvConfigItem(@NonNull ConfigManager cfg, @NonNull String key, @NonNull T defaultValue) {
        super(cfg, key, defaultValue, true);
    }

    @Override
    @NonNull
    public T getValue() {
        return Objects.requireNonNull(super.getValue(), "NonNullMmkvConfigItem getValue is null");
    }

    @Override
    public void setValue(@NonNull T value) {
        Objects.requireNonNull(value, "NonNullMmkvConfigItem value is null");
        super.setValue(value);
    }
}
