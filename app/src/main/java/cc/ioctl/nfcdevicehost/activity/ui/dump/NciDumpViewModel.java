package cc.ioctl.nfcdevicehost.activity.ui.dump;

import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import java.util.ArrayList;
import java.util.Arrays;

import cc.ioctl.nfcdevicehost.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.decoder.NxpHalV2EventTranslator;

public class NciDumpViewModel extends ViewModel implements IpcNativeHandler.IpcConnectionListener, INciHostDaemon.OnRemoteEventListener {

    private final MutableLiveData<INciHostDaemon> mNciHostDaemon;
    private final MutableLiveData<ArrayList<NxpHalV2EventTranslator.TransactionEvent>> mTransactionEvents = new MutableLiveData<>(new ArrayList<>());
    private final NxpHalV2EventTranslator mTranslator = new NxpHalV2EventTranslator();

    public NciDumpViewModel() {
        mNciHostDaemon = new MutableLiveData<>();
        IpcNativeHandler.registerConnectionListener(this);
        mNciHostDaemon.setValue(IpcNativeHandler.peekConnection());
        INciHostDaemon daemon = mNciHostDaemon.getValue();
        if (daemon != null && daemon.isConnected()) {
            daemon.registerRemoteEventListener(this);
            resetAndSyncIoPackets();
        }
    }

    @Override
    public void onDisconnect(INciHostDaemon daemon) {
        mTranslator.resetDecoders();
        mNciHostDaemon.postValue(null);
        daemon.unregisterRemoteEventListener(this);
    }

    @Override
    public void onIoEvent(INciHostDaemon.IoEventPacket event) {
        int lastSeq = mTranslator.getLastRawEventSequence();
        if (event.sequence == lastSeq + 1) {
            if (mTranslator.pushBackRawIoEvent(event)) {
                // notify the observer
                mTransactionEvents.postValue(mTranslator.getTransactionEvents());
            }
        } else {
            // we are out of sync, sync now
            synchronizeIoEvents();
        }
    }

    @Override
    public void onRemoteDeath(INciHostDaemon.RemoteDeathPacket event) {
        // ignore
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
        resetAndSyncIoPackets();
    }


    public void synchronizeIoEvents() {
        INciHostDaemon daemon = mNciHostDaemon.getValue();
        if (daemon != null && daemon.isConnected()) {
            long startIndex = mTranslator.getLastRawEventSequence();
            final int maxCountPerRequest = 100;
            long start = startIndex;
            INciHostDaemon.HistoryIoEventList historyList;
            ArrayList<INciHostDaemon.IoEventPacket> deltaRawIoEventPackets = new ArrayList<>();
            do {
                historyList = daemon.getHistoryIoEvents((int) start, maxCountPerRequest);
                if (historyList.events.length > 0) {
                    deltaRawIoEventPackets.addAll(Arrays.asList(historyList.events));
                    start = historyList.events[historyList.events.length - 1].sequence + 1;
                }
                // if we got less than maxCountPerRequest, we have reached the end
            } while (historyList.events.length >= maxCountPerRequest);
            if (deltaRawIoEventPackets.size() > 0
                    && mTranslator.pushBackRawIoEvents(deltaRawIoEventPackets)) {
                // notify the observer
                mTransactionEvents.postValue(mTranslator.getTransactionEvents());
            }
        }
    }

    public void clearTransactionEvents() {
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        if (daemon != null) {
            daemon.clearHistoryIoEvents();
        }
        mTranslator.clearTransactionEvents();
        mTransactionEvents.postValue(mTranslator.getTransactionEvents());
    }

    /**
     * Reset the local event list and synchronize with the daemon
     * This is called when the daemon is connected or ViewModel is created
     * Avoid calling this method on the main thread
     */
    private void resetAndSyncIoPackets() {
        INciHostDaemon daemon = mNciHostDaemon.getValue();
        if (daemon == null || !daemon.isConnected()) {
            return;
        }
        // clear the old data
        mTranslator.clearTransactionEvents();
        // pull the new history data
        INciHostDaemon.HistoryIoEventList historyList = daemon.getHistoryIoEvents(0, 0);
        int startIndex = historyList.totalStartIndex;
        final int maxCountPerRequest = 100;
        int start = startIndex;
        ArrayList<INciHostDaemon.IoEventPacket> deltaRawIoEventPackets = new ArrayList<>();
        do {
            historyList = daemon.getHistoryIoEvents(start, maxCountPerRequest);
            if (historyList.events.length > 0) {
                deltaRawIoEventPackets.addAll(Arrays.asList(historyList.events));
                start = historyList.events[historyList.events.length - 1].sequence + 1;
            }
            // if we got less than maxCountPerRequest, we have reached the end
        } while (historyList.events.length >= maxCountPerRequest);
        // translate events to transactions
        if (deltaRawIoEventPackets.size() > 0
                && mTranslator.pushBackRawIoEvents(deltaRawIoEventPackets)) {
            // notify the observer
            mTransactionEvents.postValue(mTranslator.getTransactionEvents());
        }
    }

    public MutableLiveData<ArrayList<NxpHalV2EventTranslator.TransactionEvent>> getTransactionEvents() {
        return mTransactionEvents;
    }

    public NxpHalV2EventTranslator getTranslator() {
        return mTranslator;
    }
}
