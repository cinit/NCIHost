package cc.ioctl.nfcncihost.decoder;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Objects;

/**
 * State machine for decoding NCI packets.
 */
public class NciPacketDecoder {

    /**
     * The type of the packet, as defined in the NCI spec.
     * The type is encoded in the first byte of the packet.
     */
    public enum Type {
        /**
         * NCI packet type: Data packet, 0x00
         */
        NCI_DATA,
        /**
         * NCI packet type: Command packet, 0x01
         */
        NCI_CMD,
        /**
         * NCI packet type: Response packet, 0x02
         */
        NCI_RSP,
        /**
         * NCI packet type: Notification packet, 0x03
         */
        NCI_NTF,
        /**
         * Unknown packet type.
         */
        UNKNOWN;

        public static Type getType(int b) {
            switch (b) {
                case 0x00:
                    return NCI_DATA;
                case 0x01:
                    return NCI_CMD;
                case 0x02:
                    return NCI_RSP;
                case 0x03:
                    return NCI_NTF;
                default:
                    return UNKNOWN;
            }
        }
    }

    public static class NciPacket {
        public Type type;
        public byte[] data;
    }

    public static class NciDataPacket extends NciPacket {
        public int connId;
        public int credits;

        public NciDataPacket(int connId, int credits, byte[] data) {
            this.type = Type.NCI_DATA;
            this.connId = connId;
            this.credits = credits;
            this.data = data;
        }
    }

    public static class NciControlPacket extends NciPacket {
        public int groupId;
        public int opcodeId;

        public NciControlPacket(int msgType, int groupId, int opcodeId, byte[] data) {
            this.type = Type.getType(msgType);
            this.groupId = groupId;
            this.opcodeId = opcodeId;
            this.data = data;
        }
    }

    public static class InvalidNciPacketException extends Exception {
        public InvalidNciPacketException() {
            super();
        }

        public InvalidNciPacketException(String message) {
            super(message);
        }
    }

    // end of static declarations

    static class State {
        /**
         * Wait for next packet.
         */
        public static final int RESET = 0;
        /**
         * Already received a packet that contains a segment of a Message that is not the last segment.
         * Wait for the next segment.
         */
        public static final int WAIT_FOR_SEGMENT = 1;
    }

    private int mState = State.RESET;
    private ArrayList<byte[]> mPreviousSegments = new ArrayList<>();
    private int mPreviousPacketIdentifier = 0;

    /**
     * Reset the state machine.
     */
    public void reset() {
        mState = State.RESET;
    }

