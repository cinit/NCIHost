package cc.ioctl.nfcncihost.daemon.internal;

import java.io.IOException;

import cc.ioctl.nfcncihost.daemon.INciHostDaemon;

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
            if (mListener != null) {
                mListener.onIoEvent(ioEvent);
            }
        } else if (event instanceof RemoteDeathPacket) {
            if (mListener != null) {
                mListener.onRemoteDeath((RemoteDeathPacket) event);
            }
        }
    }

    private OnRemoteEventListener mListener = null;

    @Override
    public OnRemoteEventListener getRemoteEventListener() {
        return mListener;
    }

    @Override
    public void setRemoteEventListener(OnRemoteEventListener listener) {
        mListener = listener;
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
    public native boolean initHwServiceConnection(String soPath) throws IOException;

    private native RawHistoryIoEventList ntGetHistoryIoEvents(int startIndex, int count);

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

    public native NativeEventPacket waitForEvent() throws IOException;
}
