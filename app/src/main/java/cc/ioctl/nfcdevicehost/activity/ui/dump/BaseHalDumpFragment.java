package cc.ioctl.nfcdevicehost.activity.ui.dump;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.RecyclerView;

import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;

import cc.ioctl.nfcdevicehost.R;
import cc.ioctl.nfcdevicehost.decoder.IoctlDecoder;
import cc.ioctl.nfcdevicehost.decoder.NciPacketDecoder;
import cc.ioctl.nfcdevicehost.decoder.NxpHalV2EventTranslator;
import cc.ioctl.nfcdevicehost.ipc.daemon.INciHostDaemon;
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

    protected void updateListViewItemForAuxIoEvent(@NonNull NciDumpViewHolder holder,
                                                   @NonNull INciHostDaemon.IoEventPacket event) {
        long timestamp = event.timestamp;
        String seqTime;
        Date now = new Date();
        Date seqTimeDate = new Date(timestamp);
        String sequenceStr = String.format(Locale.ROOT, "#%d/%s-%d", event.sequence,
                INciHostDaemon.IoEventPacket.SourceType.getShortString(event.sourceType), event.sourceSequence);
        if (now.getYear() == seqTimeDate.getYear() && now.getMonth() == seqTimeDate.getMonth()
                && now.getDate() == seqTimeDate.getDate()) {
            // the same day, HH:mm:ss.SSS
            seqTime = sequenceStr + " "
                    + String.format(Locale.ROOT, "%1$tH:%1$tM:%1$tS.%1$tL", event.timestamp);
        } else {
            // not the same day, yyyy-MM-dd HH:mm:ss.SSS
            seqTime = sequenceStr + " "
                    + String.format(Locale.ROOT, "%1$tY-%1$tm-%1$td %1$tH:%1$tM:%1$tS.%1$tL", event.timestamp);
        }
        String eventType = event.opType.toString();
        StringBuilder sb = new StringBuilder();
        switch (event.opType) {
            case OPEN: {
                String path = event.auxPath;
                sb.append(String.format(Locale.ROOT, "open(%s)=%d", path, event.retValue));
                break;
            }
            case CLOSE: {
                sb.append(String.format(Locale.ROOT, "close(%d)", event.fd));
                break;
            }
            case READ: {
                sb.append(String.format(Locale.ROOT, "read(%d)=%d", event.fd, event.retValue));
                sb.append("\ndata(").append(event.directArg2).append(")").append('\n');
                sb.append(ByteUtils.bytesToHexString(event.buffer));
                break;
            }
            case WRITE: {
                sb.append(String.format(Locale.ROOT, "write(%d)=%d", event.fd, event.retValue));
                sb.append("\ndata(").append(event.directArg2).append(")").append('\n');
                sb.append(ByteUtils.bytesToHexString(event.buffer));
                break;
            }
            case IOCTL: {
                sb.append(String.format(Locale.ROOT, "ioctl(%d, 0x%x, 0x%x)=%d",
                        event.fd, event.directArg1, event.directArg2, event.retValue));
                sb.append('\n');
                String name = IoctlDecoder.getIoctlRequestName((int) event.directArg1);
                if (name != null) {
                    sb.append(name).append(' ');
                }
                sb.append(IoctlDecoder.requestToString((int) event.directArg1));
                break;
            }
            case SELECT: {
                sb.append(String.format(Locale.ROOT, "select(...)=%d", event.retValue));
                break;
            }
            default: {
                sb.append(String.format(Locale.ROOT, "%s(%d, %d, %d)=%d",
                        event.opType, event.fd, event.directArg1, event.directArg2, event.retValue));
            }
        }
        holder.binding.textViewItemNciDumpTime.setText(seqTime);
        holder.binding.textViewItemNciDumpType.setText(eventType);
        holder.binding.textViewItemNciDumpMessage.setText(sb.toString());
    }

    protected void updateListViewItemForNciPacket(@NonNull NciDumpViewHolder holder,
                                                  @NonNull NxpHalV2EventTranslator.TransactionEvent event) {
        long timestamp = event.timestamp;
        String seqTime;
        Date now = new Date();
        Date seqTimeDate = new Date(timestamp);
        String sequenceStr = String.format(Locale.ROOT, "#%d/%s-%d", event.sequence,
                INciHostDaemon.IoEventPacket.SourceType.getShortString(event.sourceType), event.sourceSequence);
        if (now.getYear() == seqTimeDate.getYear() && now.getMonth() == seqTimeDate.getMonth()
                && now.getDate() == seqTimeDate.getDate()) {
            // the same day, HH:mm:ss.SSS
            seqTime = sequenceStr + " "
                    + String.format(Locale.ROOT, "%1$tH:%1$tM:%1$tS.%1$tL", event.timestamp);
        } else {
            // not the same day, yyyy-MM-dd HH:mm:ss.SSS
            seqTime = sequenceStr + " "
                    + String.format(Locale.ROOT, "%1$tY-%1$tm-%1$td %1$tH:%1$tM:%1$tS.%1$tL", event.timestamp);
        }
        String typeText = "<INVALID>";
        StringBuilder dataText = new StringBuilder();
        if (event instanceof NxpHalV2EventTranslator.RawTransactionEvent) {
            // raw packet
            INciHostDaemon.IoEventPacket packet = ((NxpHalV2EventTranslator.RawTransactionEvent) event).packet;
            updateListViewItemForAuxIoEvent(holder, packet);
            return;
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

        public abstract int getEventSequenceByPosition(int position);

        public abstract int findEventPositionBySequence(int sequence);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @UiThread
    protected void showDecoderOptionDialog() {
        int currentDecoder = getCurrentDecoderIndex();
        new AlertDialog.Builder(requireContext())
                .setTitle(R.string.ui_hal_dump_decoder_config_title)
                .setSingleChoiceItems(R.array.ui_hal_dump_decoder_config_items, currentDecoder, null)
                .setCancelable(true)
                .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                    int newDecoder = ((AlertDialog) dialog).getListView().getCheckedItemPosition();
                    if (newDecoder != -1 && newDecoder != currentDecoder) {
                        setCurrentDecoderIndex(newDecoder);
                    }
                })
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }

    protected abstract int getCurrentDecoderIndex();

    @UiThread
    protected abstract void setCurrentDecoderIndex(int index);

    @Override
    public void onCreateOptionsMenu(@NonNull Menu menu, @NonNull MenuInflater inflater) {
        requireActivity().getMenuInflater().inflate(R.menu.menu_main_fragment_dump_common, menu);
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_decoder_config: {
                showDecoderOptionDialog();
                return true;
            }
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    /**
     * Find the index of the event list by the given event sequence.
     *
     * @param packets  the event list
     * @param sequence the sequence of the event
     * @return a non-negative index if found, otherwise the negative value of the index where the event should be inserted.
     */
    public static int findItemIndexBySequenceIoEventPacket(List<INciHostDaemon.IoEventPacket> packets, int sequence) {
        INciHostDaemon.IoEventPacket dummy = new INciHostDaemon.IoEventPacket();
        dummy.sequence = sequence;
        return Collections.binarySearch(packets, dummy, (o1, o2) -> {
            if (o1.sequence == o2.sequence) {
                return 0;
            }
            return o1.sequence < o2.sequence ? -1 : 1;
        });
    }

    public static int findItemIndexBySequenceTransactionEvent(List<NxpHalV2EventTranslator.TransactionEvent> packets, int sequence) {
        NxpHalV2EventTranslator.TransactionEvent dummy = new NxpHalV2EventTranslator.TransactionEvent();
        dummy.sequence = sequence;
        return Collections.binarySearch(packets, dummy, (o1, o2) -> {
            if (o1.sequence == o2.sequence) {
                return 0;
            }
            return o1.sequence < o2.sequence ? -1 : 1;
        });
    }

}
