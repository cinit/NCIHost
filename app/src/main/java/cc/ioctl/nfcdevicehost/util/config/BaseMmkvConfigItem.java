package cc.ioctl.nfcdevicehost.util.config;

import androidx.annotation.NonNull;

import java.util.Objects;

public abstract class BaseMmkvConfigItem<T> extends AbsObservableItem<T> {
    @NonNull
    private final ConfigManager mConfig;
    private final T mDefaultValue;
    @NonNull
    private final String mKey;
    private final boolean mValueIsNonNull;

    protected BaseMmkvConfigItem(@NonNull ConfigManager cfg, @NonNull String key, T defaultValue, boolean valueIsNonNull) {
        super();
        Objects.requireNonNull(cfg);
        Objects.requireNonNull(key);
        mConfig = cfg;
        mKey = key;
        mDefaultValue = defaultValue;
        mValueIsNonNull = valueIsNonNull || (this instanceof NonNullConfigItem);
        if (mValueIsNonNull && defaultValue == null) {
            throw new IllegalArgumentException("defaultValue can't be null");
        }
        if ((this instanceof NullableConfigItem) && (this instanceof NonNullConfigItem)) {
            throw new IllegalArgumentException("NullableConfigItem and NonNullConfigItem can't be both");
        }
        notifyItemChanged(getValue());
    }

    @Override
    public T getValue() {
        Object value = mConfig.getOrDefault(mKey, mDefaultValue);
        return (mValueIsNonNull && value == null) ? mDefaultValue : (T) value;
    }

    @Override
    protected void setValueInternal(T value) {
        if (mValueIsNonNull && value == null) {
            throw new IllegalArgumentException("value can't be null");
        }
        mConfig.putObject(mKey, value);
    }

    @Override
    public boolean isValid() {
        return true;
    }
}
