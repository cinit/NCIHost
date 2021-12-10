package cc.ioctl.nfcncihost.activity.ui.home;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import cc.ioctl.nfcncihost.daemon.INciHostDaemon;
import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;

public class HomeViewModel extends ViewModel implements IpcNativeHandler.IpcConnectionListener {

    private final MutableLiveData<Boolean> nciHostDaemonConnected = new MutableLiveData<>();

    public HomeViewModel() {
        IpcNativeHandler.registerConnectionListener(this);
        nciHostDaemonConnected.setValue(IpcNativeHandler.peekConnection() != null);
    }

    public LiveData<Boolean> isNciHostDaemonConnected() {
        return nciHostDaemonConnected;
    }

    @Override
    protected void onCleared() {
        super.onCleared();
        IpcNativeHandler.unregisterConnectionListener(this);
    }

    @Override
    public void onConnect(INciHostDaemon daemon) {
        nciHostDaemonConnected.postValue(true);
    }

    @Override
    public void onDisconnect(INciHostDaemon daemon) {
        nciHostDaemonConnected.postValue(false);
    }
}
