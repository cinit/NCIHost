// INfcControllerManager.aidl
package cc.ioctl.nfcdevicehost.ipc;

interface INfcControllerManager {
    boolean requestStartDaemon();

    int getDaemonPid();

    int getServicePid();

    boolean hasSupportedDevice();

    int getNfcOperatingMode();

    int setNfcOperatingMode(int mode);
}