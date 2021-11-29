package cc.ioctl.nfcncihost.daemon;

import java.io.IOException;

import cc.ioctl.nfcncihost.daemon.internal.NciHostDaemonProxy;

public interface INciHostDaemon {
    boolean isConnected();

    String getVersionName();

    int getVersionCode();

    String getBuildUuid();

    void exitProcess();

    boolean isDeviceSupported();

    boolean isHwServiceConnected();

    boolean initHwServiceConnection(String soPath) throws IOException;

    interface OnRemoteEventListener {
        void onIoEvent(NciHostDaemonProxy.IoEventPacket event);

        void onRemoteDeath(NciHostDaemonProxy.RemoteDeathPacket event);
    }

    OnRemoteEventListener getRemoteEventListener();

    void setRemoteEventListener(OnRemoteEventListener listener);

}
