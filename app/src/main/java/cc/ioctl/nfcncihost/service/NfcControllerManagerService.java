package cc.ioctl.nfcncihost.service;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import androidx.annotation.NonNull;

public class NfcControllerManagerService extends Service {

    @NonNull
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
