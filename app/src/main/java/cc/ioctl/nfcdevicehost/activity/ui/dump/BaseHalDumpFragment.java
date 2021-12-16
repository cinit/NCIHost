package cc.ioctl.nfcdevicehost.activity.ui.dump;

import android.view.LayoutInflater;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.RecyclerView;

import java.util.Date;
import java.util.Locale;

import cc.ioctl.nfcdevicehost.decoder.NciPacketDecoder;
import cc.ioctl.nfcdevicehost.decoder.NxpHalV2EventTranslator;
import cc.ioctl.nfcdevicehost.util.ByteUtils;

public abstract class BaseHalDumpFragment extends Fragment {

    public static class NciDumpViewHolder extends RecyclerView.ViewHolder {
        public enum ViewType {
            TRANSACTION,
            IOCTL
        }

        public ViewType type;
        public cc.ioctl.nfcdevicehost.databinding.ItemMainNciDumpBinding binding;

        public NciDumpViewHolder(cc.ioctl.nfcdevicehost.databinding.ItemMainNciDumpBinding binding, ViewType type) {
            super(binding.getRoot());
            this.type = type;
            this.binding = binding;
        }
    }

    protected void updateListViewItem(@NonNull NciDumpViewHolder holder,
                                      @NonNull NxpHalV2EventTranslator.TransactionEvent event) {
        long timestamp = event.timestamp;
        String seqTime;
        Date now = new Date();
        Date seqTimeDate = new Date(timestamp);
        if (now.getYear() == seqTimeDate.getYear() && now.getMonth() == seqTimeDate.getMonth()
                && now.getDate() == seqTimeDate.getDate()) {
            // the same day, HH:mm:ss.SSS
            seqTime = String.format(Locale.ROOT, "#%d ", event.sequence)
                    + String.format(Locale.ROOT, "%1$tH:%1$tM:%1$tS.%1$tL", event.timestamp);
        } else {
            // not the same day, yyyy-MM-dd HH:mm:ss.SSS
            seqTime = String.format(Locale.ROOT, "#%d ", event.sequence)
                    + String.format(Locale.ROOT, "%1$tY-%1$tm-%1$td %1$tH:%1$tM:%1$tS.%1$tL", event.timestamp);
        }
        String typeText = "<INVALID>";
        StringBuilder dataText = new StringBuilder();
        if (event instanceof NxpHalV2EventTranslator.IoctlTransactionEvent) {
            typeText = "IOCTL";
            NxpHalV2EventTranslator.IoctlTransactionEvent ev = (NxpHalV2EventTranslator.IoctlTransactionEvent) event;
            dataText.append("request: 0x").append(Integer.toHexString(ev.request));
            dataText.append(" arg: 0x").append(Long.toHexString(ev.arg));
        } else if (event instanceof NxpHalV2EventTranslator.RawTransactionEvent) {
            typeText = "???";
            NxpHalV2EventTranslator.RawTransactionEvent ev = (NxpHalV2EventTranslator.RawTransactionEvent) event;
            dataText.append(ev.direction);
            dataText.append("\n");
            dataText.append(ByteUtils.bytesToHexString(ev.data));
        } else if (event instanceof NxpHalV2EventTranslator.NciTransactionEvent) {
            NxpHalV2EventTranslator.NciTransactionEvent ev = (NxpHalV2EventTranslator.NciTransactionEvent) event;
            switch (ev.type) {
                case NCI_DATA: {
                    NciPacketDecoder.NciDataPacket pk = (NciPacketDecoder.NciDataPacket) ev.packet;
                    typeText = "DAT";
                    dataText.append("conn: ").append(pk.connId).append("  ").append("credits: ").append(pk.credits);
                    dataText.append("\npayload(").append(pk.data.length).append("): \n");
                    dataText.append(ByteUtils.bytesToHexString(pk.data));
                    break;
                }
                case NCI_NTF:
                case NCI_RSP:
                case NCI_CMD: {
                    NciPacketDecoder.NciControlPacket pk = (NciPacketDecoder.NciControlPacket) ev.packet;
                    typeText = (pk.type == NciPacketDecoder.Type.NCI_CMD) ? "CMD"
                            : ((pk.type == NciPacketDecoder.Type.NCI_NTF) ? "NTF" : "RSP");
                    int msgType = pk.type.getInt();
                    String name = NciPacketDecoder.getNciOperationName(msgType, pk.groupId, pk.opcodeId);
                    if (name == null) {
                        name = "Unknown";
                        if (NciPacketDecoder.isNciOperationProprietary(msgType, pk.groupId, pk.opcodeId)) {
                            name += " Proprietary";
                        }
                        name += " " + String.format(Locale.ROOT, "GID:0x%02X", pk.groupId)
                                + " " + String.format(Locale.ROOT, "OID:0x%02X", pk.opcodeId);
                    } else {
                        name += " (" + String.format(Locale.ROOT, "0x%02X", pk.groupId)
                                + "/" + String.format(Locale.ROOT, "0x%02X", pk.opcodeId) + ")";
                    }
                    dataText.append(name);
                    dataText.append("\npayload(").append(pk.data.length).append(")\n");
                    dataText.append(ByteUtils.bytesToHexString(pk.data));
                    break;
                }
                default: {
                    typeText = "<UNKNOWN>";
                    dataText.append(ev.packet.toString());
                }
            }
        }
        holder.binding.textViewItemNciDumpTime.setText(seqTime);
        holder.binding.textViewItemNciDumpType.setText(typeText);
        holder.binding.textViewItemNciDumpMessage.setText(dataText.toString());
    }

    public abstract static class AbsNciDumpAdapter extends RecyclerView.Adapter<NciDumpViewHolder> {
        @NonNull
        @Override
        public NciDumpViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            cc.ioctl.nfcdevicehost.databinding.ItemMainNciDumpBinding binding = cc.ioctl.nfcdevicehost.databinding.ItemMainNciDumpBinding.inflate(LayoutInflater.from(parent.getContext()),
                    parent, false);
            return new NciDumpViewHolder(binding, NciDumpViewHolder.ViewType.TRANSACTION);
        }
    }

}
