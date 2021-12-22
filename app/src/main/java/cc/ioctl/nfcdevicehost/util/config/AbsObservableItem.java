package cc.ioctl.nfcdevicehost.util.config;

import androidx.lifecycle.LiveData;

import cc.ioctl.nfcdevicehost.util.ThreadManager;

public abstract class AbsObservableItem<T> extends LiveData<T> implements IObservable<T>, IConfigItem<T> {
    /**
     * The data of the item is always provided by the {@link #getValue()} method.
     * This AbsObservableItem itself is not responsible for providing the data.
     */
    protected AbsObservableItem() {
        super();
    }

    @Override
    public abstract T getValue();

    protected abstract void setValueInternal(T value);

    @Override
    public abstract boolean isValid();

    /**
     * Called when the value of the item has changed by any means, eg. by another process.
     * It's not required to call this method if value is changed by calling {@link #setValue(Object)}
     *
     * @param newValue the new value of the item
     */
    protected void notifyItemChanged(T newValue) {
        if (ThreadManager.isUiThread()) {
            super.setValue(newValue);
        } else {
            ThreadManager.runOnUiThread(() -> super.setValue(newValue));
        }
    }

    @Override
    public void setValue(T value) {
        if (ThreadManager.isUiThread()) {
            setValueInternal(value);
            super.setValue(value);
        } else {
            ThreadManager.runOnUiThread(() -> {
                setValueInternal(value);
                super.setValue(value);
            });
        }
    }

    @Override
    protected void postValue(T value) {
        setValueInternal(value);
        super.postValue(value);
    }
}
