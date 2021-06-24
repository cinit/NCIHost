package cc.ioctl.nfcncihost.ipc;

import androidx.annotation.Nullable;

public class RemoteDaemonException extends Exception {
    private final int typeId;
    private final int statusCode;

    public RemoteDaemonException(int type, int status) {
        super(type + ":" + status);
        typeId = type;
        statusCode = status;
    }

    public RemoteDaemonException(int type, int status, @Nullable String msg) {
        super((msg != null && msg.length() > 0) ?
                (type + ":" + status + " " + msg) : (type + ":" + status));
        typeId = type;
        statusCode = status;
    }

    public int getTypeId() {
        return typeId;
    }

    public int getStatusCode() {
        return statusCode;
    }
}
