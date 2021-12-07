package cc.ioctl.nfcncihost.activity;

import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;
import androidx.appcompat.widget.Toolbar;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.navigation.NavigationView;
import com.google.android.material.snackbar.Snackbar;

import java.io.IOException;

import cc.ioctl.nfcncihost.R;
import cc.ioctl.nfcncihost.daemon.INciHostDaemon;
import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;
import cc.ioctl.nfcncihost.ipc.NfcControllerManager;
import cc.ioctl.nfcncihost.service.NfcCardEmuFgSvc;
import cc.ioctl.nfcncihost.service.NfcControllerManagerService;
import cc.ioctl.nfcncihost.util.RootShell;
import cc.ioctl.nfcncihost.util.ThreadManager;

/**
 * The general purpose fragment host activity
 */
public class MainUiFragmentActivity extends BaseActivity {

    private AppBarConfiguration mAppBarConfiguration;
    volatile NfcControllerManager nfcMgr;
    private Intent mSvcIntent;
    final ServiceConnection mNfcMgrConn = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            try {
                nfcMgr = new NfcControllerManager(service);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            nfcMgr = null;
            if (!isFinishing()) {
                ThreadManager.post(() -> Toast.makeText(MainUiFragmentActivity.this,
                        "onServiceDisconnected", Toast.LENGTH_SHORT).show());
            }
        }
    };

    @Override
    protected boolean doOnCreate(Bundle savedInstanceState) {
        super.doOnCreate(savedInstanceState);
        mSvcIntent = new Intent(this, NfcControllerManagerService.class);
        if (!bindService(mSvcIntent, mNfcMgrConn, BIND_AUTO_CREATE)) {
            mSvcIntent = null;
            new AlertDialog.Builder(this).setTitle("Error").setMessage("bind error")
                    .setCancelable(false).setPositiveButton(android.R.string.ok, (dialog, which) -> {
                        finish();
                    }).show();
        }
        setContentView(R.layout.activity_dashboard);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        FloatingActionButton fab = findViewById(R.id.fab);
        fab.setOnClickListener(view -> Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                .setAction("Action", null).show());
        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        NavigationView navigationView = findViewById(R.id.nav_view);
        // Passing each menu ID as a set of Ids because each
        // menu should be considered as top level destinations.
        mAppBarConfiguration = new AppBarConfiguration.Builder(
                R.id.nav_home, R.id.nav_gallery, R.id.nav_slideshow)
                .setDrawerLayout(drawer)
                .build();
        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment);
        NavigationUI.setupActionBarWithNavController(this, navController, mAppBarConfiguration);
        NavigationUI.setupWithNavController(navigationView, navController);
        ThreadManager.execute(() -> {
            // try to connect to the NCI daemon
            INciHostDaemon daemon = IpcNativeHandler.connect(3000);
            if (daemon == null) {
                ThreadManager.post(this::requestRootToStartDaemon);
            }
        });
        return true;
    }

    @UiThread
    private void requestRootToStartDaemon() {
        // show a dialog while we are waiting for root
        final AlertDialog requestDialog = new AlertDialog.Builder(this)
                .setTitle("Requesting root")
                .setMessage("Please grant root permission")
                .show();
        // try request root permission
        ThreadManager.execute(() -> {
            requestDialog.dismiss();
            String errMsg;
            try {
                INciHostDaemon daemon = IpcNativeHandler.startDaemonWithRootShell();
            } catch (IOException | RuntimeException e) {
                errMsg = e.toString();
                ThreadManager.post(() -> new AlertDialog.Builder(this).setTitle("Error")
                        .setMessage("Error starting root daemon: \n" + errMsg)
                        .setPositiveButton(android.R.string.ok, null).show());
            } catch (RootShell.NoRootShellException e) {
                errMsg = e.getMessage();
                ThreadManager.post(() -> {
                    new AlertDialog.Builder(this).setTitle("Unable to get root")
                            .setMessage("Unable to request root permission, is your device rooted?\n" + errMsg)
                            .setPositiveButton(android.R.string.ok, null).show();
                });
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.dashboard, menu);
        return true;
    }

    @Override
    public boolean onSupportNavigateUp() {
        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment);
        return NavigationUI.navigateUp(navController, mAppBarConfiguration)
                || super.onSupportNavigateUp();
    }


    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_settings: {
                startActivity(new Intent(this, SettingsActivity.class));
                return true;
            }
            case R.id.action_tmp_start: {
                NfcCardEmuFgSvc.requestStartEmulation(this, "0");
                return true;
            }
            case R.id.action_tmp_stop: {
                NfcCardEmuFgSvc.requestStopEmulation(this);
                return true;
            }
            case R.id.action_exit: {
                finish();
                return true;
            }
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void doOnDestroy() {
        super.doOnDestroy();
        if (mSvcIntent != null) {
            unbindService(mNfcMgrConn);
        }
    }

}
