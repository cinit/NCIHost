package cc.ioctl.nfcdevicehost.util.config;

import androidx.annotation.NonNull;

public interface NonNullConfigItem<T> extends IConfigItem<T>, IObservable<T> {
    /**
     * Get the value of the config item.
     * This method should only be called if isValid() returns true.
     * This method may be called at any thread.
     *
     * @return the value
     */
    @Override
    @NonNull
    T getValue();

    /**
     * Set the value of the config item.
     * This method should only be called if isValid() returns true.
     * This method may be called at any thread.
     *
     * @param value the new value
     */
    @Override
    void setValue(@NonNull T value);

    /**
     * Check if the value is valid.
     *
     * @return true if the value is valid, false otherwise
     */
    @Override
    boolean isValid();
}
