package cc.ioctl.nfcdevicehost.ipc;

public class RemoteLowLevelOperationException extends Exception {
    public RemoteLowLevelOperationException() {
        super();
    }

    public RemoteLowLevelOperationException(String message) {
        super(message);
    }

    public RemoteLowLevelOperationException(String message, Throwable cause) {
        super(message, cause);
    }

    public RemoteLowLevelOperationException(Throwable cause) {
        super(cause);
    }
}
