package cc.ioctl.nfcncihost.daemon.internal;

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
    public native int testFunction(int value);
}
