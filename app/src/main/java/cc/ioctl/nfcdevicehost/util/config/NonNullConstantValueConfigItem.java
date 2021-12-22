package cc.ioctl.nfcdevicehost.util.config;

import androidx.annotation.NonNull;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.Observer;

public class NonNullConstantValueConfigItem<T> implements NonNullConfigItem<T> {
    @NonNull
    private final T mValue;
    private final boolean mValid;

    public NonNullConstantValueConfigItem(@NonNull T value, boolean valid) {
        mValue = value;
        mValid = valid;
    }

    @Override
    public void observe(@NonNull LifecycleOwner owner, @NonNull Observer<? super T> observer) {
    }

    @Override
    public void observeForever(@NonNull Observer<? super T> observer) {
    }

    @Override
    public boolean hasActiveObservers() {
        return false;
    }

    @Override
    public boolean hasObservers() {
        return false;
    }

    @Override
    public void removeObserver(@NonNull Observer<? super T> observer) {
    }

    @Override
    public void removeObservers(@NonNull LifecycleOwner owner) {
    }

    @NonNull
    @Override
    public T getValue() {
        return mValue;
    }

    @Override
    public void setValue(@NonNull T value) {
        throw new UnsupportedOperationException("Constant value config item cannot be set");
    }

    @Override
    public boolean isValid() {
        return mValid;
    }
}
