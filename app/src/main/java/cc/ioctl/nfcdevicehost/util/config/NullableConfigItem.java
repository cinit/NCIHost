package cc.ioctl.nfcdevicehost.util.config;

import androidx.annotation.Nullable;

public interface NullableConfigItem<T> extends IConfigItem<T>, IObservable<T> {
    /**
     * Get the value of the config item.
     * This method should only be called if isValid() returns true.
     * This method may be called at any thread.
     *
     * @return the value
     */
    @Override
    @Nullable
    T getValue();

    /**
     * Set the value of the config item.
     * This method should only be called if isValid() returns true.
     * This method may be called at any thread.
     *
     * @param value the new value
     */
    @Override
    void setValue(@Nullable T value);

    /**
     * Check if the value is valid.
     *
     * @return true if the value is valid, false otherwise
     */
    @Override
    boolean isValid();
}
