package cc.ioctl.nfcncihost.service;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.Nullable;

import cc.ioctl.nfcncihost.R;
import cc.ioctl.nfcncihost.activity.DashboardActivity;
import cc.ioctl.nfcncihost.util.ThreadManager;

public class NfcCardEmuFgSvc extends Service {

    private static final String TAG = "NfcCardEmuFgSvc";
    private static final String CHN_NAME_NFC_EMU_ID = "CHN_NAME_NFC_EMU_ID";
    private static final String ACTION_STOP_CARD_EMU = "cc.ioctl.nfcncihost.ACTION_STOP_CARD_EMU";
    private static final String ACTION_START_CARD_EMU = "cc.ioctl.nfcncihost.ACTION_START_CARD_EMU";
    private static final String ARGV_CARD_ID = "CARD_ID";


    @Override
    public void onCreate() {
        Log.i(TAG, "onCreate");
        super.onCreate();
    }

    private void stopCardEmu() {
        stopForeground(true);
        stopSelf();
    }

    private void updateEmuationFgState(String cardName) {
        PendingIntent configIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, DashboardActivity.class), 0);
        PendingIntent stopIntent = PendingIntent.getService(this, 0,
                new Intent(this, NfcCardEmuFgSvc.class)
                        .setAction(NfcCardEmuFgSvc.ACTION_STOP_CARD_EMU), 0);
        Notification.Builder builder;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            nm.createNotificationChannel(new NotificationChannel(CHN_NAME_NFC_EMU_ID,
                    getString(R.string.ntf_chn_name_card_emu), NotificationManager.IMPORTANCE_LOW));
            builder = new Notification.Builder(this, CHN_NAME_NFC_EMU_ID);
        } else {
            builder = new Notification.Builder(this);
        }
        Notification notification = builder
                .setContentTitle(getString(R.string.ntf_title_nfc_emu))
                .setContentText(cardName)
                .setOngoing(true)
                .setShowWhen(false)
                .addAction(R.drawable.ic_baseline_stop_32, getString(R.string.action_stop), stopIntent)
                .setSmallIcon(R.drawable.ic_baseline_nfc_24)
                .setContentIntent(configIntent)
                .setTicker(getString(R.string.ntf_ticker_emu_card_name, cardName))
                .build();
        startForeground(R.id.notification_emulate_card, notification);
    }

    private void updateRequestPermFgState() {
        PendingIntent configIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, DashboardActivity.class), 0);
        Notification.Builder builder;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            nm.createNotificationChannel(new NotificationChannel(CHN_NAME_NFC_EMU_ID,
                    getString(R.string.ntf_chn_name_card_emu), NotificationManager.IMPORTANCE_LOW));
            builder = new Notification.Builder(this, CHN_NAME_NFC_EMU_ID);
        } else {
            builder = new Notification.Builder(this);
        }
        Notification notification = builder
                .setContentTitle(getString(R.string.ntf_title_nfc_emu))
                .setContentText(getString(R.string.ntf_text_req_perm))
                .setOngoing(true)
                .setShowWhen(false)
                .setSmallIcon(R.drawable.ic_baseline_nfc_24)
                .setContentIntent(configIntent)
                .setTicker(getString(R.string.ntf_text_req_perm))
                .build();
        startForeground(R.id.notification_emulate_card, notification);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String action = intent.getAction();
        if (action != null) {
            switch (action) {
                case ACTION_START_CARD_EMU: {
                    String cardId = intent.getStringExtra(ARGV_CARD_ID);
                    if (!TextUtils.isEmpty(cardId)) {
                        updateRequestPermFgState();
                        ThreadManager.postDelayed(2000, () -> updateEmuationFgState("Test"));
                    }
                    break;
                }
                case ACTION_STOP_CARD_EMU: {
                    stopCardEmu();
                    break;
                }
                default:
                    break;
            }
        }
        return Service.START_NOT_STICKY;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy");
        super.onDestroy();
    }

    public static void requestStartEmulation(Context ctx, String id) {
        ctx.startService(new Intent(ctx, NfcCardEmuFgSvc.class)
                .setAction(NfcCardEmuFgSvc.ACTION_START_CARD_EMU)
                .putExtra(ARGV_CARD_ID, id));
    }

    public static void requestStopEmulation(Context ctx) {
        ctx.startService(new Intent(ctx, NfcCardEmuFgSvc.class)
                .setAction(NfcCardEmuFgSvc.ACTION_STOP_CARD_EMU));
    }
}
