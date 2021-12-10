package cc.ioctl.nfcncihost.activity.ui.cards;

import androidx.lifecycle.ViewModelProvider;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import cc.ioctl.nfcncihost.R;

public class CardInfoFragment extends Fragment {

    private CardInfoViewModel mViewModel;

    public static CardInfoFragment newInstance() {
        return new CardInfoFragment();
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        // TODO: Use the ViewModel
        mViewModel = new ViewModelProvider(this).get(CardInfoViewModel.class);
        return inflater.inflate(R.layout.fragment_card_info, container, false);
    }

}
