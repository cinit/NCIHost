package cc.ioctl.nfcdevicehost.activity.ui.home;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;

import cc.ioctl.nfcdevicehost.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.databinding.FragmentHomeBinding;

public class HomeFragment extends Fragment {

    private HomeViewModel mHomeViewModel;
    private FragmentHomeBinding mHomeBinding;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        mHomeViewModel = new ViewModelProvider(this).get(HomeViewModel.class);
        mHomeBinding = FragmentHomeBinding.inflate(inflater, container, false);
        mHomeViewModel.isNciHostDaemonConnected().observe(getViewLifecycleOwner(), conn -> updateUiState());
        // UI state will be updated on resume
        return mHomeBinding.getRoot();
    }

    @UiThread
    private void updateUiState() {
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        StringBuilder sb = new StringBuilder();
        if (daemon != null) {
            sb.append("connected to daemon").append('\n');
            sb.append("daemon version: ").append(daemon.getVersionName()).append('\n');
            sb.append("isHalConnected: ").append(daemon.isHwServiceConnected()).append('\n');
        } else {
            sb.append("NciHostDaemon is not connected");
        }
        mHomeBinding.textViewHomeFragmentStatus.setText(sb.toString());
    }

    @Override
    public void onResume() {
        super.onResume();
        updateUiState();
    }
}
