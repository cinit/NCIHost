package cc.ioctl.nfcdevicehost.decoder;

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

        public int getInt() {
            switch (this) {
                case NCI_DATA:
                    return 0x00;
                case NCI_CMD:
                    return 0x01;
                case NCI_RSP:
                    return 0x02;
                case NCI_NTF:
                    return 0x03;
                default:
                    return -1;
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

    /**
     * Get the operation name of a control packet.
     *
     * @param type message type, maybe 1, 2 or 3
     * @param gid  group id
     * @param oid  opcode id
     * @return the operation name if found, null otherwise
     */
    public static String getNciOperationName(int type, int gid, int oid) {
        String suffix;
        switch (type) {
            case 1:
                suffix = "_CMD";
                break;
            case 2:
                suffix = "_RSP";
                break;
            case 3:
                suffix = "_NTF";
                break;
            default:
                // unknown message type
                return null;
        }
        String prefix;
        if (gid == 0x00) {
            // NCI core group
            switch (oid) {
                case 0b000000:
                    prefix = "CORE_RESET";
                    break;
                case 0b000001:
                    prefix = "CORE_INIT";
                    break;
                case 0b000010:
                    prefix = "CORE_SET_CONFIG";
                    break;
                case 0b000011:
                    prefix = "CORE_GET_CONFIG";
                    break;
                case 0b000100:
                    prefix = "CORE_CONN_CREATE";
                    break;
                case 0b000101:
                    prefix = "CORE_CONN_CLOSE";
                    break;
                case 0b000110:
                    prefix = "CORE_CONN_CREDITS";
                    break;
                case 0b000111:
                    prefix = "CORE_GENERIC_ERROR";
                    break;
                case 0b001000:
                    prefix = "CORE_INTERFACE_ERROR";
                    break;
                case 0b001001:
                    prefix = "CORE_SET_POWER_SUB_STATE";
                    break;
                default:
                    // unknown core opcode
                    return null;
            }
        } else if (gid == 0x01) {
            // RF Management
            switch (oid) {
                case 0b000000:
                    prefix = "RF_DISCOVER_MAP";
                    break;
                case 0b000001:
                    prefix = "RF_SET_LISTEN_MODE_ROUTING";
                    break;
                case 0b000010:
                    prefix = "RF_GET_LISTEN_MODE_ROUTING";
                    break;
                case 0b000011:
                    prefix = "RF_DISCOVER";
                    break;
                case 0b000100:
                    prefix = "RF_DISCOVER_SELECT";
                    break;
                case 0b000101:
                    prefix = "RF_INTF_ACTIVATED";
                    break;
                case 0b000110:
                    prefix = "RF_DEACTIVATE";
                    break;
                case 0b000111:
                    prefix = "RF_FIELD_INFO";
                    break;
                case 0b001000:
                    prefix = "RF_T3T_POLLING";
                    break;
                case 0b001001:
                    prefix = "RF_NFCEE_ACTION";
                    break;
                case 0b001010:
                    prefix = "RF_NFCEE_DISCOVERY_REQ";
                    break;
                case 0b001011:
                    prefix = "RF_PARAMETER_UPDATE";
                    break;
                case 0b001100:
                    prefix = "RF_INTF_EXT_START";
                    break;
                case 0b001101:
                    prefix = "RF_INTF_EXT_STOP";
                    break;
                case 0b001110:
                    prefix = "RF_EXT_AGG_ABORT";
                    break;
                case 0b001111:
                    prefix = "RF_NDEF_ABORT";
                    break;
                case 0b010000:
                    prefix = "RF_ISO_DEP_NAK_PRESENCE";
                    break;
                case 0b010001:
                    prefix = "RF_SET_FORCED_NFCEE_ROUTING";
                    break;
                default:
                    // unknown RF opcode
                    return null;
            }
        } else if (gid == 0x02) {
            // NFCEE Management group
            //NFCEE_DISCOVER_CMD
            //NFCEE_DISCOVER_RSP
            //NFCEE_DISCOVER_NTF
            //NFCEE_MODE_SET_CMD
            //NFCEE_MODE_SET_RSP
            //NFCEE_MODE_SET_NTF
            //NFCEE_STATUS_NTF
            //NFCEE_POWER_AND_LINK_CNTRL_CMD
            //NFCEE_POWER_AND_LINK_CNTRL_RSP
            switch (oid) {
                case 0b000000:
                    prefix = "NFCEE_DISCOVER";
                    break;
                case 0b000001:
                    prefix = "NFCEE_MODE_SET";
                    break;
                case 0b000010:
                    prefix = "NFCEE_STATUS";
                    break;
                case 0b000011:
                    prefix = "NFCEE_POWER_AND_LINK_CNTRL";
                    break;
                default:
                    // unknown NFCEE opcode
                    return null;
            }
        } else {
            // unknown group
            return null;
        }
        if (prefix != null && suffix != null) {
            return prefix + suffix;
        } else {
            return null;
        }
    }

    public static boolean isNciOperationProprietary(int type, int gid, int oid) {
        if (gid == 0b0011) {
            // 100000b-111111b
            return (oid & 0b111111) == 0b100000;
        } else if (gid == 0b0100) {
            // 100000b-111111b
            return (oid & 0b111111) == 0b100000;
        } else if (gid == 0b1111) {
            // Proprietary group
            return true;
        }
        return false;
    }
}
