package cc.ioctl.nfcncihost.activity.ui.cards;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;

import cc.ioctl.nfcncihost.R;

public class CardListFragment extends Fragment {

    private CardListViewModel slideshowViewModel;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        slideshowViewModel = new ViewModelProvider(this).get(CardListViewModel.class);
        View root = inflater.inflate(R.layout.fragment_main_card_list, container, false);
        final TextView textView = root.findViewById(R.id.text_slideshow);
        slideshowViewModel.getText().observe(getViewLifecycleOwner(), textView::setText);
        return root;
    }
}
