package cc.ioctl.nfcdevicehost.xposed;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public class HookEntry implements IXposedHookLoadPackage {

    private static void initNfcServiceHook(ClassLoader classLoader) {
        XposedHelpers.findAndHookMethod("com.android.nfc.NfcService", classLoader, "playSound", int.class, new XC_MethodHook() {
            @Override
            protected void beforeHookedMethod(MethodHookParam param) {
                if (SysNfcSvcPatch.sDisableNfcDiscoverySound) {
                    param.setResult(null);
                }
            }
        });
        SysNfcSvcPatch.startMainThread();
    }

    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam lpparam) {
        if ("com.android.nfc".equals(lpparam.packageName)) {
            if (lpparam.isFirstApplication) {
                initNfcServiceHook(lpparam.classLoader);
            }
        }
    }
}
