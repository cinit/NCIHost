package cc.ioctl.nfcncihost.activity.ui.dump;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import java.util.ArrayList;
import java.util.HashMap;

import cc.ioctl.nfcncihost.daemon.INciHostDaemon;
import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;
import cc.ioctl.nfcncihost.decoder.NciPacketDecoder;
import cc.ioctl.nfcncihost.decoder.NxpHalV2IoEventHandler;

public class NciDumpViewModel extends ViewModel implements IpcNativeHandler.IpcConnectionListener, INciHostDaemon.OnRemoteEventListener {

    private final MutableLiveData<INciHostDaemon> mNciHostDaemon;
    private final HashMap<Integer, INciHostDaemon.IoEventPacket> mRawIoEventPackets = new HashMap<>();
    private int mLastRawEventSequence = 0;
    // guarded by this
    private final MutableLiveData<ArrayList<TransactionEvent>> mTransactionEvents = new MutableLiveData<>(new ArrayList<>());
    // guarded by this
    private int mLastUpdateTransactionSequence = 0;
    // guarded by this
    private int mLastSuccessTransactionSequence = 0;

    private final NxpHalV2IoEventHandler mNxpHalIoEventHandler = new NxpHalV2IoEventHandler();
    private final NciPacketDecoder mNciPacketDecoder = new NciPacketDecoder();

    public enum Direction {
        /**
         * write and ioctl
         */
        HOST_TO_DEVICE,
        /**
         * read
         */
        DEVICE_TO_HOST
    }

    public static class TransactionEvent {
        public long sequence;
        public long timestamp;
    }

    public static class NciTransactionEvent extends TransactionEvent {
        public NciPacketDecoder.Type type;
        public Direction direction;
        public NciPacketDecoder.NciPacket packet;

        public NciTransactionEvent(long sequence, long timestamp, NciPacketDecoder.Type type, Direction direction, NciPacketDecoder.NciPacket packet) {
            this.sequence = sequence;
            this.timestamp = timestamp;
            this.type = type;
            this.direction = direction;
            this.packet = packet;
        }
    }

    public static class IoctlTransactionEvent extends TransactionEvent {
        public int request;
        public long arg;

        public IoctlTransactionEvent(long sequence, long timestamp, int request, long arg) {
            this.sequence = sequence;
            this.timestamp = timestamp;
            this.request = request;
            this.arg = arg;
        }
    }

    public static class RawTransactionEvent extends TransactionEvent {
        public Direction direction;
        public byte[] data;

