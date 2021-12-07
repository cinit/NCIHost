package cc.ioctl.nfcncihost.activity.ui.dump;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;

import cc.ioctl.nfcncihost.R;

public class NciDumpFragment extends Fragment {

    private NciDumpViewModel nciDumpViewModel;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        nciDumpViewModel = new ViewModelProvider(this).get(NciDumpViewModel.class);
        View root = inflater.inflate(R.layout.fragment_gallery, container, false);
        final TextView textView = root.findViewById(R.id.text_gallery);
        nciDumpViewModel.getText().observe(getViewLifecycleOwner(), textView::setText);
        return root;
    }
}
