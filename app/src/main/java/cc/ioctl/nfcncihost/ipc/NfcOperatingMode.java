package cc.ioctl.nfcncihost.ipc;

public enum NfcOperatingMode {
    DEFAULT(0),
    STANDBY(1),
    READER(2),
    EMULATOR(3),
    RAW(4);

    private final int id;

    NfcOperatingMode(int i) {
        id = i;
    }

    public static NfcOperatingMode fromId(int id) {
        switch (id) {
            case 0:
                return DEFAULT;
            case 1:
                return STANDBY;
            case 2:
                return READER;
            case 3:
                return EMULATOR;
            case 4:
                return RAW;
            default:
                throw new IndexOutOfBoundsException("length=5, got " + id);
        }
    }

    public int getId() {
        return id;
    }
}
