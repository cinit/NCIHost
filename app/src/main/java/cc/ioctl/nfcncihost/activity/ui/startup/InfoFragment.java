package cc.ioctl.nfcncihost.activity.ui.startup;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class InfoFragment extends TransientInitActivity.AbsInteractiveStepFragment {

    private static final String PREF_INT_LAST_VERSION_CODE = "PREF_KEY_LAST_VERSION_CODE";

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return null;
    }

    @Override
    public boolean isDone() {
//        return config.getInt(PREF_INT_LAST_VERSION_CODE, 0) < BuildConfig.VERSION_CODE;
        return true;
    }

    @Override
    public int getOrder() {
        return 1;
    }
}
