package cc.ioctl.nfcncihost.daemon;

import java.io.IOException;

public interface INciHostDaemon {
    boolean isConnected();

    String getVersionName();

    int getVersionCode();

    String getBuildUuid();

    void exitProcess();

    boolean isDeviceSupported();

    boolean isHwServiceConnected();

    boolean initHwServiceConnection(String soPath) throws IOException;
}
