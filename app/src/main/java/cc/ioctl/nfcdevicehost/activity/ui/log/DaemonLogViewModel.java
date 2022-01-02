package cc.ioctl.nfcdevicehost.activity.ui.log;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import java.util.ArrayList;
import java.util.Arrays;

import cc.ioctl.nfcdevicehost.ipc.daemon.INciHostDaemon;
import cc.ioctl.nfcdevicehost.ipc.daemon.IpcNativeHandler;

public class DaemonLogViewModel extends ViewModel {

    private static final String TAG = "DaemonLogViewModel";

    private MutableLiveData<ArrayList<INciHostDaemon.LogEntryRecord>> mLogEntries;
    private ArrayList<INciHostDaemon.LogEntryRecord> mLogEntriesArrayList;
    private int mLastLogEntrySequence = 0;

    public DaemonLogViewModel() {
        super();
        mLogEntriesArrayList = new ArrayList<>();
        mLogEntries = new MutableLiveData<>(mLogEntriesArrayList);
    }

    public void refreshLogs() {
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        if (daemon != null && daemon.isConnected()) {
            long startIndex = mLastLogEntrySequence;
            final int maxCountPerRequest = 100;
            long start = startIndex;
            INciHostDaemon.LogEntryRecord[] historyLogs;
            ArrayList<INciHostDaemon.LogEntryRecord> deltaLogEntries = new ArrayList<>();
            do {
                historyLogs = daemon.getLogsPartial((int) start, maxCountPerRequest);
                if (historyLogs.length > 0) {
                    deltaLogEntries.addAll(Arrays.asList(historyLogs));
                    start = historyLogs[historyLogs.length - 1].id + 1;
                }
                // if we got less than maxCountPerRequest, we have reached the end
            } while (historyLogs.length >= maxCountPerRequest);
            if (deltaLogEntries.size() > 0) {
                mLogEntries.postValue(deltaLogEntries);
            }
        }
    }

    public LiveData<ArrayList<INciHostDaemon.LogEntryRecord>> getLogEntries() {
        return mLogEntries;
    }
}
