package cc.ioctl.nfcncihost.activity.ui.dump;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import java.util.concurrent.ConcurrentHashMap;

import cc.ioctl.nfcncihost.daemon.INciHostDaemon;
import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;

public class NciDumpViewModel extends ViewModel implements IpcNativeHandler.IpcConnectionListener, INciHostDaemon.OnRemoteEventListener {

    private final MutableLiveData<ConcurrentHashMap<Integer, INciHostDaemon.IoEventPacket>> mIoEventPackets;
    private final MutableLiveData<INciHostDaemon> mNciHostDaemon;

    public NciDumpViewModel() {
        mIoEventPackets = new MutableLiveData<>();
        mNciHostDaemon = new MutableLiveData<>();
        IpcNativeHandler.registerConnectionListener(this);
        mNciHostDaemon.setValue(IpcNativeHandler.peekConnection());
        mIoEventPackets.setValue(new ConcurrentHashMap<>());
        INciHostDaemon daemon = mNciHostDaemon.getValue();
        if (daemon != null && daemon.isConnected()) {
            daemon.registerRemoteEventListener(this);
            syncIoPackets();
        }
    }

    public LiveData<ConcurrentHashMap<Integer, INciHostDaemon.IoEventPacket>> getNciHostDaemon() {
        return mIoEventPackets;
    }

    @Override
    protected void onCleared() {
        super.onCleared();
        INciHostDaemon daemon = mNciHostDaemon.getValue();
        if (daemon != null) {
            daemon.unregisterRemoteEventListener(this);
        }
        IpcNativeHandler.unregisterConnectionListener(this);
    }

    @Override
    public void onConnect(INciHostDaemon daemon) {
        mNciHostDaemon.postValue(daemon);
        daemon.registerRemoteEventListener(this);
        syncIoPackets();
    }

    private void syncIoPackets() {
        INciHostDaemon daemon = mNciHostDaemon.getValue();
        if (daemon == null || !daemon.isConnected()) {
            return;
        }
        // clear the old data
        ConcurrentHashMap<Integer, INciHostDaemon.IoEventPacket> packets = mIoEventPackets.getValue();
        assert packets != null;
        packets.clear();
        // pull the new history data
        INciHostDaemon.HistoryIoEventList historyList = daemon.getHistoryIoEvents(0, 0);
        int startIndex = historyList.totalStartIndex;
        final int maxCountPerRequest = 100;
        int start = startIndex;
        do {
            historyList = daemon.getHistoryIoEvents(start, maxCountPerRequest);
            for (INciHostDaemon.IoEventPacket packet : historyList.events) {
                packets.put(packet.sequence, packet);
            }
            start += historyList.events.length;
            // if we got less than maxCountPerRequest, we have reached the end
        } while (historyList.events.length == maxCountPerRequest);
        // notify the observer
        mIoEventPackets.postValue(packets);
    }

    @Override
    public void onDisconnect(INciHostDaemon daemon) {
        mNciHostDaemon.postValue(null);
        daemon.unregisterRemoteEventListener(this);
    }

    @Override
    public void onIoEvent(INciHostDaemon.IoEventPacket event) {
        ConcurrentHashMap<Integer, INciHostDaemon.IoEventPacket> packets = mIoEventPackets.getValue();
        assert packets != null;
        packets.put(event.sequence, event);
        // notify the observer
        mIoEventPackets.postValue(packets);
    }

    @Override
    public void onRemoteDeath(INciHostDaemon.RemoteDeathPacket event) {
        // ignore
    }
}
