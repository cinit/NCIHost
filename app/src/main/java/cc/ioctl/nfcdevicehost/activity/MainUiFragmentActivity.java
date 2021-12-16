package cc.ioctl.nfcdevicehost.activity;

import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

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

import java.io.File;
import java.io.IOException;

import cc.ioctl.nfcdevicehost.NativeInterface;
import cc.ioctl.nfcdevicehost.R;
import cc.ioctl.nfcdevicehost.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler;
import cc.ioctl.nfcdevicehost.util.RootShell;
import cc.ioctl.nfcdevicehost.util.ThreadManager;

/**
 * The general purpose fragment host activity
 */
public class MainUiFragmentActivity extends BaseActivity {

    private AppBarConfiguration mAppBarConfiguration;
    private NavController mNavController;

    @Override
    protected boolean doOnCreate(Bundle savedInstanceState) {
        super.doOnCreate(savedInstanceState);
        setContentView(R.layout.activity_main_ui_host);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        NavigationView navigationView = findViewById(R.id.nav_view);
        // Passing each menu ID as a set of Ids because each
        // menu should be considered as top level destinations.
        mAppBarConfiguration = new AppBarConfiguration.Builder(
                R.id.nav_main_home, R.id.nav_main_dump, R.id.nav_main_cards)
                .setDrawerLayout(drawer)
                .build();
        mNavController = Navigation.findNavController(this, R.id.nav_host_fragment);
        NavigationUI.setupActionBarWithNavController(this, mNavController, mAppBarConfiguration);
        NavigationUI.setupWithNavController(navigationView, mNavController);
        ThreadManager.execute(() -> {
            // try to connect to the NCI daemon
            INciHostDaemon daemon = IpcNativeHandler.connect(200);
        });
        return true;
    }

    @UiThread
    public FloatingActionButton showFloatingActionButton() {
        FloatingActionButton fab = findViewById(R.id.fab);
        findViewById(R.id.fab).setVisibility(View.VISIBLE);
        return fab;
    }

    @UiThread
    public void hideFloatingActionButton() {
        findViewById(R.id.fab).setVisibility(View.GONE);
    }

    public NavController getNavController() {
        return mNavController;
    }

    @UiThread
    public void requestRootToStartDaemon() {
        // show a dialog while we are waiting for root
        final AlertDialog requestDialog = new AlertDialog.Builder(this)
                .setTitle(R.string.ui_dialog_title_requesting_root)
                .setMessage(R.string.ui_dialog_msg_body_requesting_root)
                .show();
        // try request root permission
        ThreadManager.execute(() -> {
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
                ThreadManager.post(() -> new AlertDialog.Builder(this).setTitle("Unable to get root")
                        .setMessage("Unable to request root permission, is your device rooted?\n" + errMsg)
                        .setPositiveButton(android.R.string.ok, null).show());
            }
            requestDialog.dismiss();
        });
    }

    /**
     * Attach to the HAL service
     * Do not call this method on the UI thread
     *
     * @return true if the HAL service is attached
     * @throws IOException if error occurs
     */
    public boolean attachToHalService() throws IOException {
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        if (daemon != null) {
            if (!daemon.isHwServiceConnected()) {
                INciHostDaemon.DaemonStatus status = daemon.getDaemonStatus();
                if (status.halServiceArch > 0) {
                    File patchFile = NativeInterface.getNfcHalServicePatchFile(
                            NativeInterface.NfcHalServicePatch.NXP_PATCH,
                            status.halServiceArch);
                    return daemon.initHwServiceConnection(patchFile.getAbsolutePath());
                } else {
                    throw new IOException("Device is not running NFC HAL service, or device is not supported");
                }
            } else {
                return true;
            }
        } else {
            throw new IOException("No daemon connection");
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_activity_main_ui_common, menu);
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
    }
}
