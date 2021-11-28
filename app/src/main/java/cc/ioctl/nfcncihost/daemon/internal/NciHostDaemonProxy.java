package cc.ioctl.nfcncihost.daemon.internal;

import java.io.IOException;

import cc.ioctl.nfcncihost.daemon.INciHostDaemon;

public class NciHostDaemonProxy implements INciHostDaemon {
    @Override
    public native boolean isConnected();

    @Override
    public native String getVersionName();

    @Override
    public native int getVersionCode();

    @Override
    public native String getBuildUuid();

    @Override
    public native void exitProcess();

    @Override
    public native boolean isDeviceSupported();

    @Override
    public native boolean isHwServiceConnected();

    @Override
    public native boolean initHwServiceConnection(String soPath) throws IOException;
}
