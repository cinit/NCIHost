package cc.ioctl.nfcncihost.service;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.NonNull;

import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;
import cc.ioctl.nfcncihost.ipc.INfcControllerManager;
import cc.ioctl.nfcncihost.ipc.NfcControllerManager;
import cc.ioctl.nfcncihost.ipc.NfcOperatingMode;
import cc.ioctl.nfcncihost.procedure.WorkerApplicationImpl;

public class NfcControllerManagerService extends Service {

    private final Object mLock = new Object();
    private static final String TAG = "NfcCtrlMgrSvc";

    private final INfcControllerManager.Stub mBinder = new INfcControllerManager.Stub() {

        @Override
        public boolean requestStartDaemon() {
            IpcNativeHandler.initForSocketDir();
            return IpcNativeHandler.isConnected();
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
            return NfcOperatingMode.DEFAULT.getId();
        }

        @Override
        public int setNfcOperatingMode(int mode) {
            return NfcControllerManager.ERR_NO_DEVICE;
        }
    };

    @NonNull
    @Override
    public IBinder onBind(Intent intent) {
        IpcNativeHandler.init(this);
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "onCreate");
        WorkerApplicationImpl.getInstance().initIpcSocketAsync();
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy");
        super.onDestroy();
    }
}
