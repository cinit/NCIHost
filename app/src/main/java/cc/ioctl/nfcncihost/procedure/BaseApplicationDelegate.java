package cc.ioctl.nfcncihost.procedure;

import android.annotation.SuppressLint;
import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.ComponentCallbacks;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentSender;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.DatabaseErrorHandler;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;

import org.lsposed.hiddenapibypass.HiddenApiBypass;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;

@SuppressWarnings({"deprecation", "MissingPermission"})
public class BaseApplicationDelegate extends BaseApplicationImpl {
    private BaseApplicationImpl mDelegate = null;

    @Override
    public void onCreate() {
        super.onCreate();
        BaseApplicationImpl.sApplication = this;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            try {
                HiddenApiBypass.setHiddenApiExemptions("L");
            } catch (NoClassDefFoundError e) {
                Log.e("HiddenApiBypass", "failed, SDK=" + Build.VERSION.SDK_INT, e);
            }
        }
        if (isMainProcess()) {
            mDelegate = new MainApplicationImpl();
        } else if (isServiceProcess()) {
            mDelegate = new WorkerApplicationImpl();
        }
        if (mDelegate != null) {
            mDelegate.attachToContextImpl(super.getBaseContext());
            mDelegate.doOnCreate();
            resetActivityThreadApplication(mDelegate);
        }
        BaseApplicationImpl.sApplication = mDelegate;
    }

    /**
     * Reset the Application for current ActivityThread, must be called before any activity or
     * service starts.
     * see {@link android.app.ActivityThread#mInitialApplication}
     * see {@link android.app.LoadedApk#mApplication}
     * see {@link android.app.Application#mLoadedApk}
     *
     * @param app new Application that has been attached to base context
     */
    @SuppressWarnings({"JavadocReference", "JavaReflectionMemberAccess"})
    @SuppressLint({"PrivateApi", "DiscouragedPrivateApi"})
    private static void resetActivityThreadApplication(@NonNull Application app) {
        Context ctxImpl = getContextImpl(app);
        try {
            // get sCurrentActivityThread
            Class<?> cActivityThread = Class.forName("android.app.ActivityThread");
            Field fAt = cActivityThread.getDeclaredField("sCurrentActivityThread");
            fAt.setAccessible(true);
            Object activityThread = fAt.get(null);
            // ActivityThread#mInitialApplication
            Field fInitApp = cActivityThread.getDeclaredField("mInitialApplication");
            fInitApp.setAccessible(true);
            fInitApp.set(activityThread, app);
            // LoadedApk#mApplication
            Class<?> cContextImpl = Class.forName("android.app.ContextImpl");
            Field fPkgInfo = cContextImpl.getDeclaredField("mPackageInfo");
            fPkgInfo.setAccessible(true);
            Object loadedApk = fPkgInfo.get(ctxImpl);
            Field fLdApp = Class.forName("android.app.LoadedApk").getDeclaredField("mApplication");
            fLdApp.setAccessible(true);
            fLdApp.set(loadedApk, app);
            // optional: set LoadedApk for Application
            Field fLoadedApk = Application.class.getDeclaredField("mLoadedApk");
            fLoadedApk.setAccessible(true);
        } catch (ReflectiveOperationException e) {
            throw (IllegalAccessError) new IllegalAccessError("resetActivityThreadApplication failed").initCause(e);
        }
    }

    public static Context getContextImpl(Context context) {
        Context nextContext;
        while ((context instanceof ContextWrapper) &&
                (nextContext = ((ContextWrapper) context).getBaseContext()) != null) {
            context = nextContext;
        }
        return context;
    }

    @Override
    public void registerComponentCallbacks(ComponentCallbacks componentCallbacks) {
        if (this.mDelegate == null) {
            super.registerComponentCallbacks(componentCallbacks);
        } else {
            this.mDelegate.registerComponentCallbacks(componentCallbacks);
        }
    }

    @Override
    public void unregisterComponentCallbacks(ComponentCallbacks componentCallbacks) {
        if (this.mDelegate == null) {
            super.unregisterComponentCallbacks(componentCallbacks);
        } else {
            this.mDelegate.unregisterComponentCallbacks(componentCallbacks);
        }
    }

    @Override
    public void registerActivityLifecycleCallbacks(Application.ActivityLifecycleCallbacks activityLifecycleCallbacks) {
        if (this.mDelegate == null) {
            super.registerActivityLifecycleCallbacks(activityLifecycleCallbacks);
        } else {
            this.mDelegate.registerActivityLifecycleCallbacks(activityLifecycleCallbacks);
        }
    }

    @Override
    public void unregisterActivityLifecycleCallbacks(Application.ActivityLifecycleCallbacks activityLifecycleCallbacks) {
        if (this.mDelegate == null) {
            super.unregisterActivityLifecycleCallbacks(activityLifecycleCallbacks);
        } else {
            this.mDelegate.unregisterActivityLifecycleCallbacks(activityLifecycleCallbacks);
        }
    }

    @Override
    public Context getBaseContext() {
        if (this.mDelegate == null) {
            return super.getBaseContext();
        }
        return this.mDelegate.getBaseContext();
    }

    @Override
    public AssetManager getAssets() {
        if (this.mDelegate == null) {
            return super.getAssets();
        }
        return this.mDelegate.getAssets();
    }

    @Override
    public Resources getResources() {
        if (this.mDelegate == null) {
            return super.getResources();
        }
        return this.mDelegate.getResources();
    }

    @Override
    public PackageManager getPackageManager() {
        if (this.mDelegate == null) {
            return super.getPackageManager();
        }
        return this.mDelegate.getPackageManager();
    }

    @Override
    public ContentResolver getContentResolver() {
        if (this.mDelegate == null) {
            return super.getContentResolver();
        }
        return this.mDelegate.getContentResolver();
    }

    @Override
    public Looper getMainLooper() {
        if (this.mDelegate == null) {
            return super.getMainLooper();
        }
        return this.mDelegate.getMainLooper();
    }

    @Override
    public Context getApplicationContext() {
        if (this.mDelegate == null) {
            return super.getApplicationContext();
        }
        return this.mDelegate.getApplicationContext();
    }

    @Override
    public void setTheme(int i) {
        if (this.mDelegate == null) {
            super.setTheme(i);
        } else {
            this.mDelegate.setTheme(i);
        }
    }

    @Override
    public Resources.Theme getTheme() {
        if (this.mDelegate == null) {
            return super.getTheme();
        }
        return this.mDelegate.getTheme();
    }

    @Override
    public ClassLoader getClassLoader() {
        if (this.mDelegate == null) {
            return super.getClassLoader();
        }
        return this.mDelegate.getClassLoader();
    }

    @Override
    public String getPackageName() {
        if (this.mDelegate == null) {
            return super.getPackageName();
        }
        return this.mDelegate.getPackageName();
    }

    @Override
    public ApplicationInfo getApplicationInfo() {
        if (this.mDelegate == null) {
            return super.getApplicationInfo();
        }
        return this.mDelegate.getApplicationInfo();
    }

    @Override
    public String getPackageResourcePath() {
        if (this.mDelegate == null) {
            return super.getPackageResourcePath();
        }
        return this.mDelegate.getPackageResourcePath();
    }

    @Override
    public String getPackageCodePath() {
        if (this.mDelegate == null) {
            return super.getPackageCodePath();
        }
        return this.mDelegate.getPackageCodePath();
    }

    @Override
    public SharedPreferences getSharedPreferences(String str, int i) {
        if (this.mDelegate == null) {
            return super.getSharedPreferences(str, i);
        }
        return this.mDelegate.getSharedPreferences(str, i);
    }

    @Override
    public FileInputStream openFileInput(String str) throws FileNotFoundException {
        if (this.mDelegate == null) {
            return super.openFileInput(str);
        }
        return this.mDelegate.openFileInput(str);
    }

    @Override
    public FileOutputStream openFileOutput(String str, int i) throws FileNotFoundException {
        if (this.mDelegate == null) {
            return super.openFileOutput(str, i);
        }
        return this.mDelegate.openFileOutput(str, i);
    }

    @Override
    public boolean deleteFile(String str) {
        if (this.mDelegate == null) {
            return super.deleteFile(str);
        }
        return this.mDelegate.deleteFile(str);
    }

    @Override
    public File getFileStreamPath(String str) {
        if (this.mDelegate == null) {
            return super.getFileStreamPath(str);
        }
        return this.mDelegate.getFileStreamPath(str);
    }

    @Override
    public String[] fileList() {
        if (this.mDelegate == null) {
            return super.fileList();
        }
        return this.mDelegate.fileList();
    }

    @Override
    public File getFilesDir() {
        if (this.mDelegate == null) {
            return super.getFilesDir();
        }
        return this.mDelegate.getFilesDir();
    }

    @Override
    public File getExternalFilesDir(String str) {
        if (this.mDelegate == null) {
            return super.getExternalFilesDir(str);
        }
        return this.mDelegate.getExternalFilesDir(str);
    }

    @Override
    public File getObbDir() {
        if (this.mDelegate == null) {
            return super.getObbDir();
        }
        return this.mDelegate.getObbDir();
    }

    @Override
    public File getCacheDir() {
        if (this.mDelegate == null) {
            return super.getCacheDir();
        }
        return this.mDelegate.getCacheDir();
    }

    @Override
    public File getExternalCacheDir() {
        if (this.mDelegate == null) {
            return super.getExternalCacheDir();
        }
        return this.mDelegate.getExternalCacheDir();
    }

    @Override
    public File getDir(String str, int i) {
        if (this.mDelegate == null) {
            return super.getDir(str, i);
        }
        return this.mDelegate.getDir(str, i);
    }

    @Override
    public SQLiteDatabase openOrCreateDatabase(String str, int i, SQLiteDatabase.CursorFactory cursorFactory) {
        if (this.mDelegate == null) {
            return super.openOrCreateDatabase(str, i, cursorFactory);
        }
        return this.mDelegate.openOrCreateDatabase(str, i, cursorFactory);
    }

    @Override
    public SQLiteDatabase openOrCreateDatabase(String str, int i, SQLiteDatabase.CursorFactory cursorFactory, DatabaseErrorHandler databaseErrorHandler) {
        if (this.mDelegate == null) {
            return super.openOrCreateDatabase(str, i, cursorFactory, databaseErrorHandler);
        }
        return this.mDelegate.openOrCreateDatabase(str, i, cursorFactory, databaseErrorHandler);
    }

    @Override
    public boolean deleteDatabase(String str) {
        if (this.mDelegate == null) {
            return super.deleteDatabase(str);
        }
        return this.mDelegate.deleteDatabase(str);
    }

    @Override
    public File getDatabasePath(String str) {
        if (this.mDelegate == null) {
            return super.getDatabasePath(str);
        }
        return this.mDelegate.getDatabasePath(str);
    }

    @Override
    public String[] databaseList() {
        if (this.mDelegate == null) {
            return super.databaseList();
        }
        return this.mDelegate.databaseList();
    }

    @Override
    public Drawable getWallpaper() {
        if (this.mDelegate == null) {
            return super.getWallpaper();
        }
        return this.mDelegate.getWallpaper();
    }

    @Override
    public Drawable peekWallpaper() {
        if (this.mDelegate == null) {
            return super.peekWallpaper();
        }
        return this.mDelegate.peekWallpaper();
    }

    @Override
    public int getWallpaperDesiredMinimumWidth() {
        if (this.mDelegate == null) {
            return super.getWallpaperDesiredMinimumWidth();
        }
        return this.mDelegate.getWallpaperDesiredMinimumWidth();
    }

    @Override
    public int getWallpaperDesiredMinimumHeight() {
        if (this.mDelegate == null) {
            return super.getWallpaperDesiredMinimumHeight();
        }
        return this.mDelegate.getWallpaperDesiredMinimumHeight();
    }

    @Override
    public void setWallpaper(Bitmap bitmap) throws IOException {
        if (this.mDelegate == null) {
            super.setWallpaper(bitmap);
        } else {
            this.mDelegate.setWallpaper(bitmap);
        }
    }

    @Override
    public void setWallpaper(InputStream inputStream) throws IOException {
        if (this.mDelegate == null) {
            super.setWallpaper(inputStream);
        } else {
            this.mDelegate.setWallpaper(inputStream);
        }
    }

    @Override
    public void clearWallpaper() throws IOException {
        if (this.mDelegate == null) {
            super.clearWallpaper();
        } else {
            this.mDelegate.clearWallpaper();
        }
    }

    @Override
    public void startActivity(Intent intent) {
        if (this.mDelegate == null) {
            super.startActivity(intent);
        } else {
            this.mDelegate.startActivity(intent);
        }
    }

    @Override
    public void startActivities(Intent[] intentArr) {
        if (this.mDelegate == null) {
            super.startActivities(intentArr);
        } else {
            this.mDelegate.startActivities(intentArr);
        }
    }

    @Override
    public void startIntentSender(IntentSender intentSender, Intent intent, int i, int i2, int i3) throws IntentSender.SendIntentException {
        if (this.mDelegate == null) {
            super.startIntentSender(intentSender, intent, i, i2, i3);
        } else {
            this.mDelegate.startIntentSender(intentSender, intent, i, i2, i3);
        }
    }

    @Override
    public void sendBroadcast(Intent intent) {
        if (this.mDelegate == null) {
            super.sendBroadcast(intent);
        } else {
            this.mDelegate.sendBroadcast(intent);
        }
    }

    @Override
    public void sendBroadcast(Intent intent, String str) {
        if (this.mDelegate == null) {
            super.sendBroadcast(intent, str);
        } else {
            this.mDelegate.sendBroadcast(intent, str);
        }
    }

    @Override
    public void sendOrderedBroadcast(Intent intent, String str) {
        if (this.mDelegate == null) {
            super.sendBroadcast(intent, str);
        } else {
            this.mDelegate.sendOrderedBroadcast(intent, str);
        }
    }

    @Override
    public void sendOrderedBroadcast(Intent intent, String str, BroadcastReceiver broadcastReceiver, Handler handler, int i, String str2, Bundle bundle) {
        if (this.mDelegate == null) {
            super.sendOrderedBroadcast(intent, str, broadcastReceiver, handler, i, str2, bundle);
        } else {
            this.mDelegate.sendOrderedBroadcast(intent, str, broadcastReceiver, handler, i, str2, bundle);
        }
    }

    @SuppressLint("MissingPermission")
    @Override
    public void sendStickyBroadcast(Intent intent) {
        if (this.mDelegate == null) {
            super.sendStickyBroadcast(intent);
        } else {
            this.mDelegate.sendStickyBroadcast(intent);
        }
    }

    @Override
    public void sendStickyOrderedBroadcast(Intent intent, BroadcastReceiver broadcastReceiver, Handler handler, int i, String str, Bundle bundle) {
        if (this.mDelegate == null) {
            super.sendStickyOrderedBroadcast(intent, broadcastReceiver, handler, i, str, bundle);
        } else {
            this.mDelegate.sendStickyOrderedBroadcast(intent, broadcastReceiver, handler, i, str, bundle);
        }
    }

    @Override
    public void removeStickyBroadcast(Intent intent) {
        if (this.mDelegate == null) {
            super.removeStickyBroadcast(intent);
        } else {
            this.mDelegate.removeStickyBroadcast(intent);
        }
    }

    @Override
    public Intent registerReceiver(BroadcastReceiver broadcastReceiver, IntentFilter intentFilter) {
        if (this.mDelegate == null) {
            return super.registerReceiver(broadcastReceiver, intentFilter);
        }
        return this.mDelegate.registerReceiver(broadcastReceiver, intentFilter);
    }

    @Override
    public Intent registerReceiver(BroadcastReceiver broadcastReceiver, IntentFilter intentFilter, String str, Handler handler) {
        if (this.mDelegate == null) {
            return super.registerReceiver(broadcastReceiver, intentFilter, str, handler);
        }
        return this.mDelegate.registerReceiver(broadcastReceiver, intentFilter, str, handler);
    }

    @Override
    public void unregisterReceiver(BroadcastReceiver broadcastReceiver) {
        if (this.mDelegate == null) {
            super.unregisterReceiver(broadcastReceiver);
        } else {
            this.mDelegate.unregisterReceiver(broadcastReceiver);
        }
    }

    @Override
    public ComponentName startService(Intent intent) {
        if (this.mDelegate == null) {
            return super.startService(intent);
        }
        return this.mDelegate.startService(intent);
    }

    @Override
    public boolean stopService(Intent intent) {
        if (this.mDelegate == null) {
            return super.stopService(intent);
        }
        return this.mDelegate.stopService(intent);
    }

    @Override
    public boolean bindService(Intent intent, ServiceConnection serviceConnection, int i) {
        if (this.mDelegate == null) {
            return super.bindService(intent, serviceConnection, i);
        }
        return this.mDelegate.bindService(intent, serviceConnection, i);
    }

    @Override
    public void unbindService(ServiceConnection serviceConnection) {
        if (this.mDelegate == null) {
            super.unbindService(serviceConnection);
        } else {
            this.mDelegate.unbindService(serviceConnection);
        }
    }

    @Override
    public boolean startInstrumentation(ComponentName componentName, String str, Bundle bundle) {
        if (this.mDelegate == null) {
            return super.startInstrumentation(componentName, str, bundle);
        }
        return this.mDelegate.startInstrumentation(componentName, str, bundle);
    }

    @Override
    public Object getSystemService(String str) {
        if (this.mDelegate == null) {
            return super.getSystemService(str);
        }
        return this.mDelegate.getSystemService(str);
    }

    @Override
    public int checkPermission(String str, int i, int i2) {
        if (this.mDelegate == null) {
            return super.checkPermission(str, i, i2);
        }
        return this.mDelegate.checkPermission(str, i, i2);
    }

    @Override
    public int checkCallingPermission(String str) {
        if (this.mDelegate == null) {
            return super.checkCallingPermission(str);
        }
        return this.mDelegate.checkCallingPermission(str);
    }

    @Override
    public int checkCallingOrSelfPermission(String str) {
        if (this.mDelegate == null) {
            return super.checkCallingOrSelfPermission(str);
        }
        return this.mDelegate.checkCallingOrSelfPermission(str);
    }

    @Override
    public void enforcePermission(String str, int i, int i2, String str2) {
        if (this.mDelegate == null) {
            super.enforcePermission(str, i, i2, str2);
        } else {
            this.mDelegate.enforcePermission(str, i, i2, str2);
        }
    }

    @Override
    public void enforceCallingPermission(String str, String str2) {
        if (this.mDelegate == null) {
            super.enforceCallingPermission(str, str2);
        } else {
            this.mDelegate.enforceCallingPermission(str, str2);
        }
    }

    @Override
    public void enforceCallingOrSelfPermission(String str, String str2) {
        if (this.mDelegate == null) {
            super.enforceCallingOrSelfPermission(str, str2);
        } else {
            this.mDelegate.enforceCallingOrSelfPermission(str, str2);
        }
    }

    @Override
    public void grantUriPermission(String str, Uri uri, int i) {
        if (this.mDelegate == null) {
            super.grantUriPermission(str, uri, i);
        } else {
            this.mDelegate.grantUriPermission(str, uri, i);
        }
    }

    @Override
    public void revokeUriPermission(Uri uri, int i) {
        if (this.mDelegate == null) {
            super.revokeUriPermission(uri, i);
        } else {
            this.mDelegate.revokeUriPermission(uri, i);
        }
    }

    @Override
    public int checkUriPermission(Uri uri, int i, int i2, int i3) {
        if (this.mDelegate == null) {
            return super.checkUriPermission(uri, i, i2, i3);
        }
        return this.mDelegate.checkUriPermission(uri, i, i2, i3);
    }

    @Override
    public int checkCallingUriPermission(Uri uri, int i) {
        if (this.mDelegate == null) {
            return super.checkCallingUriPermission(uri, i);
        }
        return this.mDelegate.checkCallingUriPermission(uri, i);
    }

    @Override
    public int checkCallingOrSelfUriPermission(Uri uri, int i) {
        if (this.mDelegate == null) {
            return super.checkCallingOrSelfUriPermission(uri, i);
        }
        return this.mDelegate.checkCallingOrSelfUriPermission(uri, i);
    }

    @Override
    public int checkUriPermission(Uri uri, String str, String str2, int i, int i2, int i3) {
        if (this.mDelegate == null) {
            return super.checkUriPermission(uri, str, str2, i, i2, i3);
        }
        return this.mDelegate.checkUriPermission(uri, str, str2, i, i2, i3);
    }

    @Override
    public void enforceUriPermission(Uri uri, int i, int i2, int i3, String str) {
        if (this.mDelegate == null) {
            super.enforceUriPermission(uri, i, i2, i3, str);
        } else {
            this.mDelegate.enforceUriPermission(uri, i, i2, i3, str);
        }
    }

    @Override
    public void enforceCallingUriPermission(Uri uri, int i, String str) {
        if (this.mDelegate == null) {
            super.enforceCallingUriPermission(uri, i, str);
        } else {
            this.mDelegate.enforceCallingUriPermission(uri, i, str);
        }
    }

    @Override
    public void enforceCallingOrSelfUriPermission(Uri uri, int i, String str) {
        if (this.mDelegate == null) {
            super.enforceCallingOrSelfUriPermission(uri, i, str);
        } else {
            this.mDelegate.enforceCallingOrSelfUriPermission(uri, i, str);
        }
    }

    @Override
    public void enforceUriPermission(Uri uri, String str, String str2, int i, int i2, int i3, String str3) {
        if (this.mDelegate == null) {
            super.enforceUriPermission(uri, str, str2, i, i2, i3, str3);
        } else {
            this.mDelegate.enforceUriPermission(uri, str, str2, i, i2, i3, str3);
        }
    }

    @Override
    public Context createPackageContext(String str, int i)
            throws PackageManager.NameNotFoundException {
        if (this.mDelegate == null) {
            return super.createPackageContext(str, i);
        }
        return this.mDelegate.createPackageContext(str, i);
    }

    @Override
    public boolean isRestricted() {
        if (this.mDelegate == null) {
            return super.isRestricted();
        }
        return this.mDelegate.isRestricted();
    }

    @Override
    public void onLowMemory() {
        if (this.mDelegate == null) {
            super.onLowMemory();
        }
        this.mDelegate.onLowMemory();
    }

    @Override
    public void onTrimMemory(int level) {
        if (this.mDelegate == null) {
            super.onTrimMemory(level);
        }
        this.mDelegate.onTrimMemory(level);
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        if (this.mDelegate == null) {
            super.onConfigurationChanged(newConfig);
        }
        this.mDelegate.onConfigurationChanged(newConfig);
    }
}
