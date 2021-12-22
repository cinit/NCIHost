package cc.ioctl.nfcdevicehost.util.config;

import androidx.annotation.NonNull;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.Observer;

/**
 * Observable items like LiveData, where the callback is always called on the main thread.
 *
 * @param <T> the value type
 */
public interface IObservable<T> {
    void observe(@NonNull LifecycleOwner owner, @NonNull Observer<? super T> observer);

    void observeForever(@NonNull Observer<? super T> observer);

    boolean hasActiveObservers();

    boolean hasObservers();

    void removeObserver(@NonNull Observer<? super T> observer);

    void removeObservers(@NonNull LifecycleOwner owner);
}
