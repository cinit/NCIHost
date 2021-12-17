package cc.ioctl.nfcdevicehost.activity.ui.dump;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.DefaultItemAnimator;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

import cc.ioctl.nfcdevicehost.R;
import cc.ioctl.nfcdevicehost.activity.MainUiFragmentActivity;
import cc.ioctl.nfcdevicehost.ipc.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.databinding.FragmentMainDumpBinding;
import cc.ioctl.nfcdevicehost.decoder.NxpHalV2EventTranslator;
import cc.ioctl.nfcdevicehost.util.ThreadManager;

/**
 * Fragment for showing a dump file.
 * <p>
 * Host Activity may be {@link MainUiFragmentActivity}
 * or {@link cc.ioctl.nfcdevicehost.activity.SidebandHostActivity}
 */
public class HalDumpFileViewFragment extends BaseHalDumpFragment {

    public static final String EXTRA_CONTENT = HalDumpFileViewFragment.class.getName() + ".EXTRA_CONTENT";

    private String mRawSerializedDumpData = null;
    private ArrayList<INciHostDaemon.IoEventPacket> mRawIoEvents = null;
    private ArrayList<NxpHalV2EventTranslator.TransactionEvent> mTransactionEvents = null;

    private FragmentMainDumpBinding mBinding;
    private final AbsNciDumpAdapter mDumpAdapter = new AbsNciDumpAdapter() {
        @Override
        public void onBindViewHolder(@NonNull NciDumpViewHolder holder, int position) {
            NxpHalV2EventTranslator.TransactionEvent event = mTransactionEvents.get(position);
            updateListViewItem(holder, event);
        }

        @Override
        public int getItemCount() {
            return mTransactionEvents == null ? 0 : mTransactionEvents.size();
        }
    };

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        Context context = inflater.getContext();
        mBinding = FragmentMainDumpBinding.inflate(inflater, container, false);
        LinearLayoutManager layoutManager = new LinearLayoutManager(context);
        RecyclerView recyclerView = mBinding.recyclerViewMainFragmentDumpList;
        recyclerView.setLayoutManager(layoutManager);
        layoutManager.setOrientation(RecyclerView.VERTICAL);
        recyclerView.addItemDecoration(new DividerItemDecoration(context, DividerItemDecoration.VERTICAL));
        recyclerView.setItemAnimator(new DefaultItemAnimator());
        mBinding.recyclerViewMainFragmentDumpList.setAdapter(mDumpAdapter);
        // load data file
        Bundle args = getArguments();
        final String uriPath = args == null ? null : args.getString(EXTRA_CONTENT);
        if (uriPath == null) {
            popSelf();
            return null;
        }
        Uri uri = Uri.parse(uriPath);
        final ContentResolver resolver = requireContext().getContentResolver();
        ThreadManager.async(() -> {
            try {
                InputStream is = resolver.openInputStream(uri);
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                byte[] buffer = new byte[1024];
                int len;
                while ((len = is.read(buffer)) > 0) {
                    baos.write(buffer, 0, len);
                }
                is.close();
                mRawSerializedDumpData = baos.toString();
                try {
                    mRawIoEvents = NxpHalV2EventTranslator.loadIoEventPacketsFromString(mRawSerializedDumpData);
                    NxpHalV2EventTranslator translator = new NxpHalV2EventTranslator();
                    translator.pushBackRawIoEvents(mRawIoEvents);
                    mTransactionEvents = translator.getTransactionEvents();
                    ThreadManager.runOnUiThread(() -> mDumpAdapter.notifyDataSetChanged());
                } catch (RuntimeException e) {
                    ThreadManager.runOnUiThread(() -> new AlertDialog.Builder(context)
                            .setTitle(R.string.ui_dialog_error_title)
                            .setMessage(e.toString())
                            .setCancelable(false)
                            .setPositiveButton(android.R.string.ok, (dialog1, which) -> popSelf())
                            .show());
                }
            } catch (IOException e) {
                ThreadManager.runOnUiThread(() -> new AlertDialog.Builder(context)
                        .setTitle(R.string.ui_dialog_error_title)
                        .setMessage(context.getString(R.string.ui_dialog_unable_to_open_file_v0s, uri.toString())
                                + "\n" + e)
                        .setCancelable(false)
                        .setPositiveButton(android.R.string.ok, (dialog1, which) -> popSelf())
                        .show());
            }
        });
        return mBinding.getRoot();
    }

    @Override
    public void onResume() {
        super.onResume();
        Activity activity = requireActivity();
        activity.setTitle(R.string.ui_title_main_dump);
        if (activity instanceof MainUiFragmentActivity) {
            ((MainUiFragmentActivity) activity).hideFloatingActionButton();
        }

    }

    private void popSelf() {
        requireActivity().onBackPressed();
    }

}
