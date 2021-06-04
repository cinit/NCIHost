package cc.ioctl.nfcncihost.activity.splash;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;

import androidx.activity.OnBackPressedCallback;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Locale;

import cc.ioctl.nfcncihost.R;
import cc.ioctl.nfcncihost.util.ThreadManager;

public class WarnFragment extends SplashActivity.AbsInteractiveStepFragment {
    private static final String PREF_BOOL_USAGE_WARN = "PREF_BOOL_ABOUT_WARN";

    private final OnBackPressedCallback mOnBackPressedCallback = new OnBackPressedCallback(true) {
        @Override
        public void handleOnBackPressed() {
            WarnFragment.this.handleOnBackPressed();
        }
    };

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        ViewGroup root = (ViewGroup) inflater.inflate(R.layout.frag_warn_app_risk, container, false);
        final CheckBox cb = root.findViewById(R.id.dialog_warnAppRisk_informed);
        View tvContinue = root.findViewById(android.R.id.button1);
        View tvCancel = root.findViewById(android.R.id.button2);
        tvContinue.setOnClickListener(v -> {
            mOnBackPressedCallback.setEnabled(false);
            mOnBackPressedCallback.remove();
            config.putBoolean(PREF_BOOL_USAGE_WARN, true);
            activity.switchToNextStep();
        });
        tvCancel.setOnClickListener(v -> {
            activity.abortInteractiveStartup();
        });
        tvContinue.setEnabled(false);
        cb.setOnCheckedChangeListener((self, checked) -> tvContinue.setEnabled(checked));
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
        requireActivity().getOnBackPressedDispatcher().addCallback(mOnBackPressedCallback);
        return root;
    }

    public void handleOnBackPressed() {
        activity.abortInteractiveStartup();
    }

    @Override
    public void onDestroy() {
        mOnBackPressedCallback.setEnabled(false);
        mOnBackPressedCallback.remove();
        super.onDestroy();
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
