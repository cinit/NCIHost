package cc.ioctl.nfcdevicehost.util;

import androidx.annotation.NonNull;

import java.util.UUID;

public class Utils {

    @NonNull
    public static String generateUuid() {
        return UUID.randomUUID().toString();
    }

    public static void test(){
//        Parcel p=new Parcel();
//        p.writeArray();

    }
}
