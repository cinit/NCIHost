package cc.ioctl.nfcdevicehost.decoder;

import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import cc.ioctl.nfcdevicehost.daemon.INciHostDaemon;

public class NxpHalV2EventTranslator {
    private final HashMap<Integer, INciHostDaemon.IoEventPacket> mRawIoEventPackets = new HashMap<>();
    private int mLastRawEventSequence = 0;
    // guarded by this
    private final ArrayList<TransactionEvent> mTransactionEvents = new ArrayList<>();
    // guarded by this
    private int mLastUpdateTransactionSequence = 0;
    // guarded by this
    private int mLastSuccessTransactionSequence = 0;

    private final NxpHalV2IoEventHandler mNxpHalIoEventHandler = new NxpHalV2IoEventHandler();
    private final NciPacketDecoder mNciPacketDecoder = new NciPacketDecoder();

    public ArrayList<TransactionEvent> getTransactionEvents() {
        return mTransactionEvents;
    }

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

    private synchronized boolean translateIoEvents() {
        ArrayList<TransactionEvent> transactionEvents = mTransactionEvents;
        boolean updated = false;
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
        return updated;
    }

    public StringBuilder exportRawEventsAsCsv() {
        StringBuilder sb = new StringBuilder();
        for (INciHostDaemon.IoEventPacket eventPacket : mRawIoEventPackets.values()) {
            sb.append(eventPacket.sequence).append(",");
            sb.append(eventPacket.timestamp).append(",");
            sb.append(eventPacket.fd).append(",");
            sb.append(eventPacket.opType.getValue()).append(",");
            sb.append(eventPacket.retValue).append(",");
            sb.append(eventPacket.directArg1).append(",");
            sb.append(eventPacket.directArg2).append(",");
            sb.append(Arrays.toString(eventPacket.buffer).replace(" ", "")).append("\n");
        }
        return sb;
    }

    public static ArrayList<INciHostDaemon.IoEventPacket> loadIoEventPacketsFromString(@NonNull String source) {
        ArrayList<INciHostDaemon.IoEventPacket> result = new ArrayList<>();
        String[] lines = source.split("\n");
        for (String line : lines) {
            line = line.replace(" ", "");
            if (line.isEmpty()) {
                continue;
            }
            int index = Math.max(line.indexOf('['), line.indexOf("null"));
            String part1 = line.substring(0, index);
            String[] items = part1.split(",");
            INciHostDaemon.IoEventPacket packet = new INciHostDaemon.IoEventPacket();
            packet.sequence = Integer.parseInt(items[0]);
            packet.timestamp = Long.parseLong(items[1]);
            packet.fd = Integer.parseInt(items[2]);
            packet.opType = INciHostDaemon.IoEventPacket.IoOperationType.fromValue(Integer.parseInt(items[3]));
            packet.retValue = Long.parseLong(items[4]);
            packet.directArg1 = Long.parseLong(items[5]);
            packet.directArg2 = Long.parseLong(items[6]);
            String[] bufferStrArray = line.substring(index).replace("[", "").replace("]", "").split(",");
            if ("null".equals(bufferStrArray[0])) {
                packet.buffer = null;
            } else {
                byte[] buffer = new byte[bufferStrArray.length];
                for (int i = 0; i < bufferStrArray.length; i++) {
                    buffer[i] = Byte.parseByte(bufferStrArray[i]);
                }
                packet.buffer = buffer;
            }
            result.add(packet);
        }
        return result;
    }

    /**
     * Add a raw event to the list end. This event sequence must exactly match the last event sequence in the list.
     * That is @{code eventPacket.sequence == mLastRawEventSequence + 1}.
     *
     * @param eventPacket the event packet to add
     * @return true if there is a transaction event generated from this raw event
     */
    public boolean pushBackRawIoEvent(@NonNull INciHostDaemon.IoEventPacket eventPacket) {
        mRawIoEventPackets.put(eventPacket.sequence, eventPacket);
        return translateIoEvents();
    }

    /**
     * Add a raw event to the list front. This event sequence must exactly match the last event sequence in the list.
     * That is @{code eventPacket.sequence == mLastRawEventSequence + 1}.
     *
     * @param eventPackets the event packets to add
     * @return true if there are any transaction events generated from these raw events
     */
    public boolean pushBackRawIoEvents(@NonNull List<INciHostDaemon.IoEventPacket> eventPackets) {
        for (INciHostDaemon.IoEventPacket eventPacket : eventPackets) {
            mRawIoEventPackets.put(eventPacket.sequence, eventPacket);
            if (eventPacket.sequence > mLastRawEventSequence) {
                mLastRawEventSequence = eventPacket.sequence;
            }
        }
        return translateIoEvents();
    }

    public int getLastRawEventSequence() {
        return mLastRawEventSequence;
    }

    public int getLastSuccessTransactionSequence() {
        return mLastSuccessTransactionSequence;
    }

    /**
     * Reset the decoder state machine.
     * Note that this method dose not clear the event list.
     */
    public void resetDecoders() {
        mNciPacketDecoder.reset();
        mNxpHalIoEventHandler.reset();
    }

    /**
     * Clear all the events and reset the decoder state machine.
     */
    public void clearTransactionEvents() {
        mNciPacketDecoder.reset();
        mNxpHalIoEventHandler.reset();
        mLastRawEventSequence = 0;
        mLastUpdateTransactionSequence = 0;
        mLastSuccessTransactionSequence = 0;
        mRawIoEventPackets.clear();
        mTransactionEvents.clear();
    }
}
