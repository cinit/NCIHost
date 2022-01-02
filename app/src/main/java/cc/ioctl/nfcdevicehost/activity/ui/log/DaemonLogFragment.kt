package cc.ioctl.nfcdevicehost.activity.ui.log

import android.os.Bundle
import android.util.Log
import android.view.*
import androidx.core.content.res.ResourcesCompat
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.DefaultItemAnimator
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import cc.ioctl.nfcdevicehost.R
import cc.ioctl.nfcdevicehost.activity.MainUiFragmentActivity
import cc.ioctl.nfcdevicehost.databinding.FragmentLogBinding
import cc.ioctl.nfcdevicehost.databinding.ItemLogEntryBinding
import cc.ioctl.nfcdevicehost.ipc.daemon.IpcNativeHandler
import com.google.android.material.snackbar.Snackbar
import java.text.SimpleDateFormat
import java.util.*

class DaemonLogFragment : Fragment() {

    private var mModel: DaemonLogViewModel? = null
    private var mBinding: FragmentLogBinding? = null
    private val mAdapter = LogEntryAdapter(this)

    class LogEntryViewHolder(val binding: ItemLogEntryBinding) :
        RecyclerView.ViewHolder(binding.root)

    class LogEntryAdapter(val that: DaemonLogFragment) :
        RecyclerView.Adapter<LogEntryViewHolder>() {
        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): LogEntryViewHolder {
            val binding = ItemLogEntryBinding
                .inflate(LayoutInflater.from(parent.context), parent, false)
            return LogEntryViewHolder(binding)
        }

        override fun onBindViewHolder(holder: LogEntryViewHolder, position: Int) {
            val log = that.mModel!!.logEntries.value!![position]
            val tvPriority = holder.binding.itemLogEntryPriority
            val tvTag = holder.binding.itemLogEntryTag
            val tvMessage = holder.binding.itemLogEntryMessage
            val tvTime = holder.binding.itemLogEntryTime
            tvTag.text = log.tag
            tvMessage.text = log.message
            val timestamp: Long = log.timestamp
            // yyyy-MM-dd HH:mm:ss.SSS
            tvTime.text = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.ROOT).format(timestamp)
            when (log.level) {
                Log.VERBOSE -> {
                    tvPriority.setBackgroundColor(
                        ResourcesCompat.getColor(
                            that.resources,
                            R.color.log_level_verbose,
                            that.context?.theme
                        )
                    )
                    tvPriority.text = "V"
                }
                Log.DEBUG -> {
                    tvPriority.setBackgroundColor(
                        ResourcesCompat.getColor(
                            that.resources,
                            R.color.log_level_debug,
                            that.context?.theme
                        )
                    )
                    tvPriority.text = "D"
                }
                Log.INFO -> {
                    tvPriority.setBackgroundColor(
                        ResourcesCompat.getColor(
                            that.resources,
                            R.color.log_level_info,
                            that.context?.theme
                        )
                    )
                    tvPriority.text = "I"
                }
                Log.WARN -> {
                    tvPriority.setBackgroundColor(
                        ResourcesCompat.getColor(
                            that.resources,
                            R.color.log_level_warning,
                            that.context?.theme
                        )
                    )
                    tvPriority.text = "W"
                }
                Log.ERROR -> {
                    tvPriority.setBackgroundColor(
                        ResourcesCompat.getColor(
                            that.resources,
                            R.color.log_level_error,
                            that.context?.theme
                        )
                    )
                    tvPriority.text = "E"
                }
                Log.ASSERT -> {
                    tvPriority.setBackgroundColor(
                        ResourcesCompat.getColor(
                            that.resources,
                            R.color.log_level_assert,
                            that.context?.theme
                        )
                    )
                    tvPriority.text = "A"
                }
                else -> {
                    tvPriority.setBackgroundColor(
                        ResourcesCompat.getColor(
                            that.resources,
                            R.color.log_level_silent,
                            that.context?.theme
                        )
                    )
                    tvPriority.text = "?"
                }
            }
        }

        override fun getItemCount(): Int {
            return that.mModel?.logEntries?.value?.size ?: 0
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setHasOptionsMenu(true)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val context = inflater.context
        mModel = ViewModelProvider(this).get(DaemonLogViewModel::class.java)
        mBinding = FragmentLogBinding.inflate(inflater, container, false)
        mModel!!.logEntries.observe(viewLifecycleOwner) {
            mAdapter.notifyDataSetChanged()
        }
        mBinding!!.recyclerViewLogFragmentLogList.apply {
            layoutManager = LinearLayoutManager(context).apply {
                orientation = RecyclerView.VERTICAL
            }
            addItemDecoration(
                DividerItemDecoration(
                    context,
                    DividerItemDecoration.VERTICAL
                )
            )
            itemAnimator = DefaultItemAnimator()
        }
        mBinding!!.recyclerViewLogFragmentLogList.adapter = mAdapter
        return mBinding!!.root
    }

    override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater) {
        inflater.inflate(R.menu.menu_fragment_log, menu)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_refresh -> {
                mModel!!.refreshLogs()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    override fun onResume() {
        super.onResume()
        if (activity is MainUiFragmentActivity) {
            (activity as MainUiFragmentActivity).hideFloatingActionButton();
        }
        val daemon = IpcNativeHandler.peekConnection()
        if (daemon == null) {
            Snackbar.make(
                mBinding!!.root,
                R.string.ui_toast_daemon_is_not_running,
                Snackbar.LENGTH_LONG
            ).setAction(R.string.ui_snackbar_action_view_or_jump) {
                (requireActivity() as MainUiFragmentActivity).navController.navigate(R.id.nav_main_home)
            }.show()
        } else {
            mModel!!.refreshLogs()
        }
    }

}
