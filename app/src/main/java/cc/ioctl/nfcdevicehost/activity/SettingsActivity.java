package cc.ioctl.nfcdevicehost.activity;

import android.os.Bundle;
import android.util.Log;

import androidx.appcompat.app.ActionBar;
import androidx.preference.PreferenceFragmentCompat;

import cc.ioctl.nfcdevicehost.R;
import cc.ioctl.nfcdevicehost.activity.base.BaseActivity;

public class SettingsActivity extends BaseActivity {

    private static final String TAG = "SettingsActivity";

    @Override
    protected boolean doOnCreate(Bundle savedInstanceState) {
        super.doOnCreate(savedInstanceState);
        Log.d(TAG, "doOnCreate: invoked");
        setContentView(R.layout.settings_activity);
        if (savedInstanceState == null) {
            getSupportFragmentManager()
                    .beginTransaction()
                    .replace(R.id.settings, new SettingsFragment())
                    .commit();
        }
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
        return true;
    }

    public static class SettingsFragment extends PreferenceFragmentCompat {
        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.root_preferences, rootKey);
        }
    }
}
