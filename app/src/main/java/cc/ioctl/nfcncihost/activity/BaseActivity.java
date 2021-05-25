package cc.ioctl.nfcncihost.activity;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.FragmentManager;

import java.lang.reflect.Field;

import cc.ioctl.nfcncihost.BaseApplication;
import cc.ioctl.nfcncihost.activity.splash.SplashActivity;
import cc.ioctl.nfcncihost.procedure.StartupDirector;

public class BaseActivity extends AppCompatActivity {
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
    private boolean mImmersiveStatusBar = false;

    @Deprecated
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        Intent intent = getIntent();
        this.mIsSplashing = BaseApplication.sApplication.onActivityCreate(this, intent);
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

    protected void setImmersiveStatusBar(boolean immersive) {
        mImmersiveStatusBar = immersive;
        View decor = getWindow().getDecorView();
        if (mImmersiveStatusBar) {
            Window window = getWindow();
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.setStatusBarColor(Color.TRANSPARENT);
            decor.setSystemUiVisibility(decor.getWindowSystemUiVisibility()
                    | View.SYSTEM_UI_FLAG_IMMERSIVE | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        } else {
            decor.setSystemUiVisibility(decor.getWindowSystemUiVisibility()
                    & ~(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN));
        }
    }

    protected boolean doOnCreate(@Nullable Bundle savedInstanceState) {
        this.mOnCreateBundle = null;
        return true;
    }

    @Deprecated
    protected void onPostCreate(@Nullable Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        if (!this.mIsSplashing) {
            doOnPostCreate(savedInstanceState);
        } else {
            this.mPostCreateBundle = savedInstanceState;
        }
    }

    @Deprecated
    protected void onRestoreInstanceState(@NonNull Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        if (!this.mIsSplashing) {
            doOnRestoreInstanceState(savedInstanceState);
        } else {
            this.mOnRestoreBundle = savedInstanceState;
        }
    }

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

    @Deprecated
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (!this.mIsSplashing) {
            doOnNewIntent(intent);
        } else {
            this.mNewIntent = intent;
        }
    }

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

    @Deprecated
    protected void onStart() {
        super.onStart();
        if (!this.mIsSplashing) {
            doOnStart();
        } else {
            this.mIsStartSkipped = true;
        }
    }

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

    @Deprecated
    protected void onStop() {
        if (!this.mIsSplashing) {
            doOnStop();
        } else {
            this.mIsStartSkipped = false;
        }
        super.onStop();
    }

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

    @Deprecated
    protected void onPostResume() {
        super.onPostResume();
        if (!this.mIsSplashing) {
            doOnPostResume();
        }
    }

    @Deprecated
    public void onConfigurationChanged(Configuration newConfig) {
        if (!this.mIsSplashing) {
            doOnConfigurationChanged(newConfig);
        }
        super.onConfigurationChanged(newConfig);
    }


    @Deprecated
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (!this.mIsSplashing) {
            doOnWindowFocusChanged(hasFocus);
        } else {
            this.mWindowFocusState = hasFocus ? 1 : 0;
        }
    }

    @Deprecated
    public void onBackPressed() {
        if (!this.mIsSplashing) {
            doOnBackPressed();
        }
    }

    @Deprecated
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (this.mIsSplashing) {
            return false;
        }
        return doOnKeyDown(keyCode, event);
    }

    @Deprecated
    protected void onUserLeaveHint() {
        if (!this.mIsSplashing) {
            doOnUserLeaveHint();
        }
        super.onUserLeaveHint();
    }

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
        if (hasFocus && mImmersiveStatusBar) {
            View decorView = getWindow().getDecorView();
            decorView.setSystemUiVisibility(decorView.getWindowSystemUiVisibility()
                    | View.SYSTEM_UI_FLAG_IMMERSIVE | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        }
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
            StartupDirector director = BaseApplication.sDirector;
            if (director != null && director.isStartup()) {
                director.setDisableInterrupt(true);
            }
        }
        super.startActivityForResult(intent, requestCode, options);
    }
}
