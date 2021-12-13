package cc.ioctl.nfcdevicehost.activity.ui.dump;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.DefaultItemAnimator;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Locale;

import cc.ioctl.nfcdevicehost.NativeInterface;
import cc.ioctl.nfcdevicehost.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.databinding.FragmentMainDumpBinding;
import cc.ioctl.nfcdevicehost.databinding.ItemMainNciDumpBinding;
import cc.ioctl.nfcdevicehost.decoder.NciPacketDecoder;
import cc.ioctl.nfcdevicehost.util.ByteUtils;

public class NciDumpFragment extends Fragment implements Observer<ArrayList<NciDumpViewModel.TransactionEvent>> {

    private NciDumpViewModel mNciDumpViewModel;
    private FragmentMainDumpBinding mBinding;
    private int mLastNciTransactionCount = 0;

    static class NciDumpViewHolder extends RecyclerView.ViewHolder {
        public enum ViewType {
            TRANSACTION,
            IOCTL
        }

        public ViewType type;
        public ItemMainNciDumpBinding binding;

        public NciDumpViewHolder(ItemMainNciDumpBinding binding, ViewType type) {
            super(binding.getRoot());
            this.type = type;
            this.binding = binding;
        }
    }

    class NciDumpAdapter extends RecyclerView.Adapter<NciDumpViewHolder> {
        @NonNull
        @Override
        public NciDumpViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            ItemMainNciDumpBinding binding = ItemMainNciDumpBinding.inflate(LayoutInflater.from(parent.getContext()),
                    parent, false);
            return new NciDumpViewHolder(binding, NciDumpViewHolder.ViewType.TRANSACTION);
        }

        @Override
        public void onBindViewHolder(@NonNull NciDumpViewHolder holder, int position) {
            NciDumpViewModel.TransactionEvent event = mNciDumpViewModel.getTransactionEvents().getValue().get(position);
            String seqTime = String.format(Locale.ROOT, "#%d ", event.sequence)
                    + String.format(Locale.ROOT, "%1$tH:%1$tM:%1$tS.%1$tL", event.timestamp);
            String typeText = "<INVALID>";
            StringBuilder dataText = new StringBuilder();
            if (event instanceof NciDumpViewModel.IoctlTransactionEvent) {
                typeText = "IOCTL";
                NciDumpViewModel.IoctlTransactionEvent ev = (NciDumpViewModel.IoctlTransactionEvent) event;
                dataText.append("request: 0x").append(Integer.toHexString(ev.request));
                dataText.append(" arg: 0x").append(Long.toHexString(ev.arg));
            } else if (event instanceof NciDumpViewModel.RawTransactionEvent) {
                typeText = "???";
                NciDumpViewModel.RawTransactionEvent ev = (NciDumpViewModel.RawTransactionEvent) event;
                dataText.append(ev.direction);
                dataText.append("\n");
                dataText.append(ByteUtils.bytesToHexString(ev.data));
            } else if (event instanceof NciDumpViewModel.NciTransactionEvent) {
                NciDumpViewModel.NciTransactionEvent ev = (NciDumpViewModel.NciTransactionEvent) event;
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
                            name += "(" + Integer.toHexString(pk.groupId) + ":" + Integer.toHexString(pk.opcodeId) + ")";
                        }
                        dataText.append(name).append('\n');
                        dataText.append("\npayload(").append(pk.data.length).append("): \n");
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

        @Override
        public int getItemCount() {
            return mNciDumpViewModel.getTransactionEvents().getValue().size();
        }
    }

    @Override
    public void onChanged(ArrayList<NciDumpViewModel.TransactionEvent> transactionEvents) {
        mDumpAdapter.notifyItemRangeInserted(mLastNciTransactionCount, transactionEvents.size() - mLastNciTransactionCount);
        mLastNciTransactionCount = transactionEvents.size();
    }

    private final NciDumpAdapter mDumpAdapter = new NciDumpAdapter();

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        Context context = inflater.getContext();
        mNciDumpViewModel = new ViewModelProvider(this).get(NciDumpViewModel.class);
        mBinding = FragmentMainDumpBinding.inflate(inflater, container, false);
        mNciDumpViewModel.getTransactionEvents().observe(getViewLifecycleOwner(), this);
        LinearLayoutManager layoutManager = new LinearLayoutManager(context);
        RecyclerView recyclerView = mBinding.recyclerViewMainFragmentDumpList;
        recyclerView.setLayoutManager(layoutManager);
        layoutManager.setOrientation(RecyclerView.VERTICAL);
        recyclerView.addItemDecoration(new DividerItemDecoration(context, DividerItemDecoration.VERTICAL));
        recyclerView.setItemAnimator(new DefaultItemAnimator());
        mBinding.recyclerViewMainFragmentDumpList.setAdapter(mDumpAdapter);
        return mBinding.getRoot();
    }

    @Override
    public void onResume() {
        super.onResume();
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        if (daemon != null) {
            if (!daemon.isHwServiceConnected()) {
                StringBuilder sb = new StringBuilder();
                try {
                    // TODO: 2021-12-11 dynamically detect the architecture of the HW service
                    File patchFile = NativeInterface.getNfcHalServicePatchFile(
                            NativeInterface.NfcHalServicePatch.NXP_PATCH,
                            NativeInterface.ABI.ABI_ARM_64);
                    boolean isNfcConn = daemon.initHwServiceConnection(patchFile.getAbsolutePath());
                    sb.append("isNfcConn: ").append(isNfcConn).append("\n");
                } catch (RuntimeException | IOException re) {
                    sb.append("initHwServiceConnection failed: ").append(re);
                }
                Toast.makeText(getActivity(), sb.toString(), Toast.LENGTH_SHORT).show();
            }
            mNciDumpViewModel.synchronizeIoEvents();
        } else {
            Toast.makeText(getActivity(), "failed to connect to daemon", Toast.LENGTH_SHORT).show();
        }
    }
}
