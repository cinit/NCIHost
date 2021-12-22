package cc.ioctl.nfcdevicehost.xposed;

import android.app.AndroidAppHelper;
import android.content.Context;
import android.net.Credentials;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import de.robv.android.xposed.XposedBridge;

public class SysNfcSvcPatch {

    private static final String TAG = "SysNfcSvcPatch";
    private static final String PREF_DISABLE_NFC_DISCOVERY_SOUND = "disable_nfc_discovery_sound";

    private static Thread sWorkingThread = null;
    public static boolean sDisableNfcDiscoverySound = false;
    private static boolean sShouldRunning = true;

    static void run() {
        try {
            // wait for system service to start
            Thread.sleep(3000);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return;
        }
        try {
            initMisc();
        } catch (RuntimeException | LinkageError e) {
            XposedBridge.log("SysNfcSvcPatch: " + e.getMessage());
        }
        try {
            int myPid = android.os.Process.myPid();
            Log.d(TAG, "SysNfcSvcPatch.run: myPid = " + myPid);
            while (sShouldRunning) {
                LocalServerSocket localServerSocket;
                try {
                    localServerSocket = new LocalServerSocket("com.android.nfc/cc.ioctl.nfcdevicehost.xposed.SysNfcSvcPatch@" + myPid);
                    while (sShouldRunning) {
                        Log.i(TAG, "SysNfcSvcPatch.run: waiting for connection");
                        LocalSocket localSocket = localServerSocket.accept();
                        Credentials cred = null;
                        try {
                            cred = localSocket.getPeerCredentials();
                        } catch (IOException ignored) {
                        }
                        if (cred != null && cred.getUid() == 0) {
                            Log.i(TAG, "SysNfcSvcPatch.run: connection accepted, uid = " + cred.getUid()
                                    + ", pid = " + cred.getPid() + ", gid = " + cred.getGid());
                            localServerSocket.close();
                            handleTransaction(localSocket);
                            // socket is closed in handleTransaction
                            break;
                        } else {
                            try {
                                localSocket.close();
                            } catch (IOException ignored) {
                            }
                        }
                    }
                } catch (IOException e) {
                    XposedBridge.log("SysNfcSvcPatch: " + e.getMessage());
                    XposedBridge.log(e);
                    return;
                }
            }
        } catch (RuntimeException e) {
            XposedBridge.log(e);
        }
    }

    static final int TYPE_REQUEST = 1;
    static final int TYPE_RESPONSE = 2;
    static final int TYPE_EVENT = 3;
    static final byte[] EMPTY_BYTE_ARRAY = new byte[0];

    static final int REQUEST_SET_NFC_SOUND_DISABLE = 0x30;
    static final int REQUEST_GET_NFC_SOUND_DISABLE = 0x31;

    static void handleTransaction(LocalSocket socket) {
        try {
            byte[] buffer = new byte[65536];
            InputStream is = socket.getInputStream();
            while (sShouldRunning) {
                // read 4 bytes length
                readFully(is, buffer, 0, 4);
                int length = getInt32Le(buffer, 0);
                Log.d(TAG, "handleTransaction: length = " + length);
                if (length < 16 || length > 65535) {
                    break;
                }
                // read length bytes
                readFully(is, buffer, 0, length);
                // uint32_t type
                int type = getInt32Le(buffer, 0);
                // uint32_t sequence
                int sequence = getInt32Le(buffer, 4);
                // uint32_t requestCode
                int requestCode = getInt32Le(buffer, 8);
                // uint32_t arg0
                int arg0 = getInt32Le(buffer, 12);
                byte[] data;
                if (length > 16) {
                    data = new byte[length - 16];
                    System.arraycopy(buffer, 16, data, 0, length - 16);
                } else {
                    data = EMPTY_BYTE_ARRAY;
                }
                switch (type) {
                    case TYPE_REQUEST:
                        handleRequest(socket, sequence, requestCode, arg0, data);
                        break;
                    case TYPE_RESPONSE:
                    case TYPE_EVENT:
                    default:
                        XposedBridge.log("SysNfcSvcPatch: unknown type " + type);
                        break;
                }
            }
        } catch (IOException e) {
            XposedBridge.log("SysNfcSvcPatch: " + e.getMessage());
        }
        try {
            socket.close();
        } catch (IOException ignored) {
        }
    }