        public RawTransactionEvent(long sequence, long timestamp, Direction direction, byte[] data) {
            this.sequence = sequence;
            this.timestamp = timestamp;
            this.direction = direction;
            this.data = data;
        }
    }

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
            boolean updated = false;
            long startIndex = mLastRawEventSequence;
            final int maxCountPerRequest = 100;
            long start = startIndex;
            INciHostDaemon.HistoryIoEventList historyList;
            do {
                historyList = daemon.getHistoryIoEvents((int) start, maxCountPerRequest);
                if (historyList.events.length > 0) {
                    updated = true;
                    for (INciHostDaemon.IoEventPacket packet : historyList.events) {
                        mRawIoEventPackets.put(packet.sequence, packet);
                    }
                    mLastRawEventSequence = historyList.events[historyList.events.length - 1].sequence;
                    start = historyList.events[historyList.events.length - 1].sequence + 1;
                }
                // if we got less than maxCountPerRequest, we have reached the end
            } while (historyList.events.length >= maxCountPerRequest);
            if (updated) {
                // notify the observer
                translateIoEvents();
            }
        }
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
        HashMap<Integer, INciHostDaemon.IoEventPacket> packets = mRawIoEventPackets;
        packets.clear();
        // pull the new history data
        INciHostDaemon.HistoryIoEventList historyList = daemon.getHistoryIoEvents(0, 0);
        int startIndex = historyList.totalStartIndex;
        final int maxCountPerRequest = 100;
        int start = startIndex;
        do {
            historyList = daemon.getHistoryIoEvents(start, maxCountPerRequest);
            if (historyList.events.length > 0) {
                for (INciHostDaemon.IoEventPacket packet : historyList.events) {
                    packets.put(packet.sequence, packet);
                }
                mLastRawEventSequence = historyList.events[historyList.events.length - 1].sequence;
                start = historyList.events[historyList.events.length - 1].sequence + 1;
            }
            // if we got less than maxCountPerRequest, we have reached the end
        } while (historyList.events.length >= maxCountPerRequest);
        // translate events to transactions
        translateIoEvents();
    }

    @Override
    public void onDisconnect(INciHostDaemon daemon) {
        mNciPacketDecoder.reset();
        mNxpHalIoEventHandler.reset();
        mNciHostDaemon.postValue(null);
        daemon.unregisterRemoteEventListener(this);
    }

    @Override
    public void onIoEvent(INciHostDaemon.IoEventPacket event) {
        if (event.sequence == mLastRawEventSequence + 1) {
            mRawIoEventPackets.put(event.sequence, event);
            mLastRawEventSequence = event.sequence;
            translateIoEvents();
        } else {
            // we are out of sync, sync now
            synchronizeIoEvents();
        }
    }

    @Override
    public void onRemoteDeath(INciHostDaemon.RemoteDeathPacket event) {
        mNciPacketDecoder.reset();
        mNxpHalIoEventHandler.reset();
        // ignore
    }

    private synchronized void translateIoEvents() {
        ArrayList<TransactionEvent> transactionEvents = mTransactionEvents.getValue();
        boolean updated = false;
        assert transactionEvents != null;
        while (mLastUpdateTransactionSequence < mLastRawEventSequence) {
            // translate the events
            INciHostDaemon.IoEventPacket eventPacket = mRawIoEventPackets.get(mLastUpdateTransactionSequence + 1);
            if (eventPacket != null) {
                boolean isFailedReadOrWrite = (eventPacket.opType == INciHostDaemon.IoEventPacket.IoOperationType.READ
                        || eventPacket.opType == INciHostDaemon.IoEventPacket.IoOperationType.WRITE)
                        && eventPacket.retValue < 0;
                if (isFailedReadOrWrite) {
                    // failed read or write, ignore
                    mLastUpdateTransactionSequence++;
                    continue;
                }
                NxpHalV2IoEventHandler.BaseEvent baseEvent = mNxpHalIoEventHandler.update(eventPacket);
                if (baseEvent != null) {
                    if (baseEvent instanceof NxpHalV2IoEventHandler.WriteEvent
                            || baseEvent instanceof NxpHalV2IoEventHandler.ReadEvent) {
                        // NCI packet
                        Direction direction = (baseEvent instanceof NxpHalV2IoEventHandler.ReadEvent) ?
                                Direction.DEVICE_TO_HOST : Direction.HOST_TO_DEVICE;
                        byte[] data = (baseEvent instanceof NxpHalV2IoEventHandler.ReadEvent) ?
                                ((NxpHalV2IoEventHandler.ReadEvent) baseEvent).data
                                : ((NxpHalV2IoEventHandler.WriteEvent) baseEvent).data;
                        try {
                            if (data == null) {
                                throw new NciPacketDecoder.InvalidNciPacketException("buffer is null");
                            }
                            NciPacketDecoder.NciPacket packet = mNciPacketDecoder.decode(data);
                            if (packet != null) {
                                NciTransactionEvent result = new NciTransactionEvent(eventPacket.sequence,
                                        eventPacket.timestamp, packet.type, direction, packet);
                                transactionEvents.add(result);
                            }
                        } catch (NciPacketDecoder.InvalidNciPacketException e) {
                            // decode failed, push as a raw event
                            RawTransactionEvent result = new RawTransactionEvent(eventPacket.sequence,
                                    eventPacket.timestamp, direction, data);
                            transactionEvents.add(result);
                        }
                    } else if (baseEvent instanceof NxpHalV2IoEventHandler.IoctlEvent) {
                        // ioctl event
                        IoctlTransactionEvent result = new IoctlTransactionEvent(eventPacket.sequence,
                                eventPacket.timestamp, ((NxpHalV2IoEventHandler.IoctlEvent) baseEvent).request,
                                ((NxpHalV2IoEventHandler.IoctlEvent) baseEvent).arg);
                        transactionEvents.add(result);
                    }
                    updated = true;
                    mLastSuccessTransactionSequence = eventPacket.sequence;
                }
            }
            mLastUpdateTransactionSequence++;
        }
        if (updated) {
            // notify the observers
            mTransactionEvents.postValue(transactionEvents);
        }
    }

    public LiveData<ArrayList<TransactionEvent>> getTransactionEvents() {
        return mTransactionEvents;
    }

}