    /**
     * Decode a NCI packet.
     * Note: If a packet is divided into multiple segments, the decoder will only return one packet when the last
     * segment is received. Before the last segment is received, the decoder will return null.
     * If an exception is thrown, the decoder is not reset, so you can call {@link #reset()} to reset the decoder.
     *
     * @param packet The packet to decode.
     * @return The decoded packet, or null if the packet is not complete.
     * @throws InvalidNciPacketException If the packet is invalid.
     */
    @Nullable
    public NciPacket decode(@NonNull byte[] packet) throws InvalidNciPacketException {
        Objects.requireNonNull(packet, "packet must not be null");
        if (packet.length < 3) {
            throw new InvalidNciPacketException("packet length must be at least 3");
        }
        int messageType = (packet[0] & 0xFF) >> 5;
        boolean isLastSegment = (packet[0] & 0x10) == 0;
        if (!(messageType == 0 || messageType == 1 || messageType == 2 || messageType == 3)) {
            throw new InvalidNciPacketException("invalid message type " + messageType);
        }
        // TODO: NCI spec says that reassembly of data packets and control packets should be performed independently.
        //       Currently, we assemble all data packets and control packets together.
        //       We should probably split the packets into data packets and control packets.
        int packetIdentifier;
        if (messageType == 0) {
            // data packet
            int connectionId = packet[0] & 0x0F;
            int credit = packet[1] & 0x03;
            int payloadLength = packet[2] & 0xFF;
            if (packet.length < 3 + payloadLength) {
                throw new InvalidNciPacketException("payload length " + payloadLength
                        + " is too long for packet length " + packet.length);
            }
            packetIdentifier = messageType << 16 | connectionId << 8 | credit;
            if (isLastSegment) {
                // check if there is a previous segment
                if (mState == State.WAIT_FOR_SEGMENT) {
                    // assemble previous segment
                    if (mPreviousPacketIdentifier != packetIdentifier) {
                        throw new InvalidNciPacketException("packet identifier " + packetIdentifier
                                + " does not match previous packet identifier " + mPreviousPacketIdentifier);
                    }
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                    for (byte[] previousSegment : mPreviousSegments) {
                        int payloadLengthOfPreviousSegment = previousSegment[2] & 0xFF;
                        outputStream.write(previousSegment, 3, payloadLengthOfPreviousSegment);
                    }
                    outputStream.write(packet, 3, payloadLength);
                    byte[] payload = outputStream.toByteArray();
                    mState = State.RESET;
                    mPreviousSegments.clear();
                    mPreviousPacketIdentifier = 0;
                    return new NciDataPacket(connectionId, credit, payload);
                } else {
                    // complete packet
                    return new NciDataPacket(connectionId, credit, Arrays.copyOfRange(packet, 3, packet.length));
                }
            } else {
                // incomplete packet, save segment
                if (mState == State.RESET) {
                    mState = State.WAIT_FOR_SEGMENT;
                    mPreviousSegments.clear();
                    mPreviousPacketIdentifier = packetIdentifier;
                } else {
                    if (mPreviousPacketIdentifier != packetIdentifier) {
                        throw new InvalidNciPacketException("packet identifier " + packetIdentifier
                                + " does not match previous packet identifier " + mPreviousPacketIdentifier);
                    }
                }
                mPreviousSegments.add(packet);
                return null;
            }
        } else {
            // control packet
            // group code is 4 bits
            int groupId = packet[0] & 0x0F;
            // opcode is 6 bits
            int opcodeId = packet[1] & 0x3F;
            int payloadLength = packet[2] & 0xFF;
            if (packet.length < 3 + payloadLength) {
                throw new InvalidNciPacketException("payload length " + payloadLength
                        + " is too long for packet length " + packet.length);
            }
            packetIdentifier = messageType << 16 | groupId << 8 | opcodeId;
            if (isLastSegment) {
                // check if there is a previous segment
                if (mState == State.WAIT_FOR_SEGMENT) {
                    // assemble previous segment
                    if (mPreviousPacketIdentifier != packetIdentifier) {
                        throw new InvalidNciPacketException("packet identifier " + packetIdentifier
                                + " does not match previous packet identifier " + mPreviousPacketIdentifier);
                    }
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                    for (byte[] previousSegment : mPreviousSegments) {
                        int payloadLengthOfPreviousSegment = previousSegment[2] & 0xFF;
                        outputStream.write(previousSegment, 3, payloadLengthOfPreviousSegment);
                    }
                    outputStream.write(packet, 3, payloadLength);
                    byte[] payload = outputStream.toByteArray();
                    mState = State.RESET;
                    mPreviousSegments.clear();
                    mPreviousPacketIdentifier = 0;
                    return new NciControlPacket(messageType, groupId, opcodeId, payload);
                } else {
                    // complete packet
                    return new NciControlPacket(messageType, groupId, opcodeId, Arrays.copyOfRange(packet, 3, packet.length));
                }
            } else {
                // incomplete packet, save segment
                if (mState == State.RESET) {
                    mState = State.WAIT_FOR_SEGMENT;
                    mPreviousSegments.clear();
                    mPreviousPacketIdentifier = packetIdentifier;
                } else {
                    if (mPreviousPacketIdentifier != packetIdentifier) {
                        throw new InvalidNciPacketException("packet identifier " + packetIdentifier
                                + " does not match previous packet identifier " + mPreviousPacketIdentifier);
                    }
                }
                mPreviousSegments.add(packet);
                return null;
            }
        }
    }
}
