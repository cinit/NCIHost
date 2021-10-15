package cc.ioctl.nfcncihost.activity.ui.home;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import cc.ioctl.nfcncihost.R;
import cc.ioctl.nfcncihost.daemon.INciHostDaemon;
import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;

public class HomeFragment extends Fragment {

    private HomeViewModel homeViewModel;
    static ExecutorService tp = Executors.newCachedThreadPool();

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        homeViewModel =
                new ViewModelProvider(this).get(HomeViewModel.class);
        View root = inflater.inflate(R.layout.fragment_home, container, false);
        final TextView textView = root.findViewById(R.id.text_home);
        homeViewModel.getText().observe(getViewLifecycleOwner(), new Observer<String>() {
            @Override
            public void onChanged(@Nullable String s) {
                textView.setText(s);
            }
        });
        Activity activity = getActivity();
        textView.setOnClickListener((v) -> {
            tp.execute(() -> {
                INciHostDaemon daemon = IpcNativeHandler.connect(200);
                if (daemon == null) {
                    activity.runOnUiThread(() -> {
                        Toast.makeText(activity, "connect timeout", Toast.LENGTH_SHORT).show();
                    });
                } else {
                    int val = daemon.testFunction(13);
                    activity.runOnUiThread(() -> {
                        Toast.makeText(activity, "val: " + val, Toast.LENGTH_SHORT).show();
                    });
                }
            });
        });
        return root;
    }
}
