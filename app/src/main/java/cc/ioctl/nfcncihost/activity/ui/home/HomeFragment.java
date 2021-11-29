package cc.ioctl.nfcncihost.activity.ui.home;

import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import java.io.File;
import java.io.IOException;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import cc.ioctl.nfcncihost.NativeInterface;
import cc.ioctl.nfcncihost.daemon.INciHostDaemon;
import cc.ioctl.nfcncihost.daemon.IpcNativeHandler;
import cc.ioctl.nfcncihost.daemon.internal.NciHostDaemonProxy;
import cc.ioctl.nfcncihost.util.ByteUtils;

public class HomeFragment extends Fragment {

    //    private HomeViewModel homeViewModel;
    static ExecutorService tp = Executors.newCachedThreadPool();

    TextView textView;
    HashSet<Integer> plainTextFds = new HashSet<>();
    HashMap<Integer, String> fileNames = new HashMap<>();

    private void appendIoEvent(NciHostDaemonProxy.IoEventPacket event) {
        getActivity().runOnUiThread(() -> {
            // date format HH:mm:ss.SSS, GTM+8
            Date date = new Date(event.timestamp);
            String time = String.format("%tT.%tL", date, date);
            String operationName = ("" + event.opType).toLowerCase(Locale.ROOT);
            String shortFileName = fileNames.get(event.fd);
            switch (event.opType) {
                case OPEN: {
                    String fileName = new String(event.buffer, 0, event.buffer.length);
                    if (!fileName.startsWith("/dev/")) {
                        plainTextFds.add(event.fd);
                    }
                    String shortFileName2 = fileName.split("/")[fileName.split("/").length - 1];
                    fileNames.put(event.fd, shortFileName2);
                    textView.append("\n" + time + ": " + operationName
                            + "(" + (shortFileName == null ? event.fd : shortFileName) + "): " + fileName);
                    break;
                }
                case CLOSE: {
                    plainTextFds.remove(event.fd);
                    textView.append("\n" + time + ": " + operationName + "("
                            + event.fd + ")" + (shortFileName == null ? "" : ": " + shortFileName));
                    break;
                }
                case READ:
                case WRITE: {
                    if (plainTextFds.contains(event.fd)) {
                        String data = new String(event.buffer, 0, event.buffer.length);
                        textView.append("\n" + time + ": " + operationName
                                + "(" + (shortFileName == null ? event.fd : shortFileName) + "): \"" + data + "\"");
                    } else {
                        // hex
                        String data = ByteUtils.bytesToHexString(event.buffer);
                        textView.append("\n" + time + ": " + operationName
                                + "(" + (shortFileName == null ? event.fd : shortFileName) + "): " + data + "");
                    }
                    break;
                }
                case IOCTL: {
                    int request = (int) event.directArg1;
                    long arg = event.directArg2;
                    textView.append("\n" + time + ": " + operationName
                            + "(" + (shortFileName == null ? event.fd : shortFileName) + "): request="
                            + Integer.toHexString(request) + ", arg=" + Long.toHexString(arg)
                            + ", ret=" + Long.toHexString(event.retValue));
                    break;
                }
                case SELECT: {
                    textView.append("\n" + time + ": " + operationName + "(...)");
                    break;
                }
                default: {
                    textView.append("\n" + time + ": " + operationName + "(" + event.fd + ")");
                }
            }

        });
    }

    private void appendDeathEvent(NciHostDaemonProxy.RemoteDeathPacket event) {
        getActivity().runOnUiThread(() -> {
            textView.append("*** REMOTE DEATH PID:" + event.pid + " ***");
        });
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
//        homeViewModel = new ViewModelProvider(this).get(HomeViewModel.class);
//        View root = inflater.inflate(R.layout.fragment_home, container, false);
//        final TextView textView = root.findViewById(R.id.text_home);
//        homeViewModel.getText().observe(getViewLifecycleOwner(), textView::setText);
////        Activity activity = getActivity();
//        return root;
        textView = new TextView(getActivity());
        textView.setText("");
        textView.setTextSize(12);
        // Get the primary text color of the theme
        TypedValue typedValue = new TypedValue();
        Resources.Theme theme = getActivity().getTheme();
        theme.resolveAttribute(android.R.attr.textColorPrimary, typedValue, true);
        TypedArray arr = getActivity().obtainStyledAttributes(typedValue.data, new int[]{
                android.R.attr.textColorPrimary});
        int primaryColor = arr.getColor(0, -1);
        textView.setTextColor(primaryColor);
        arr.recycle();
        textView.setTypeface(Typeface.MONOSPACE);
        textView.setTextIsSelectable(true);
        // wrap in scroll view
        ScrollView scrollView = new ScrollView(getActivity());
        scrollView.addView(textView, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        return scrollView;
    }

    @Override
    public void onResume() {
        super.onResume();
        INciHostDaemon daemon = IpcNativeHandler.peekConnection();
        if (daemon == null) {
            daemon = IpcNativeHandler.connect(100);
        }
        if (daemon != null) {
            if (!daemon.isHwServiceConnected()) {
                StringBuilder sb = new StringBuilder();
                try {
                    File patchFile = NativeInterface.getNfcHalServicePatchFile(
                            NativeInterface.NfcHalServicePatch.NXP_PATCH,
                            NativeInterface.ABI.ABI_ARM_64);
                    boolean isNfcConn = daemon.initHwServiceConnection(patchFile.getAbsolutePath());
                    sb.append("isNfcConn: ").append(isNfcConn).append("\n");
                } catch (RuntimeException | IOException re) {
                    sb.append("initHwServiceConnection failed: ").append(re.toString());
                }
                Toast.makeText(getActivity(), sb.toString(), Toast.LENGTH_SHORT).show();
            }
            if (daemon.isHwServiceConnected()) {
                daemon.setRemoteEventListener(new INciHostDaemon.OnRemoteEventListener() {
                    @Override
                    public void onIoEvent(NciHostDaemonProxy.IoEventPacket event) {
                        appendIoEvent(event);
                    }

                    @Override
                    public void onRemoteDeath(NciHostDaemonProxy.RemoteDeathPacket event) {
                        appendDeathEvent(event);
                    }
                });
            }
        } else {
            Toast.makeText(getActivity(), "connect timeout", Toast.LENGTH_SHORT).show();
        }
    }
}
