package cc.ioctl.nfcncihost.activity.splash;

import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;

import java.util.Locale;

import cc.ioctl.nfcncihost.R;
import cc.ioctl.nfcncihost.procedure.MainApplicationImpl;
import cc.ioctl.nfcncihost.procedure.StartupDirector;
import cc.ioctl.nfcncihost.util.ThreadManager;

public class WarnFragment extends SplashActivity.AbsInteractiveStepFragment {
    private static final String PREF_BOOL_USAGE_WARN = "PREF_BOOL_ABOUT_WARN";

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        final ViewGroup view = (ViewGroup) LayoutInflater.from(builder.getContext()).inflate(R.layout.dialog_warn_app_risk, null);
        final CheckBox cb = view.findViewById(R.id.dialog_warnAppRisk_informed);
        AlertDialog dialog = builder.setTitle(R.string.title_warning).setView(view)
                .setPositiveButton(R.string.btn_continue, (dailog, which) -> {
                    config.putBoolean(PREF_BOOL_USAGE_WARN, true);
                    activity.switchToNextStep();
                })
                .setNegativeButton(android.R.string.cancel, (dailog, which) -> {
                    activity.finish();
                    StartupDirector director = MainApplicationImpl.sDirector;
                    if (director != null) {
                        director.cancelAllPendingActivity();
                    }
                })
                .setCancelable(false).show();
        final Button btn = dialog.getButton(DialogInterface.BUTTON_POSITIVE);
        btn.setEnabled(false);
        cb.setOnCheckedChangeListener((self, checked) -> btn.setEnabled(checked));
        cb.setEnabled(false);
        ThreadManager.execute(() -> {
            int j = 5;
            while (j > 0) {
                final int i = j;
                ThreadManager.post(() -> cb.setText(String.format(Locale.ROOT, "%s(%ds)",
                        cb.getContext().getText(R.string.checkbox_accept_risk), i)));
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
                j--;
            }
            ThreadManager.post(() -> {
                cb.setText(R.string.checkbox_accept_risk);
                cb.setEnabled(true);
            });
        });
        return null;
    }

    @Override
    public boolean isDone() {
        return config.getBoolean(PREF_BOOL_USAGE_WARN, false);
    }

    @Override
    public int getOrder() {
        return 20;
    }
}
