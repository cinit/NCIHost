package cc.ioctl.nfcdevicehost.util.config

import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.Observer

open class LazyNullableMmkvItemConfig<T>(
    private val group: String?,
    private val key: String,
    private val defValue: T?
) : NullableConfigItem<T> {

    private val impl: NullableMmkvConfigItem<T> by lazy {
        val keyName: String = if (group.isNullOrEmpty()) key else "$group:$key"
        NullableMmkvConfigItem(ConfigManager.getDefaultConfig(), keyName, defValue)
    }

    override fun observe(owner: LifecycleOwner, observer: Observer<in T>) {
        impl.observe(owner, observer)
    }

    override fun observeForever(observer: Observer<in T>) {
        impl.observeForever(observer)
    }

    override fun hasActiveObservers(): Boolean {
        return impl.hasActiveObservers()
    }

    override fun hasObservers(): Boolean {
        return impl.hasObservers()
    }

    override fun removeObserver(observer: Observer<in T>) {
        impl.removeObserver(observer)
    }

    override fun removeObservers(owner: LifecycleOwner) {
        impl.removeObservers(owner)
    }

    override fun getValue(): T? {
        return impl.value
    }

    override fun setValue(value: T?) {
        impl.value = value
    }

    override fun isValid(): Boolean {
        return impl.isValid
    }
}
