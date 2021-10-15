package cc.ioctl.nfcncihost.daemon;

public interface INciHostDaemon {
    boolean isConnected();

    String getVersionName();

    int getVersionCode();

    String getBuildUuid();

    void exitProcess();

    int testFunction(int value);
}
