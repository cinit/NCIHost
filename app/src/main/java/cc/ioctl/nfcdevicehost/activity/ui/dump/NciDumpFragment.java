package cc.ioctl.nfcdevicehost.activity.ui.dump;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.DefaultItemAnimator;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;
import java.util.Objects;

import cc.ioctl.nfcdevicehost.R;
import cc.ioctl.nfcdevicehost.activity.MainUiFragmentActivity;
import cc.ioctl.nfcdevicehost.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.databinding.FragmentMainDumpBinding;
import cc.ioctl.nfcdevicehost.databinding.ItemMainNciDumpBinding;
import cc.ioctl.nfcdevicehost.decoder.NciPacketDecoder;
import cc.ioctl.nfcdevicehost.decoder.NxpHalV2EventTranslator;
import cc.ioctl.nfcdevicehost.util.ByteUtils;
import cc.ioctl.nfcdevicehost.util.SafUtils;

public class NciDumpFragment extends Fragment implements Observer<ArrayList<NxpHalV2EventTranslator.TransactionEvent>> {

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
            NxpHalV2EventTranslator.TransactionEvent event = mNciDumpViewModel.getTransactionEvents().getValue().get(position);
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

        @Override
        public int getItemCount() {
            return mNciDumpViewModel.getTransactionEvents().getValue().size();
        }
    }

    @SuppressLint("NotifyDataSetChanged")
    @Override
    public void onChanged(ArrayList<NxpHalV2EventTranslator.TransactionEvent> transactionEvents) {
//        int delta = transactionEvents.size() - mLastNciTransactionCount;
//        if (delta > 0) {
//            mDumpAdapter.notifyItemRangeInserted(mLastNciTransactionCount, transactionEvents.size() - mLastNciTransactionCount);
//        } else {
//            // remove items, should be empty now
        // I have no idea why this is would crash the app
        // Inconsistency detected. Invalid view holder adapter positionNciDumpViewHolder
        mDumpAdapter.notifyDataSetChanged();
//        }
//        mLastNciTransactionCount = transactionEvents.size();
    }

    private final NciDumpAdapter mDumpAdapter = new NciDumpAdapter();

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

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
        Activity activity = getActivity();
        if (activity instanceof MainUiFragmentActivity) {
            FloatingActionButton fab = ((MainUiFragmentActivity) activity).showFloatingActionButton();
            fab.setOnClickListener(v -> Snackbar.make(v, R.string.ui_toast_not_implemented, Snackbar.LENGTH_SHORT).show());
        }
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        if (daemon == null) {
            Snackbar.make(mBinding.getRoot(), R.string.ui_toast_daemon_is_not_running, Snackbar.LENGTH_LONG)
                    .setAction(R.string.ui_snackbar_action_view_or_jump, v -> jumpToHomeFragment()).show();
        } else {
            if (!daemon.isHwServiceConnected()) {
                Snackbar.make(mBinding.getRoot(), R.string.ui_toast_hal_service_not_attached, Snackbar.LENGTH_LONG)
                        .setAction(R.string.ui_snackbar_action_view_or_jump, v -> jumpToHomeFragment()).show();
            }
        }
    }

    @UiThread
    public void jumpToHomeFragment() {
        MainUiFragmentActivity activity = (MainUiFragmentActivity) requireActivity();
        activity.getNavController().navigate(R.id.nav_main_home);
    }

    @Override
    public void onCreateOptionsMenu(@NonNull Menu menu, @NonNull MenuInflater inflater) {
        requireActivity().getMenuInflater().inflate(R.menu.menu_main_fragment_dump, menu);
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_clear: {
                mNciDumpViewModel.clearTransactionEvents();
                return true;
            }
            case R.id.action_save: {
                if (Objects.requireNonNull(mNciDumpViewModel.getTransactionEvents().getValue()).isEmpty()) {
                    Snackbar.make(mBinding.getRoot(), R.string.ui_toast_no_data_to_save, Snackbar.LENGTH_SHORT).show();
                    return true;
                }
                String shortFileName = "nfc_hal_dump_" + System.currentTimeMillis() + ".txt";
                SafUtils.requestSaveFile(requireActivity()).setDefaultFileName(shortFileName).onResult(uri -> {
                    try {
                        StringBuilder data = mNciDumpViewModel.getTranslator().exportRawEventsAsCsv();
                        OutputStream os = requireContext().getContentResolver().openOutputStream(uri);
                        os.write(data.toString().getBytes(StandardCharsets.UTF_8));
                        os.flush();
                        os.close();
                        Snackbar.make(mBinding.getRoot(), R.string.ui_toast_data_saved, Snackbar.LENGTH_SHORT).show();
                    } catch (IOException e) {
                        AlertDialog.Builder builder = new AlertDialog.Builder(requireContext())
                                .setTitle(R.string.ui_dialog_error_title)
                                .setMessage(e.getMessage())
                                .setPositiveButton(android.R.string.ok, null);
                        builder.create().show();
                    }
                }).onCancel(() -> Toast.makeText(requireContext(), R.string.ui_toast_operation_canceled, Toast.LENGTH_SHORT).show()).commit();
                return true;
            }
            default:
                return super.onContextItemSelected(item);
        }
    }
}
