package cc.ioctl.nfcdevicehost.activity.ui.home;

import android.util.Log;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import cc.ioctl.nfcdevicehost.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler;

public class HomeViewModel extends ViewModel implements IpcNativeHandler.IpcConnectionListener {

    private final MutableLiveData<INciHostDaemon.DaemonStatus> nciHostDaemonStatus = new MutableLiveData<>();

    public HomeViewModel() {
        IpcNativeHandler.registerConnectionListener(this);
        Log.i("HomeViewModel", "HomeViewModel created");
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        if (daemon != null && daemon.isConnected()) {
            INciHostDaemon.DaemonStatus status = daemon.getDaemonStatus();
            nciHostDaemonStatus.setValue(status);
        }
    }

    public LiveData<INciHostDaemon.DaemonStatus> getDaemonStatus() {
        return nciHostDaemonStatus;
    }

    @Override
    protected void onCleared() {
        super.onCleared();
        IpcNativeHandler.unregisterConnectionListener(this);
    }

    public void refreshDaemonStatus() {
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        if (daemon != null && daemon.isConnected()) {
            INciHostDaemon.DaemonStatus status = daemon.getDaemonStatus();
            nciHostDaemonStatus.postValue(status);
        } else {
            nciHostDaemonStatus.postValue(null);
        }
    }

    @Override
    public void onConnect(INciHostDaemon daemon) {
        INciHostDaemon.DaemonStatus status = daemon.getDaemonStatus();
        nciHostDaemonStatus.postValue(status);
    }

    @Override
    public void onDisconnect(INciHostDaemon daemon) {
        nciHostDaemonStatus.postValue(null);
    }
}
