package cc.ioctl.nfcncihost.activity.ui.dump;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

public class NciDumpViewModel extends ViewModel {

    private MutableLiveData<String> mText;

    public NciDumpViewModel() {
        mText = new MutableLiveData<>();
        mText.setValue("This is gallery fragment");
    }

    public LiveData<String> getText() {
        return mText;
    }
}