package cc.ioctl.nfcdevicehost.ipc.daemon.internal;

import androidx.annotation.NonNull;

import java.io.IOException;
import java.util.Collections;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;

import cc.ioctl.nfcdevicehost.ipc.daemon.INciHostDaemon;

public class NciHostDaemonProxy implements INciHostDaemon {

    public interface NativeEventPacket {
    }

    public static class RawIoEventPacket implements NativeEventPacket {
        public byte[] event;
        public byte[] payload;

        public RawIoEventPacket() {
        }

        public RawIoEventPacket(byte[] event, byte[] payload) {
            this.event = event;
            this.payload = payload;
        }
    }

    static class RawHistoryIoEventList {
        public int totalStartIndex;
        public int totalCount;
        public RawIoEventPacket[] events;

        public RawHistoryIoEventList() {
        }

        public RawHistoryIoEventList(int totalStartIndex, int totalCount, RawIoEventPacket[] events) {
            this.totalStartIndex = totalStartIndex;
            this.totalCount = totalCount;
            this.events = events;
        }
    }

    public void dispatchRemoteEvent(NativeEventPacket event) {
        if (event instanceof RawIoEventPacket) {
            IoEventPacket ioEvent = new IoEventPacket((RawIoEventPacket) event);
            for (OnRemoteEventListener listener : mListeners) {
                listener.onIoEvent(ioEvent);
            }
        } else if (event instanceof RemoteDeathPacket) {
            RemoteDeathPacket death = (RemoteDeathPacket) event;
            for (OnRemoteEventListener listener : mListeners) {
                listener.onRemoteDeath(death);
            }
        }
    }

    private final Set<OnRemoteEventListener> mListeners = Collections.synchronizedSet(new HashSet<>());

    @Override
    public void registerRemoteEventListener(@NonNull OnRemoteEventListener listener) {
        Objects.requireNonNull(listener);
        mListeners.add(listener);
    }

    @Override
    public boolean unregisterRemoteEventListener(@NonNull OnRemoteEventListener listener) {
        Objects.requireNonNull(listener);
        return mListeners.remove(listener);
    }

    @Override
    public native boolean isConnected();

    @Override
    public native String getVersionName();

    @Override
    public native int getVersionCode();

    @Override
    public native String getBuildUuid();

    @Override
    public native int getSelfPid();

    @Override
    public native void exitProcess();

    @Override
    public native boolean isDeviceSupported();

    @Override
    public native boolean isHwServiceConnected();

    @Override
    public native boolean initHwServiceConnection(@NonNull String[] soPath) throws IOException;

    private native RawHistoryIoEventList ntGetHistoryIoEvents(int startIndex, int count);

    @Override
    public native int deviceDriverWriteRaw(@NonNull byte[] data);

    @Override
    public native int deviceDriverIoctl0(int request, long arg);

    @NonNull
    @Override
    public native DaemonStatus getDaemonStatus();

    @Override
    public native boolean isAndroidNfcServiceConnected();

    @Override
    public native boolean connectToAndroidNfcService();

    @Override
    public native boolean isNfcDiscoverySoundDisabled();

    @Override
    public native boolean setNfcDiscoverySoundDisabled(boolean disable);

    @Override
    public HistoryIoEventList getHistoryIoEvents(int startIndex, int count) {
        RawHistoryIoEventList raw = ntGetHistoryIoEvents(startIndex, count);
        HistoryIoEventList list = new HistoryIoEventList();
        list.totalStartIndex = raw.totalStartIndex;
        list.totalCount = raw.totalCount;
        list.events = new IoEventPacket[raw.events.length];
        for (int i = 0; i < raw.events.length; i++) {
            list.events[i] = new IoEventPacket(raw.events[i]);
        }
        return list;
    }

    @Override
    public native boolean clearHistoryIoEvents();

    public native NativeEventPacket waitForEvent() throws IOException;
}
