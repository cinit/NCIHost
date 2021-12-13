package cc.ioctl.nfcdevicehost.ipc;

import android.os.IBinder;
import android.os.RemoteException;

public class NfcControllerManager {

    private final INfcControllerManager mManager;

    public static final int ERR_NO_DEVICE = 1;
    public static final int ERR_PERMISSION = 2;
    public static final int ERR_UNSUPPORTED = 3;


    public NfcControllerManager(IBinder binder) throws RemoteException {
        if (!binder.isBinderAlive()) {
            throw new RemoteException();
        }
        mManager = INfcControllerManager.Stub.asInterface(binder);
    }

    public int getDaemonPid() throws RemoteException {
        return mManager.getDaemonPid();
    }

    public int getServicePid() throws RemoteException {
        return mManager.getServicePid();
    }

    public boolean hasSupportedDevice() throws RemoteException {
        return mManager.hasSupportedDevice();
    }

    public NfcOperatingMode getNfcOperatingMode() throws RemoteException {
        return NfcOperatingMode.fromId(mManager.getNfcOperatingMode());
    }

    public void setNfcOperatingMode(NfcOperatingMode mode) throws RemoteException,
            NoDeviceFoundException, RemoteLowLevelOperationException {
        int id = mode.getId();
        int error = mManager.setNfcOperatingMode(id);
        switch (error) {
            case 0:
                break;
            case 1:
                throw new NoDeviceFoundException();
            case 2:
            case 3:
                throw new RemoteLowLevelOperationException("err = " + error);
            default:
                throw new IllegalArgumentException("invalid resp err = " + error);
        }
    }

}
