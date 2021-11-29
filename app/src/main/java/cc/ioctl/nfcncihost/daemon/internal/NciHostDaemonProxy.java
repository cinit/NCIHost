package cc.ioctl.nfcncihost.daemon.internal;

import java.io.IOException;

import cc.ioctl.nfcncihost.daemon.INciHostDaemon;
import cc.ioctl.nfcncihost.util.ByteUtils;

public class NciHostDaemonProxy implements INciHostDaemon {

    public interface NativeEventPacket {
    }

    public static class RemoteDeathPacket implements NativeEventPacket {
        public int pid;

        public RemoteDeathPacket() {
        }

        public RemoteDeathPacket(int pid) {
            this.pid = pid;
        }
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

    public static class IoEventPacket implements NativeEventPacket {
        public enum IoOperationType {
            OPEN(1),
            CLOSE(2),
            READ(3),
            WRITE(4),
            IOCTL(5),
            SELECT(6);

            final int value;

            IoOperationType(int v) {
                value = v;
            }

            public int getValue() {
                return value;
            }
        }

        public int sequence;
        public long timestamp;
        public int fd;
        public IoOperationType opType;
        public long retValue;
        public long directArg1;
        public long directArg2;
        public byte[] buffer;

        public IoEventPacket() {
        }

        //struct IoOperationEvent {
        //    uint32_t sequence;
        //    uint32_t rfu;
        //    uint64_t timestamp;
        //    IoSyscallInfo info;
        //};
        //struct IoSyscallInfo {
        //    int32_t opType;
        //    int32_t fd;
        //    int64_t retValue;
        //    uint64_t directArg1;
        //    uint64_t directArg2;
        //    int64_t bufferLength;
        //};
        //

        public IoEventPacket(RawIoEventPacket raw) {
            sequence = ByteUtils.readInt32(raw.event, 0);
            // rfu ignored
            timestamp = ByteUtils.readInt64(raw.event, 8);
            int op = ByteUtils.readInt32(raw.event, 16);
            opType = IoOperationType.values()[op - 1]; // 1-based
            fd = ByteUtils.readInt32(raw.event, 20);
            retValue = ByteUtils.readInt64(raw.event, 24);
            directArg1 = ByteUtils.readInt64(raw.event, 32);
            directArg2 = ByteUtils.readInt64(raw.event, 40);
            buffer = raw.payload;
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
    public native void exitProcess();

    @Override
    public native boolean isDeviceSupported();

    @Override
    public native boolean isHwServiceConnected();

    @Override
    public native boolean initHwServiceConnection(String soPath) throws IOException;

    public native NativeEventPacket waitForEvent() throws IOException;
}
