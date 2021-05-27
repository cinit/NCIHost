package cc.ioctl.nfcncihost.service;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import androidx.annotation.NonNull;

import cc.ioctl.nfcncihost.ipc.INfcControllerManager;

public class NfcControllerManagerService extends Service {

    private final Object mLock = new Object();

    private final INfcControllerManager.Stub mBinder = new INfcControllerManager.Stub() {

        @Override
        public boolean requestStartDaemon() {
            return false;
        }

        @Override
        public int getDaemonPid() {
            return 0;
        }

        @Override
        public int getServicePid() {
            return android.os.Process.myPid();
        }

        @Override
        public boolean hasSupportedDevice() {
            return false;
        }

        @Override
        public int getNfcOperatingMode() {
            return 0;
        }

        @Override
        public int setNfcOperatingMode(int mode) {
            return 0;
        }
    };

    @NonNull
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }
}
