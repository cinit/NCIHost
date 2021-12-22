package cc.ioctl.nfcdevicehost.activity.ui.settings

import android.app.Activity
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.Preference
import cc.ioctl.nfcdevicehost.R
import cc.ioctl.nfcdevicehost.activity.MainUiFragmentActivity
import cc.ioctl.nfcdevicehost.activity.base.BasePreferenceFragment
import cc.ioctl.nfcdevicehost.activity.ui.misc.LicenseNoticeDialogFragment
import cc.ioctl.nfcdevicehost.ipc.daemon.IpcNativeHandler
import cc.ioctl.nfcdevicehost.util.ThreadManager.async
import cc.ioctl.nfcdevicehost.util.ThreadManager.runOnUiThread
import cc.ioctl.nfcdevicehost.util.config.GenericConfig
import com.google.android.material.snackbar.Snackbar

class SettingsFragment : BasePreferenceFragment() {
    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val activity: Activity? = activity
        if (activity is AppCompatActivity) {
            val actionBar = activity.supportActionBar
            actionBar?.setDisplayHomeAsUpEnabled(true)
        }
        return super.onCreateView(inflater, container, savedInstanceState)
    }

    override fun onResume() {
        super.onResume()
        val activity: Activity? = activity
        if (activity is MainUiFragmentActivity) {
            activity.hideFloatingActionButton()
        }
    }

    public override fun doOnCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.root_preferences, rootKey)
        preferenceScreen.findPreference<Preference>("pref_open_source_licenses")!!
            .setOnPreferenceClickListener {
                val dialog = LicenseNoticeDialogFragment()
                dialog.show(requireActivity().supportFragmentManager, tag)
                true
            }
        GenericConfig.cfgDisableNfcDiscoverySound.observe(this) { value: Boolean ->
            val view = requireActivity().window.decorView
            val daemon = IpcNativeHandler.peekConnection()
            if (daemon == null) {
                Snackbar.make(view, R.string.ui_toast_daemon_is_not_running, Snackbar.LENGTH_SHORT)
                    .show()
            } else {
                val r = Runnable {
                    if (!daemon.isAndroidNfcServiceConnected) {
                        Snackbar.make(
                            view, R.string.ui_toast_android_nfc_service_not_connected,
                            Snackbar.LENGTH_SHORT
                        ).show()
                    } else {
                        async {
                            val result = daemon.setNfcDiscoverySoundDisabled(value)
                            if (!result) {
                                runOnUiThread {
                                    Snackbar.make(
                                        view,
                                        R.string.ui_toast_operation_failed, Snackbar.LENGTH_SHORT
                                    ).show()
                                }
                            }
                        }
                    }
                }
                if (!daemon.isAndroidNfcServiceConnected) {
                    async {
                        daemon.connectToAndroidNfcService()
                        runOnUiThread(r)
                    }
                } else {
                    r.run()
                }
            }
        }
    }
}
