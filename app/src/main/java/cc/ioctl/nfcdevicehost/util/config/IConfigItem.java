package cc.ioctl.nfcdevicehost.util.config;

public interface IConfigItem<T> {
    /**
     * Get the value of the config item.
     * This method should only be called if isValid() returns true.
     * This method may be called at any thread.
     *
     * @return the value
     */
    T getValue();

    /**
     * Set the value of the config item.
     * This method should only be called if isValid() returns true.
     * This method may be called at any thread.
     *
     * @param value the new value
     */
    void setValue(T value);

    /**
     * Check if the value is valid.
     *
     * @return true if the value is valid, false otherwise
     */
    boolean isValid();
}
