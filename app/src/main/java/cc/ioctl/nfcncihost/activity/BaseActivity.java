package cc.ioctl.nfcncihost.activity;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;

import java.lang.reflect.Field;

import cc.ioctl.nfcncihost.activity.splash.SplashActivity;
import cc.ioctl.nfcncihost.procedure.MainApplicationImpl;
import cc.ioctl.nfcncihost.procedure.StartupDirector;

/**
 * Late-onCreate feature for Activity
 */
public class BaseActivity extends AppActivity {
    private static final String FRAGMENTS_TAG = "android:support:fragments";
    private boolean mIsFinishingInOnCreate = false;
    private boolean mIsResultWaiting;
    private boolean mIsResume = false;
    private boolean mIsSplashing = false;
    private boolean mIsStartSkipped = false;
    private Intent mNewIntent;
    private Bundle mOnCreateBundle = null;
    private Bundle mOnRestoreBundle;
    private Bundle mPostCreateBundle = null;
    private int mRequestCode;
    private int mResultCode;
    private Intent mResultData;
    private int mWindowFocusState = -1;

    @Deprecated
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        Intent intent = getIntent();
        this.mIsSplashing = MainApplicationImpl.getInstance().onActivityCreate(this, intent);
        if (this.mIsSplashing) {
            this.mOnCreateBundle = savedInstanceState;
            if (savedInstanceState != null) {
                savedInstanceState.remove(FRAGMENTS_TAG);
            }
            super.onCreate(savedInstanceState);
        } else {
            super.onCreate(savedInstanceState);
            doOnCreate(savedInstanceState);
        }
    }

    protected boolean doOnCreate(@Nullable Bundle savedInstanceState) {
        this.mOnCreateBundle = null;
        return true;
    }

    @Override
    @Deprecated
    protected void onPostCreate(@Nullable Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        if (!this.mIsSplashing) {
            doOnPostCreate(savedInstanceState);
        } else {
            this.mPostCreateBundle = savedInstanceState;
        }
    }

    @Override
    @Deprecated
    protected void onRestoreInstanceState(@NonNull Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        if (!this.mIsSplashing) {
            doOnRestoreInstanceState(savedInstanceState);
        } else {
            this.mOnRestoreBundle = savedInstanceState;
        }
    }

    @Override
    @Deprecated
    protected void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        if (!this.mIsSplashing) {
            doOnSaveInstanceState(outState);
        }
    }

    @Override
    @Deprecated
    protected void onDestroy() {
        if (!this.mIsSplashing || this.mIsFinishingInOnCreate) {
            doOnDestroy();
        }
        super.onDestroy();
    }

    public void callOnCreateProc() {
        boolean hasFocus = true;
        if (this.mIsSplashing) {
            this.mIsSplashing = false;
            bypassStateLossCheck();
            if (doOnCreate(this.mOnCreateBundle) && !isFinishing()) {
                if (this.mIsStartSkipped) {
                    doOnStart();
                    this.mIsStartSkipped = false;
                    if (this.mOnRestoreBundle != null) {
                        doOnRestoreInstanceState(this.mOnRestoreBundle);
                        this.mOnRestoreBundle = null;
                    }
                    doOnPostCreate(this.mPostCreateBundle);
                    if (this.mIsResultWaiting) {
                        doOnActivityResult(this.mRequestCode, this.mResultCode, this.mResultData);
                        this.mIsResultWaiting = false;
                        this.mResultData = null;
                    }
                    if (this.mNewIntent != null) {
                        doOnNewIntent(this.mNewIntent);
                        this.mNewIntent = null;
                    }
                    if (isResume()) {
                        doOnResume();
                        doOnPostResume();
                    }
                    if (this.mWindowFocusState != -1) {
                        if (this.mWindowFocusState != 1) {
                            hasFocus = false;
                        }
                        doOnWindowFocusChanged(hasFocus);
                    }
                }
            } else if (isFinishing()) {
                this.mIsSplashing = true;
                this.mIsFinishingInOnCreate = true;
            }
        }
    }

    private static final Field fStateSaved, fStopped;

    static {
        Class<?> clz = androidx.fragment.app.FragmentManager.class;
        try {
            fStateSaved = clz.getDeclaredField("mStateSaved");
            fStateSaved.setAccessible(true);
            fStopped = clz.getDeclaredField("mStopped");
            fStopped.setAccessible(true);
        } catch (NoSuchFieldException e) {
            throw new RuntimeException(e);
        }
    }

    private void bypassStateLossCheck() {
        FragmentManager fragmentMgr = getSupportFragmentManager();
        try {
            fStopped.set(fragmentMgr, false);
            fStateSaved.set(fragmentMgr, false);
        } catch (IllegalAccessException ignored) {
        }
    }

    @Override
    @Deprecated
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (!this.mIsSplashing) {
            doOnNewIntent(intent);
        } else {
            this.mNewIntent = intent;
        }
    }

    @Override
    @Deprecated
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (!this.mIsSplashing) {
            doOnActivityResult(requestCode, resultCode, intent);
        } else {
            this.mIsResultWaiting = true;
            this.mRequestCode = requestCode;
            this.mResultCode = resultCode;
            this.mResultData = intent;
        }
        super.onActivityResult(requestCode, resultCode, intent);
    }

    @Override
    @Deprecated
    protected void onStart() {
        super.onStart();
        if (!this.mIsSplashing) {
            doOnStart();
        } else {
            this.mIsStartSkipped = true;
        }
    }

    @Override
    @Deprecated
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (this.mIsSplashing) {
            return false;
        }
        return doDispatchKeyEvent(event);
    }

    protected boolean doDispatchKeyEvent(KeyEvent event) {
        return super.dispatchKeyEvent(event);
    }

    @Override
    @Deprecated
    protected void onStop() {
        if (!this.mIsSplashing) {
            doOnStop();
        } else {
            this.mIsStartSkipped = false;
        }
        super.onStop();
    }

    @Override
    @Deprecated
    protected void onResume() {
        try {
            super.onResume();
        } catch (IllegalArgumentException e) {
            try {
                Field mCalled = Activity.class.getDeclaredField("mCalled");
                mCalled.setAccessible(true);
                mCalled.set(this, true);
            } catch (Exception ignored) {
            }
        } catch (NullPointerException e6) {
        }
        this.mIsResume = true;
        if (!this.mIsSplashing) {
            doOnResume();
        }
    }

    @Override
    @Deprecated
    protected void onPostResume() {
        super.onPostResume();
        if (!this.mIsSplashing) {
            doOnPostResume();
        }
    }

    @Override
    @Deprecated
    public void onConfigurationChanged(Configuration newConfig) {
        if (!this.mIsSplashing) {
            doOnConfigurationChanged(newConfig);
        }
        super.onConfigurationChanged(newConfig);
    }


    @Override
    @Deprecated
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (!this.mIsSplashing) {
            doOnWindowFocusChanged(hasFocus);
        } else {
            this.mWindowFocusState = hasFocus ? 1 : 0;
        }
    }

    @Override
    @Deprecated
    public void onBackPressed() {
        if (!this.mIsSplashing) {
            doOnBackPressed();
        }
    }

    @Override
    @Deprecated
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (this.mIsSplashing) {
            return false;
        }
        return doOnKeyDown(keyCode, event);
    }

    @Override
    @Deprecated
    protected void onUserLeaveHint() {
        if (!this.mIsSplashing) {
            doOnUserLeaveHint();
        }
        super.onUserLeaveHint();
    }

    @Override
    @Deprecated
    protected void onPause() {
        if (!this.mIsSplashing) {
            doOnPause();
        }
        this.mIsResume = false;
        super.onPause();
    }

    protected void doOnPostCreate(Bundle bundle) {
        this.mPostCreateBundle = null;
    }

    protected void doOnRestoreInstanceState(Bundle bundle) {
    }

    protected void doOnSaveInstanceState(Bundle outState) {
    }

    protected void doOnStart() {
    }

    protected void doOnNewIntent(Intent intent) {
    }

    protected void doOnActivityResult(int requestCode, int resultCode, Intent intent) {
    }

    protected void doOnResume() {
    }

    protected void doOnPostResume() {
    }

    protected void doOnWindowFocusChanged(boolean hasFocus) {
    }

    protected void doOnConfigurationChanged(Configuration newConfig) {
    }

    protected void doOnBackPressed() {
        try {
            super.onBackPressed();
        } catch (Throwable th) {
        }
    }

    protected boolean doOnKeyDown(int keyCode, KeyEvent event) {
        return super.onKeyDown(keyCode, event);
    }

    protected void doOnUserLeaveHint() {
    }

    protected void doOnPause() {
    }

    protected void doOnStop() {
    }

    protected void doOnDestroy() {
    }

    protected final boolean isResume() {
        return this.mIsResume;
    }

    @Override
    public void startActivityForResult(Intent intent, int requestCode, @Nullable Bundle options) {
        if (this instanceof SplashActivity) {
            StartupDirector director = MainApplicationImpl.sDirector;
            if (director != null && director.isStartup()) {
                director.setDisableInterception(true);
            }
        }
        super.startActivityForResult(intent, requestCode, options);
    }
}
