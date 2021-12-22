package cc.ioctl.nfcdevicehost.activity.ui.misc

import android.annotation.SuppressLint
import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import cc.ioctl.nfcdevicehost.activity.base.BaseBottomSheetDialogFragment
import cc.ioctl.nfcdevicehost.databinding.DialogLicenseNoticeBinding
import cc.ioctl.nfcdevicehost.databinding.ItemLicenseNoticeBinding
import de.psdev.licensesdialog.LicensesDialog
import de.psdev.licensesdialog.licenses.ApacheSoftwareLicense20
import de.psdev.licensesdialog.licenses.BSD3ClauseLicense
import de.psdev.licensesdialog.licenses.MITLicense
import de.psdev.licensesdialog.model.Notice
import de.psdev.licensesdialog.model.Notices
import io.noties.markwon.Markwon

class LicenseNoticeDialogFragment : BaseBottomSheetDialogFragment() {

    private var binding: DialogLicenseNoticeBinding? = null

    override fun createRootView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        binding = DialogLicenseNoticeBinding.inflate(inflater, container, false)
        binding!!.dialogButtonClose.setOnClickListener { dismiss() }
        val recycleView = binding!!.dialogRecyclerViewNotices
        recycleView.adapter = LicenseNoticesAdapter(requireContext())
        return binding!!.root
    }

    override fun onDestroyView() {
        super.onDestroyView()
        binding = null
    }

    class LicenseNoticeViewHolder(val binding: ItemLicenseNoticeBinding) :
        RecyclerView.ViewHolder(binding.root) {
    }

    class LicenseNoticesAdapter(val context: Context) :
        RecyclerView.Adapter<LicenseNoticeViewHolder>() {
        private val markwon: Markwon = Markwon.create(context)
        private val notices: Notices = Notices().also {
            it.addNotice(
                Notice(
                    "nfcandroid_nfc_hidlimpl",
                    "https://github.com/NXPNFCProject/nfcandroid_nfc_hidlimpl",
                    "Copyright 2020-2021 NXP",
                    ApacheSoftwareLicense20()
                )
            )
            it.addNotice(
                Notice(
                    "MMKV", "https://github.com/Tencent/MMKV",
                    "Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.",
                    BSD3ClauseLicense()
                )
            )
            it.addNotice(
                Notice(
                    "xHook", "https://github.com/iqiyi/xHook",
                    "Copyright (c) 2018-present, iQIYI, Inc. All rights reserved.\n\n" +
                            "Most source code in xhook are MIT licensed. " +
                            "Some other source code have BSD-style licenses.",
                    MITLicense()
                )
            )
            it.addNotice(
                Notice(
                    "libsu",
                    "https://github.com/topjohnwu/libsu",
                    "Copyright 2021 John \"topjohnwu\" Wu",
                    ApacheSoftwareLicense20()
                )
            )
            it.addNotice(
                Notice(
                    "AndroidHiddenApiBypass",
                    "https://github.com/LSPosed/AndroidHiddenApiBypass",
                    "Copyright (C) 2021 LSPosed",
                    ApacheSoftwareLicense20()
                )
            )
            it.addNotice(
                Notice(
                    "Xposed",
                    "https://github.com/rovo89/XposedBridge",
                    "Copyright 2013 rovo89, Tungstwenty",
                    ApacheSoftwareLicense20()
                )
            )
            it.addNotice(
                Notice(
                    "Markwon",
                    "https://github.com/noties/Markwon",
                    "Copyright 2017 Dimitry Ivanov (mail@dimitryivanov.ru)",
                    ApacheSoftwareLicense20()
                )
            )
            it.addNotice(LicensesDialog.LICENSES_DIALOG_NOTICE)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) =
            LicenseNoticeViewHolder(
                ItemLicenseNoticeBinding.inflate(
                    LayoutInflater.from(parent.context),
                    parent,
                    false
                )
            )

        @SuppressLint("SetTextI18n")
        override fun onBindViewHolder(holder: LicenseNoticeViewHolder, position: Int) {
            val notice = notices.notices[position]
            val title: TextView = holder.binding.sLicenseItemTitle
            val licenseView: TextView = holder.binding.sLicenseItemLicensePrev
            markwon.setMarkdown(title, "- " + notice.name + "  \n<" + notice.url + ">")
            licenseView.text = notice.copyright + "\n\n" + notice.license.getSummaryText(context)
        }

        override fun getItemCount() = notices.notices.size
    }
}
