package cc.ioctl.nfcdevicehost.activity.ui.home

import android.graphics.Typeface
import android.os.Bundle
import android.view.*
import android.widget.TextView
import androidx.annotation.UiThread
import androidx.appcompat.app.AlertDialog
import androidx.core.content.res.ResourcesCompat
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import cc.ioctl.nfcdevicehost.R
import cc.ioctl.nfcdevicehost.daemon.IpcNativeHandler
import cc.ioctl.nfcdevicehost.databinding.FragmentMainHomeBinding
import cc.ioctl.nfcdevicehost.service.NfcCardEmuFgSvc
import cc.ioctl.nfcdevicehost.util.NativeUtils
import cc.ioctl.nfcdevicehost.util.UiUtils
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
            mBinding.homeDetailInfoDetailMsg.text = ""
            mBinding.homeOperationsHints.text = getString(R.string.ui_home_need_start_daemon)
            mBinding.homeOperationsHintDetails.text = getString(R.string.ui_home_root_is_required)
            mBinding.homeLinearLayoutOperationA.visibility = View.VISIBLE
            mBinding.homeLinearLayoutOperationB.visibility = View.GONE
            mBinding.homeLinearLayoutOperationC.visibility = View.GONE
        } else {
            // daemon is running
            if (status.isHalServiceAttached) {
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
                mBinding.homeDetailInfoDesc.text = "HAL Service: ${status.halServicePid}" +
                        " | Daemon: ${status.processId}"
                mBinding.homeDetailInfoDetailMsg.text =
                    status.halServiceExePath?.substringAfterLast("/") ?: "<unknown>"
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
                if (status.halServicePid > 0) {
                    // found HAL service
                    mBinding.homeDetailInfoDesc.text = String.format(
                        Locale.ROOT,
                        "HAL service PID: %d, %s",
                        status.halServicePid,
                        NativeUtils.archIntToAndroidLibArch(status.halServiceArch)
                    )
                    mBinding.homeDetailInfoDetailMsg.text =
                        status.halServiceExePath.toString().substringAfterLast("/")
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
