package cc.ioctl.nfcdevicehost.activity.base;

import android.os.Bundle;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;
import androidx.preference.EditTextPreference;
import androidx.preference.ListPreference;
import androidx.preference.MultiSelectListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceGroup;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;

import com.google.android.material.snackbar.Snackbar;

import cc.ioctl.nfcdevicehost.R;
import cc.ioctl.nfcdevicehost.util.config.IConfigItem;
import cc.ioctl.nfcdevicehost.util.config.PrefLutUtils;

/**
 * Base class for all preference fragments.
 * Preference key name should be like "#default_config/generic:key_name$sub_key_name"
 * Each key should be unique and start with "#"
 * # [default_config/] generic:key_name [$sub_key_name]
 * 1.mmkv file name  2.group  3.key_name 4.sub_key_name(optional)
 */
public abstract class BasePreferenceFragment extends PreferenceFragmentCompat implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "BasePreferenceFragment";

    protected FragmentActivity mActivity;

    @Deprecated
    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        doOnCreatePreferences(savedInstanceState, rootKey);
        mActivity = getActivity();
        initAllPreferences();
    }

    protected abstract void doOnCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey);

    private void initPreference(Preference preference) {
        preference.setIconSpaceReserved(false);
        if (preference.getKey() != null) {
            preference.setPersistent(false);
            // find the config item
            String prefKey = preference.getKey();
            if (prefKey == null) {
                return;
            }
            if (prefKey.startsWith("#")) {
                String fullName = prefKey.substring(1).replace("!", "");
                IConfigItem<?> item = PrefLutUtils.findConfigByFullName(fullName);
                bindPreferenceToItem(preference, item, prefKey);
                preference.setOnPreferenceChangeListener(this);
            }
        }
        if (preference instanceof PreferenceGroup) {
            PreferenceGroup preferenceGroup = ((PreferenceGroup) preference);
            for (int i = 0; i < preferenceGroup.getPreferenceCount(); i++) {
                initPreference(preferenceGroup.getPreference(i));
            }
        }
    }

    private void bindPreferenceToItem(@NonNull Preference preference, @Nullable IConfigItem rawItem,
                                      String keyName) {
        if ((rawItem == null || !rawItem.isValid())
                && (preference instanceof TwoStatePreference
                || preference instanceof ListPreference
                || preference instanceof MultiSelectListPreference
                || preference instanceof EditTextPreference)) {
            preference.setEnabled(false);
            if (preference.getSummaryProvider() != null) {
                // remove summary provider
                preference.setSummaryProvider(null);
            }
            preference.setSummary(R.string.ui_summary_not_implemented);
        } else if (rawItem != null) {
            try {
                if (preference instanceof TwoStatePreference) {
                    TwoStatePreference pref = (TwoStatePreference) preference;
                    pref.setChecked((Boolean) (rawItem.getValue()));
                } else if (preference instanceof ListPreference) {
                    ListPreference pref = (ListPreference) preference;
                    pref.setValue(rawItem.getValue().toString());
                } else if (preference instanceof EditTextPreference) {
                    EditTextPreference pref = (EditTextPreference) preference;
                    String value = (String) rawItem.getValue();
                    pref.setText(value == null ? "" : value);
                }
            } catch (Exception e) {
                Log.e(TAG, "bindPreferenceValue: ", e);
                preference.setSummary((e + "").replaceAll("java\\.[a-z]+\\.", ""));
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        try {
            String prefKey = preference.getKey();
            if (prefKey == null) {
                return false;
            }
            if (prefKey.startsWith("#")) {
                String fullName = prefKey.substring(1).replace("!", "");
                IConfigItem<?> item = PrefLutUtils.findConfigByFullName(fullName);
                if (item != null) {
                    if (preference instanceof TwoStatePreference) {
                        IConfigItem<Boolean> it = (IConfigItem<Boolean>) item;
                        it.setValue((Boolean) newValue);
                    } else if (preference instanceof ListPreference) {
                        IConfigItem<String> it = (IConfigItem<String>) item;
                        it.setValue((String) newValue);
                    } else if (preference instanceof EditTextPreference) {
                        IConfigItem<String> it = (IConfigItem<String>) item;
                        it.setValue(newValue.toString());
                    }
                }
            }
            return true;
        } catch (Exception e) {
            Log.e(TAG, "onPreferenceChange: save error", e);
            Snackbar.make(mActivity.getWindow().getDecorView(), getString(R.string.ui_toast_save_error_v0s,
                    e.getMessage()), Snackbar.LENGTH_LONG).show();
            return false;
        }
    }

    protected void initAllPreferences() {
        PreferenceScreen preferenceScreen = getPreferenceScreen();
        for (int i = 0; i < preferenceScreen.getPreferenceCount(); i++) {
            Preference preference = preferenceScreen.getPreference(i);
            initPreference(preference);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        initAllPreferences();
    }
}
