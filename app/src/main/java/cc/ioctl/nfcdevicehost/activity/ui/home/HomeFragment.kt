package cc.ioctl.nfcdevicehost.activity.ui.home

import android.content.Intent
import android.graphics.Typeface
import android.net.Uri
import android.os.Bundle
import android.view.*
import android.widget.TextView
import androidx.annotation.UiThread
import androidx.appcompat.app.AlertDialog
import androidx.core.content.res.ResourcesCompat
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import cc.ioctl.nfcdevicehost.R
import cc.ioctl.nfcdevicehost.activity.MainUiFragmentActivity
import cc.ioctl.nfcdevicehost.ipc.daemon.IpcNativeHandler
import cc.ioctl.nfcdevicehost.databinding.FragmentMainHomeBinding
import cc.ioctl.nfcdevicehost.service.NfcCardEmuFgSvc
import cc.ioctl.nfcdevicehost.util.NativeUtils
import cc.ioctl.nfcdevicehost.util.ThreadManager.async
import cc.ioctl.nfcdevicehost.util.UiUtils
import com.google.android.material.snackbar.Snackbar
import java.util.*

class HomeFragment : Fragment() {
    private lateinit var mViewModel: HomeViewModel
    private lateinit var mBinding: FragmentMainHomeBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setHasOptionsMenu(true)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?, savedInstanceState: Bundle?
    ): View {
        mViewModel = ViewModelProvider(this).get(HomeViewModel::class.java)
        mBinding = FragmentMainHomeBinding.inflate(inflater, container, false)
        mViewModel.daemonStatus.observe(viewLifecycleOwner) { updateUiState() }
        // UI state will be updated on resume
        mBinding.homeDetailInfoLinearLayout.setOnClickListener { showDetailInfoDialog() }
        mBinding.homeLinearLayoutOperationA.setOnClickListener {
            (requireActivity() as MainUiFragmentActivity).requestRootToStartDaemon()
        }
        mBinding.homeLinearLayoutOperationB.setOnClickListener {
            val act = requireActivity() as MainUiFragmentActivity
            val dialog = AlertDialog.Builder(act)
                .setTitle(R.string.ui_home_dialog_title_attach_hal_service)
                .setMessage(R.string.ui_home_dialog_msg_attach_hal_service)
                .setCancelable(false)
                .show()
            async {
                var result = false
                var exception: java.lang.Exception? = null
                try {
                    result = act.attachToHalService()
                } catch (e: Exception) {
                    exception = e
                }
                dialog.dismiss()
                if (result) {
                    act.runOnUiThread {
                        Snackbar.make(
                            mBinding.homeLinearLayoutOperationB,
                            R.string.ui_home_toast_attach_hal_service_success,
                            Snackbar.LENGTH_SHORT
                        ).show()
                    }
                    mViewModel.refreshDaemonStatus()
                } else {
                    act.runOnUiThread {
                        AlertDialog.Builder(act)
                            .setTitle(R.string.ui_home_dialog_title_attach_hal_service)
                            .setMessage(
                                getString(R.string.ui_home_dialog_msg_attach_hal_service_fail)
                                        + "\n" + exception?.message
                            )
                            .setCancelable(true)
                            .setPositiveButton(android.R.string.ok, null)
                            .show()
                    }
                }
            }
        }
        mBinding.homeLinearLayoutOperationC.setOnClickListener {
            Snackbar.make(
                mBinding.homeLinearLayoutOperationC,
                R.string.ui_toast_not_implemented,
                Snackbar.LENGTH_SHORT
            ).show()
        }
        mBinding.homeLayoutHelp.setOnClickListener {
            Snackbar.make(
                mBinding.homeLinearLayoutOperationC,
                R.string.ui_toast_not_implemented,
                Snackbar.LENGTH_SHORT
            ).show()
        }
        mBinding.homeLayoutGithubRepo.setOnClickListener { jumpToGitHubRepo() }
        mViewModel.refreshDaemonStatus()
        return mBinding.root
    }

    @UiThread
    private fun updateUiState() {
        val status = mViewModel.daemonStatus.value
        if (status == null) {
            // daemon is not running
            mBinding.homeMainStatusTitle.text = getString(R.string.status_general_not_running)
            mBinding.homeMainStatusDesc.text = getString(R.string.status_daemon_not_running)
            mBinding.homeMainStatusIcon.setImageDrawable(
                ResourcesCompat.getDrawable(
                    resources,
                    R.drawable.ic_failure_white,
                    null
                )
            )
            mBinding.homeMainStatusLinearLayout.background = ResourcesCompat.getDrawable(
                resources,
                R.drawable.bg_red_solid,
                null
            )
            val kernelAbi = IpcNativeHandler.getKernelArchitecture();
            mBinding.homeDetailInfoDesc.text = getString(R.string.ui_device_arch_v0s, kernelAbi)
            mBinding.homeDetailInfoDetailMsg.text = getString(R.string.ui_home_hal_service_unknown)
            mBinding.homeOperationsHints.text = getString(R.string.ui_home_need_start_daemon)
            mBinding.homeOperationsHintDetails.text = getString(R.string.ui_home_root_is_required)
            mBinding.homeLinearLayoutOperationA.visibility = View.VISIBLE
            mBinding.homeLinearLayoutOperationB.visibility = View.GONE
            mBinding.homeLinearLayoutOperationC.visibility = View.GONE
        } else {
            // daemon is running
            if (status.nfcHalServiceStatus.isHalServiceAttached
                && status.esePmServiceStatus.isHalServiceAttached
            ) {
                // HAL attached
                mBinding.homeMainStatusTitle.text = getString(R.string.status_general_running_ok)
                mBinding.homeMainStatusDesc.text = getString(R.string.status_hal_service_attached)
                mBinding.homeMainStatusIcon.setImageDrawable(
                    ResourcesCompat.getDrawable(
                        resources,
                        R.drawable.ic_success_white,
                        null
                    )
                )
                mBinding.homeMainStatusLinearLayout.background = ResourcesCompat.getDrawable(
                    resources,
                    R.drawable.bg_green_solid,
                    null
                )
                mBinding.homeDetailInfoDesc.text =
                    "NFC HAL: ${status.nfcHalServiceStatus?.halServicePid}" +
                            " | eSE PM: ${status.esePmServiceStatus?.halServicePid}" +
                            " | Daemon: ${status.processId}"
                mBinding.homeDetailInfoDetailMsg.text =
                    (status.nfcHalServiceStatus?.halServiceExePath?.substringAfterLast("/")
                        ?: "<unknown>") +
                            "\n" +
                            (status.esePmServiceStatus?.halServiceExePath?.substringAfterLast("/")
                                ?: "<unknown>")
                mBinding.homeOperationsHints.text = getString(R.string.ui_home_no_operation_needed)
                mBinding.homeOperationsHintDetails.text =
                    getString(R.string.ui_home_restart_hal_service_if_needed)
                mBinding.homeLinearLayoutOperationA.visibility = View.GONE
                mBinding.homeLinearLayoutOperationB.visibility = View.GONE
                mBinding.homeLinearLayoutOperationC.visibility = View.VISIBLE
            } else {
                // HAL not attached
                mBinding.homeMainStatusTitle.text = getString(R.string.status_general_not_attached)
                mBinding.homeMainStatusDesc.text =
                    getString(R.string.status_hal_service_not_attached)
                mBinding.homeMainStatusIcon.setImageDrawable(
                    ResourcesCompat.getDrawable(
                        resources,
                        R.drawable.ic_warning_white,
                        null
                    )
                )
                mBinding.homeMainStatusLinearLayout.background = ResourcesCompat.getDrawable(
                    resources,
                    R.drawable.bg_yellow_solid,
                    null
                )
                if (status.nfcHalServiceStatus.halServicePid > 0 || status.esePmServiceStatus.halServicePid > 0) {
                    // found HAL service
                    mBinding.homeDetailInfoDesc.text = String.format(
                        Locale.ROOT,
                        "NFC HAL: %d, %s | eSE PM: %d, %s",
                        status.nfcHalServiceStatus?.halServicePid,
                        if ((status.nfcHalServiceStatus?.halServicePid ?: -1) > 0)
                            status.nfcHalServiceStatus?.halServiceArch?.let {
                                NativeUtils.archIntToAndroidLibArch(it)
                            } else "N/A",
                        status.esePmServiceStatus?.halServicePid,
                        if ((status.esePmServiceStatus?.halServicePid ?: -1) > 0)
                            status.esePmServiceStatus?.halServiceArch?.let {
                                NativeUtils.archIntToAndroidLibArch(it)
                            } else "N/A"
                    )
                    mBinding.homeDetailInfoDetailMsg.text =
                        status.nfcHalServiceStatus?.halServiceExePath.toString()
                            .substringAfterLast("/") +
                                "\n" +
                                status.esePmServiceStatus?.halServiceExePath.toString()
                                    .substringAfterLast("/")
                } else {
                    // unable to find HAL service
                    mBinding.homeDetailInfoDesc.text =
                        getString(R.string.ui_home_hal_service_not_found)
                    mBinding.homeDetailInfoDetailMsg.text =
                        getString(R.string.ui_home_maybe_service_not_running)
                }
                mBinding.homeOperationsHints.text =
                    getString(R.string.ui_click_to_attach_hal_service)
                mBinding.homeOperationsHintDetails.text =
                    getString(R.string.ui_home_attach_hal_service_to_work)
                mBinding.homeLinearLayoutOperationA.visibility = View.GONE
                mBinding.homeLinearLayoutOperationB.visibility = View.VISIBLE
                mBinding.homeLinearLayoutOperationC.visibility = View.GONE
            }
        }
    }

    override fun onResume() {
        super.onResume()
        updateUiState()
        if (activity is MainUiFragmentActivity) {
            (activity as MainUiFragmentActivity).hideFloatingActionButton();
        }
    }

    override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater) {
        inflater.inflate(R.menu.menu_fragment_home, menu)
    }

    private fun showDetailInfoDialog() {
        AlertDialog.Builder(requireContext())
            .setTitle(R.string.status)
            .setView(TextView(requireContext()).apply {
                text = mViewModel.daemonStatus.value.toString()
                typeface = Typeface.MONOSPACE
                val pxValue = UiUtils.dip2px(requireContext(), 10f)
                setPadding(pxValue, pxValue, pxValue, pxValue)
                setTextIsSelectable(true)
            })
            .setPositiveButton(android.R.string.ok) { dialog, _ -> dialog.dismiss() }
            .setCancelable(true)
            .show()
    }

    @UiThread
    private fun jumpToGitHubRepo() {
        val intent = Intent(Intent.ACTION_VIEW)
        intent.data = Uri.parse("https://github.com/cinit/NCIHost")
        startActivity(intent)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_refresh -> {
                mViewModel.refreshDaemonStatus()
                true
            }
            R.id.action_tmp_start -> {
                NfcCardEmuFgSvc.requestStartEmulation(requireActivity(), "0")
                true
            }
            R.id.action_tmp_stop -> {
                NfcCardEmuFgSvc.requestStopEmulation(requireActivity())
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }
}