    private static void handleRequest(LocalSocket socket, int sequence, int requestCode, int arg0, byte[] data) {
        switch (requestCode) {
            case REQUEST_SET_NFC_SOUND_DISABLE: {
                sDisableNfcDiscoverySound = (arg0 != 0);
                try {
                    Context ctx = getApplicationContext();
                    setStringConfig(ctx, PREF_DISABLE_NFC_DISCOVERY_SOUND, sDisableNfcDiscoverySound ? "1" : "0");
                } catch (RuntimeException | LinkageError e) {
                    XposedBridge.log(e);
                }
                sendResponse(socket, sequence, 0, 0, null);
                break;
            }
            case REQUEST_GET_NFC_SOUND_DISABLE: {
                sendResponse(socket, sequence, sDisableNfcDiscoverySound ? 1 : 0, 0, null);
                break;
            }
            default: {
                Log.e(TAG, "handleRequest: requestCode = " + requestCode + ", arg0 = " + arg0);
                sendResponse(socket, sequence, -1, -1, null);
            }
        }
    }

    static void sendResponse(LocalSocket socket, int sequence, int resultCode, int errCode, byte[] data) {
        if (data == null) {
            data = EMPTY_BYTE_ARRAY;
        }
        byte[] bufferLen4 = new byte[4];
        putInt32Le(bufferLen4, 0, data.length + 16);
        byte[] buffer = new byte[16 + data.length];
        putInt32Le(buffer, 0, TYPE_RESPONSE);
        putInt32Le(buffer, 4, sequence);
        putInt32Le(buffer, 8, resultCode);
        putInt32Le(buffer, 12, errCode);
        System.arraycopy(data, 0, buffer, 16, data.length);
        try {
            OutputStream os = socket.getOutputStream();
            os.write(bufferLen4);
            os.write(buffer);
            os.flush();
        } catch (IOException e) {
            XposedBridge.log("SysNfcSvcPatch: " + e.getMessage());
        }
    }

    public static synchronized void startMainThread() {
        if (sWorkingThread == null || !sWorkingThread.isAlive()) {
            sWorkingThread = new Thread(SysNfcSvcPatch::run);
            sWorkingThread.start();
        }
    }

    static void readFully(InputStream is, byte[] buffer, int offset, int length) throws IOException {
        int remaining = length;
        while (remaining > 0) {
            int read = is.read(buffer, offset, remaining);
            if (read < 0) {
                throw new IOException("Unexpected EOF");
            }
            remaining -= read;
            offset += read;
        }
    }

    /**
     * Read a little-endian 32-bit integer from the given byte array.
     *
     * @param buffer The byte array to read from.
     * @param offset The offset to start reading from.
     * @return The 32-bit integer read.
     */
    static int getInt32Le(byte[] buffer, int offset) {
        return (buffer[offset] & 0xFF) | ((buffer[offset + 1] & 0xFF) << 8)
                | ((buffer[offset + 2] & 0xFF) << 16) | ((buffer[offset + 3] & 0xFF) << 24);
    }

    /**
     * Put a little-endian 32-bit integer to the given byte array.
     *
     * @param buffer The byte array to write to.
     * @param offset The offset to start writing to.
     * @param value  the value to put
     */
    static void putInt32Le(byte[] buffer, int offset, int value) {
        buffer[offset] = (byte) (value & 0xFF);
        buffer[offset + 1] = (byte) ((value >> 8) & 0xFF);
        buffer[offset + 2] = (byte) ((value >> 16) & 0xFF);
        buffer[offset + 3] = (byte) ((value >> 24) & 0xFF);
    }

    private static void initMisc() {
        Context ctx = getApplicationContext();
        String disableNfcDiscoverySound = getStringConfig(ctx, PREF_DISABLE_NFC_DISCOVERY_SOUND, "0");
        sDisableNfcDiscoverySound = "1".equals(disableNfcDiscoverySound);
    }

    public static Context getApplicationContext() {
        Context ctx = AndroidAppHelper.currentApplication();
        if (ctx == null) {
            IllegalStateException t = new IllegalStateException("SysNfcSvcPatch: getApplicationContext() failed");
            XposedBridge.log(t);
            Log.e(TAG, "SysNfcSvcPatch: getApplicationContext() failed", t);
            throw t;
        }
        return ctx;
    }

    private static String getStringConfig(Context ctx, String key, String def) {
        return ctx.getSharedPreferences("cc.ioctl.nfcdevicehost", Context.MODE_PRIVATE).getString(key, def);
    }

    private static boolean setStringConfig(Context ctx, String key, String value) {
        return ctx.getSharedPreferences("cc.ioctl.nfcdevicehost", Context.MODE_PRIVATE).edit().putString(key, value).commit();
    }
}
