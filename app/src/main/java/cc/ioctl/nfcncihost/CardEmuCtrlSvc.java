package cc.ioctl.nfcncihost;

import android.os.Build;
import android.service.controls.Control;
import android.service.controls.ControlsProviderService;
import android.service.controls.actions.ControlAction;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import java.util.List;
import java.util.concurrent.Flow;
import java.util.function.Consumer;

@RequiresApi(api = Build.VERSION_CODES.R)
public class CardEmuCtrlSvc extends ControlsProviderService {
    @NonNull
    @Override
    public Flow.Publisher<Control> createPublisherForAllAvailable() {
//        Context context = getBaseContext();
//        Intent i = new Intent();
//        PendingIntent pi = PendingIntent.getActivity(context, 1, i, PendingIntent.FLAG_UPDATE_CURRENT);
//        List<Control> controls = new ArrayList<>();
//        Control control = new Control.StatelessBuilder(MY-UNIQUE-DEVICE-ID, pi)
//                // Required: The name of the control
//                .setTitle(MY-CONTROL-TITLE)
//                // Required: Usually the room where the control is located
//                .setSubtitle(MY-CONTROL-SUBTITLE)
//                // Optional: Structure where the control is located, an example would be a house
//                .setStructure(MY-CONTROL-STRUCTURE)
//                // Required: Type of device, i.e., thermostat, light, switch
//                .setDeviceType(DeviceTypes.DEVICE-TYPE) // For example, DeviceTypes.TYPE_THERMOSTAT
//                .build();
//        controls.add(control);
//        // Create more controls here if needed and add it to the ArrayList
//
//        // Uses the RxJava 2 library
        return Flow.Subscriber::onComplete;
    }

    @NonNull
    @Override
    public Flow.Publisher<Control> createPublisherFor(@NonNull List<String> controlIds) {
        return null;
    }

    @Override
    public void performControlAction(@NonNull String controlId,
                                     @NonNull ControlAction action, @NonNull Consumer<Integer> consumer) {

    }
}
