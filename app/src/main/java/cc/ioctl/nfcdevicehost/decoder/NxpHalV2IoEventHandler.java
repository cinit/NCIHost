package cc.ioctl.nfcdevicehost.decoder;

import androidx.annotation.Nullable;

import java.util.HashMap;
import java.util.Objects;

import cc.ioctl.nfcdevicehost.ipc.daemon.INciHostDaemon;

/**
 * I/O system call event handler for NXP HAL-Impl v2.
 * Supports only the following device files: /dev/nq-nci.
 * Device like NQ2xx, NQ3xx, SN100, and maybe other NCI devices.
 * Only the following events are handled: open, close, read, write, ioctl and select.
 * This decoder assumes that the each write operation is a complete packet,
 * but each read operation is separated by a select system call.
 */
public class NxpHalV2IoEventHandler {

    public enum FileType {
        /**
         * Unable to determine the file type.
         */
        UNKNOWN,
        /**
         * Device file, in /dev.
         */
        DEVICE,
        /**
         * Not a /dev file.
         */
        NON_DEVICE,
    }

    public interface BaseEvent {
    }

    public static class ReadEvent implements BaseEvent {
        public FileType fileType;
        public byte[] data;
        @Nullable
        public String auxPath;

        public ReadEvent(FileType fileType, byte[] data, @Nullable String auxPath) {
            this.fileType = fileType;
            this.data = data;
            this.auxPath = auxPath;
        }
    }

    public static class WriteEvent implements BaseEvent {
        public FileType fileType;
        public byte[] data;
        @Nullable
        public String auxPath;

        public WriteEvent(FileType fileType, byte[] data, @Nullable String auxPath) {
            this.fileType = fileType;
            this.data = data;
            this.auxPath = auxPath;
        }
    }

    public static class IoctlEvent implements BaseEvent {
        public FileType fileType;
        public int request;
        public long arg;
        @Nullable
        public String auxPath;

        public IoctlEvent(FileType fileType, int request, long arg, @Nullable String auxPath) {
            this.fileType = fileType;
            this.request = request;
            this.arg = arg;
            this.auxPath = auxPath;
        }
    }

    // end of public static inner class

    static class ReadStatus {
        public static int RESET = 0;
        public static int WAIT_FOR_PAYLOAD = 1;
    }

    private int mReadStatus = 0;
    private byte[] mLastReadBuffer = null;
    private HashMap<Integer, String> mFilePathMap = new HashMap<>();

    /**
     * Handle the event.
     * If the event is read/write/ioctl, return the corresponding event object.
     * For other events, return null.
     *
     * @param event the event to handle.
     * @return the corresponding event object or null.
     */
    public BaseEvent update(INciHostDaemon.IoEventPacket event) {
        Objects.requireNonNull(event, "event is null");
        if (event.opType != INciHostDaemon.IoEventPacket.IoOperationType.OPEN && event.fd >= 0) {
            String path = mFilePathMap.get(event.fd);
            if (path != null) {
                event.auxPath = path;
            }
        }
        switch (event.opType) {
            case OPEN: {
                String fileName = new String(event.buffer, 0, event.buffer.length);
                mFilePathMap.put(event.fd, fileName);
                event.auxPath = fileName;
                return null;
            }
            case CLOSE: {
                mFilePathMap.remove(event.fd);
                return null;
            }
            case READ: {
                String fileName = mFilePathMap.get(event.fd);
                if (fileName != null && !fileName.startsWith("/dev/")) {
                    // not a device file, just return as is
                    return new ReadEvent(FileType.NON_DEVICE, event.buffer, fileName);
                } else {
                    FileType fileType = fileName != null ? FileType.DEVICE : FileType.UNKNOWN;
                    // treat as a device file, each read is separated by a select system call
                    if (mReadStatus == ReadStatus.RESET) {
                        if (event.buffer.length == 2 || event.buffer.length == 3) {
                            // the message header is 2-3 bytes long
                            mReadStatus = ReadStatus.WAIT_FOR_PAYLOAD;
                            mLastReadBuffer = event.buffer;
                            return null;
                        } else {
                            // not a valid message header, return as is
                            return new ReadEvent(fileType, event.buffer, fileName);
                        }
                    } else {
                        // we have payload, combine the last read buffer with this one
                        byte[] combinedBuffer = new byte[mLastReadBuffer.length + event.buffer.length];
                        System.arraycopy(mLastReadBuffer, 0, combinedBuffer, 0, mLastReadBuffer.length);
                        System.arraycopy(event.buffer, 0, combinedBuffer, mLastReadBuffer.length, event.buffer.length);
                        mLastReadBuffer = null;
                        mReadStatus = ReadStatus.RESET;
                        return new ReadEvent(fileType, combinedBuffer, fileName);
                    }
                }
            }
            case WRITE: {
                // every write is considered as a complete packet
                String fileName = mFilePathMap.get(event.fd);
                FileType fileType = fileName != null ? (
                        fileName.startsWith("/dev/") ? FileType.DEVICE : FileType.NON_DEVICE) : FileType.UNKNOWN;
                return new WriteEvent(fileType, event.buffer, fileName);
            }
            case IOCTL: {
                String fileName = mFilePathMap.get(event.fd);
                FileType fileType = fileName != null ? (
                        fileName.startsWith("/dev/") ? FileType.DEVICE : FileType.NON_DEVICE) : FileType.UNKNOWN;
                return new IoctlEvent(fileType, (int) (event.directArg1), event.directArg2, fileName);
            }
            case SELECT: {
                String fileName = mFilePathMap.get(event.fd);
                // select is used to separate the read events, if we have a previous read event, but we are waiting
                // for the payload, we should flush it out
                if (mReadStatus == ReadStatus.WAIT_FOR_PAYLOAD) {
                    mReadStatus = ReadStatus.RESET;
                    byte[] unknownBuffer = mLastReadBuffer;
                    mLastReadBuffer = null;
                    return new ReadEvent(FileType.UNKNOWN, unknownBuffer, fileName);
                } else {
                    return null;
                }
            }
            default: {
                // unknown event
                return null;
            }
        }
    }

    public void reset() {
        mReadStatus = ReadStatus.RESET;
        mLastReadBuffer = null;
        mFilePathMap.clear();
    }
}
