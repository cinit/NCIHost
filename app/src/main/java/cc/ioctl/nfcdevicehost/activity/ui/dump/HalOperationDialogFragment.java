package cc.ioctl.nfcdevicehost.activity.ui.dump;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.appcompat.app.AlertDialog;

import com.google.android.material.snackbar.Snackbar;

import java.io.ByteArrayOutputStream;

import cc.ioctl.nfcdevicehost.R;
import cc.ioctl.nfcdevicehost.activity.base.BaseBottomSheetDialogFragment;
import cc.ioctl.nfcdevicehost.databinding.DialogBsdHalOperationBinding;
import cc.ioctl.nfcdevicehost.ipc.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.ipc.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.util.Errno;
import cc.ioctl.nfcdevicehost.util.ThreadManager;

public class HalOperationDialogFragment extends BaseBottomSheetDialogFragment implements AdapterView.OnItemSelectedListener {

    private DialogBsdHalOperationBinding mBinding;

    @NonNull
    @Override
    protected View createRootView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                  @Nullable Bundle savedInstanceState) {
        mBinding = DialogBsdHalOperationBinding.inflate(inflater, container, false);
        mBinding.halOperationButtonSubmit.setOnClickListener(v -> submitOperation());
        mBinding.halOperationButtonClose.setOnClickListener(v -> dismiss());
        mBinding.halOperationSpinnerSyscallType.setAdapter(new ArrayAdapter<>(requireContext(),
                R.layout.item_spinner_text, R.id.item_spinner_item_textView, new String[]{"WRITE", "IOCTL"}));
        mBinding.halOperationSpinnerSyscallType.setOnItemSelectedListener(this);
        mBinding.halOperationSpinnerSyscallType.setSelection(0);
        return mBinding.getRoot();
    }

    @UiThread
    private void submitOperation() {
        int operationPos = mBinding.halOperationSpinnerSyscallType.getSelectedItemPosition();
        if (operationPos == 0) {
            // write
            String input = mBinding.halOperationEditTextWritePayload.getText().toString();
            String[] inputSplit = input.split("[，\n, |-]");
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            try {
                for (String s : inputSplit) {
                    if (s.isEmpty()) {
                        continue;
                    }
                    int value = Integer.parseInt(s, 16);
                    baos.write(value);
                }
            } catch (NumberFormatException e) {
                new AlertDialog.Builder(requireContext())
                        .setTitle(R.string.ui_dialog_error_title)
                        .setMessage(requireContext().getString(R.string.ui_dialog_error_message_invalid_input_syntax_v0s, e.getMessage()))
                        .setCancelable(true)
                        .setPositiveButton(android.R.string.ok, null)
                        .show();
                return;
            }
            byte[] bytes = baos.toByteArray();
            if (bytes.length > 0) {
                INciHostDaemon daemon = IpcNativeHandler.peekConnection();
                if (daemon != null && daemon.isHwServiceConnected()) {
                    ThreadManager.async(() -> {
                        try {
                            int result = daemon.deviceDriverWriteRaw(bytes);
                            ThreadManager.runOnUiThread(() -> {
                                if (result < 0) {
                                    String errMsg = Errno.getErrnoString(result);
                                    if (errMsg == null) {
                                        errMsg = "Unknown error: " + result;
                                    }
                                    String finalErrMsg = errMsg;
                                    ThreadManager.runOnUiThread(() -> new AlertDialog.Builder(requireContext())
                                            .setTitle(R.string.ui_dialog_error_title)
                                            .setMessage(finalErrMsg)
                                            .setCancelable(true)
                                            .setPositiveButton(android.R.string.ok, null)
                                            .show());
                                } else {
                                    dismiss();
                                    Snackbar.make(requireActivity().getWindow().getDecorView(),
                                            requireContext().getString(R.string.ui_toast_operation_success_v0d, result),
                                            Snackbar.LENGTH_SHORT).show();
                                }
                            });
                        } catch (Exception e) {
                            ThreadManager.runOnUiThread(() -> new AlertDialog.Builder(requireContext())
                                    .setTitle(R.string.ui_dialog_error_title)
                                    .setMessage(e.getMessage())
                                    .setCancelable(true)
                                    .setPositiveButton(android.R.string.ok, null)
                                    .show());
                        }
                    });
                } else {
                    new AlertDialog.Builder(requireContext())
                            .setTitle(R.string.ui_dialog_error_title)
                            .setMessage(R.string.ui_toast_hal_service_not_attached)
                            .setCancelable(true)
                            .setPositiveButton(android.R.string.ok, null)
                            .show();
                    dismiss();
                }
            }
        } else if (operationPos == 1) {
            // ioctl
            String requestStr = mBinding.halOperationEditTextIoctlRequest.getText().toString();
            String argStr = mBinding.halOperationEditTextIoctlArg.getText().toString();
            int requestValue;
            long argValue;
            try {
                requestValue = Integer.parseUnsignedInt(requestStr, 16);
                argValue = Long.parseUnsignedLong(argStr, 16);
            } catch (NumberFormatException e) {
                new AlertDialog.Builder(requireContext())
                        .setTitle(R.string.ui_dialog_error_title)
                        .setMessage(requireContext().getString(R.string.ui_dialog_error_message_invalid_input_syntax_v0s, e.getMessage()))
                        .setCancelable(true)
                        .setPositiveButton(android.R.string.ok, null)
                        .show();
                return;
            }
            INciHostDaemon daemon = IpcNativeHandler.peekConnection();
            if (daemon != null && daemon.isHwServiceConnected()) {
                ThreadManager.async(() -> {
                    try {
                        int result = daemon.deviceDriverIoctl0(requestValue, argValue);
                        ThreadManager.runOnUiThread(() -> {
                            if (result < 0) {
                                String errMsg = Errno.getErrnoString(result);
                                if (errMsg == null) {
                                    errMsg = "Unknown error: " + result;
                                }
                                String finalErrMsg = errMsg;
                                ThreadManager.runOnUiThread(() -> new AlertDialog.Builder(requireContext())
                                        .setTitle(R.string.ui_dialog_error_title)
                                        .setMessage(finalErrMsg)
                                        .setCancelable(true)
                                        .setPositiveButton(android.R.string.ok, null)
                                        .show());
                            } else {
                                dismiss();
                                Snackbar.make(requireActivity().getWindow().getDecorView(),
                                        requireContext().getString(R.string.ui_toast_operation_success_v0d, result),
                                        Snackbar.LENGTH_SHORT).show();
                            }
                        });
                    } catch (Exception e) {
                        ThreadManager.runOnUiThread(() -> new AlertDialog.Builder(requireContext())
                                .setTitle(R.string.ui_dialog_error_title)
                                .setMessage(e.getMessage())
                                .setCancelable(true)
                                .setPositiveButton(android.R.string.ok, null)
                                .show());
                    }
                });
            } else {
                new AlertDialog.Builder(requireContext())
                        .setTitle(R.string.ui_dialog_error_title)
                        .setMessage(R.string.ui_toast_hal_service_not_attached)
                        .setCancelable(true)
                        .setPositiveButton(android.R.string.ok, null)
                        .show();
                dismiss();
            }
        }
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        if (id == 0) {
            // write
            mBinding.halOperationLinearLayoutWrite.setVisibility(View.VISIBLE);
            mBinding.halOperationLinearLayoutIoctl.setVisibility(View.GONE);
        } else if (id == 1) {
            // ioctl
            mBinding.halOperationLinearLayoutWrite.setVisibility(View.GONE);
            mBinding.halOperationLinearLayoutIoctl.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {

    }
}
