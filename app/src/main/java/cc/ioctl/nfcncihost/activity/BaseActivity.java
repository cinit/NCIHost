package cc.ioctl.nfcncihost.activity;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;

import java.lang.reflect.Field;

import cc.ioctl.nfcncihost.activity.ui.startup.TransientInitActivity;
import cc.ioctl.nfcncihost.procedure.MainApplicationImpl;
import cc.ioctl.nfcncihost.procedure.StartupDirector;

/**
 * Late-onCreate feature for Activity
 * TODO: implement Fragment late-onCreate feature
 */
public class BaseActivity extends AppActivity {
    private static final String FRAGMENTS_TAG = "android:support:fragments";
    private boolean mIsFinishingInOnCreate = false;
    private boolean mIsResultWaiting;
    private boolean mIsResume = false;
    private boolean mIsInitializing = false;
    private boolean mIsStartSkipped = false;
    private Intent mNewIntent;
    private Bundle mOnCreateBundle = null;
    private Bundle mOnRestoreBundle;
    private Bundle mPostCreateBundle = null;
    private int mRequestCode;
    private int mResultCode;
    private Intent mResultData;
    private int mWindowFocusState = -1;

    protected boolean isTranslucentSystemUi() {
        return true;
    }

    /**
     * @deprecated use {@link #doOnCreate(Bundle)} instead.
     */
    @Deprecated
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        Intent intent = getIntent();
        this.mIsInitializing = MainApplicationImpl.getInstance().onActivityCreate(this, intent);
        if (this.mIsInitializing) {
            this.mOnCreateBundle = savedInstanceState;
            if (savedInstanceState != null) {
                savedInstanceState.remove(FRAGMENTS_TAG);
            }
            super.onCreate(shouldRetainActivitySavedInstanceState() ? savedInstanceState : null);
        } else {
            super.onCreate(shouldRetainActivitySavedInstanceState() ? savedInstanceState : null);
            doOnCreate(savedInstanceState);
        }
    }

    @SuppressWarnings("unused")
    protected boolean doOnCreate(@Nullable Bundle savedInstanceState) {
        this.mOnCreateBundle = null;
        if (isTranslucentSystemUi()) {
            requestTranslucentSystemUi();
        }
        return true;
    }

    /**
     * @deprecated use {@link #doOnPostCreate(Bundle)} instead.
     */
    @Override
    @Deprecated
    protected void onPostCreate(@Nullable Bundle savedInstanceState) {
        super.onPostCreate(shouldRetainActivitySavedInstanceState() ? savedInstanceState : null);
        if (!this.mIsInitializing) {
            doOnPostCreate(savedInstanceState);
        } else {
            this.mPostCreateBundle = savedInstanceState;
        }
    }

    /**
     * Get whether the savedInstanceState should be ignored. This is useful for activities that
     * saved instance are transient and should not be saved. If this method returns true, the
     * savedInstanceState passed to {@link Activity#onCreate(Bundle)}, {@link Activity#onPostCreate(Bundle)}
     * and {@link Activity#onRestoreInstanceState(Bundle)} will be null, but {@link BaseActivity#doOnCreate(Bundle)}
     * and {@link BaseActivity#doOnPostCreate(Bundle)} will still receive the original savedInstanceState.
     * A trivial activity that does not handle savedInstanceState specially can return true.
     * Note: This method is called before {@link #doOnCreate(Bundle)} and should be constexpr.
     *
     * @return true if the savedInstanceState should be ignored.
     */
    protected boolean shouldRetainActivitySavedInstanceState() {
        return true;
    }

    /**
     * @deprecated use {@link #doOnRestoreInstanceState(Bundle)} instead.
     */
    @Override
    @Deprecated
    protected void onRestoreInstanceState(@NonNull Bundle savedInstanceState) {
        super.onRestoreInstanceState(shouldRetainActivitySavedInstanceState() ? savedInstanceState : null);
        if (!this.mIsInitializing) {
            doOnRestoreInstanceState(savedInstanceState);
        } else {
            this.mOnRestoreBundle = savedInstanceState;
        }
    }

    /**
     * @deprecated use {@link #doOnSaveInstanceState(Bundle)} instead.
     */
    @Override
    @Deprecated
    protected void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        if (!this.mIsInitializing) {
            doOnSaveInstanceState(outState);
        }
    }

    /**
     * @deprecated use {@link #doOnDestroy()} instead.
     */
    @Override
    @Deprecated
    protected void onDestroy() {
        if (!this.mIsInitializing || this.mIsFinishingInOnCreate) {
            doOnDestroy();
        }
        super.onDestroy();
    }

    public void callOnCreateProcInternal() {
        boolean hasFocus = true;
        if (this.mIsInitializing) {
            this.mIsInitializing = false;
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
                this.mIsInitializing = true;
                this.mIsFinishingInOnCreate = true;
            }
        }
    }

    private static final Field fStateSaved;
    private static final Field fStopped;

    static {
        Class<?> clz = androidx.fragment.app.FragmentManager.class;
        try {
            fStateSaved = clz.getDeclaredField("mStateSaved");
            fStateSaved.setAccessible(true);
            fStopped = clz.getDeclaredField("mStopped");
            fStopped.setAccessible(true);
        } catch (NoSuchFieldException e) {
            // we can hardly do anything here, we just want to die die die...
            throw new NoSuchFieldError(e.getMessage());
        }
    }

    private void bypassStateLossCheck() {
        FragmentManager fragmentMgr = getSupportFragmentManager();
        try {
            fStopped.set(fragmentMgr, false);
            fStateSaved.set(fragmentMgr, false);
        } catch (IllegalAccessException ignored) {
            // should not happen
        }
    }

    /**
     * @deprecated use {@link #doOnNewIntent(Intent)} instead.
     */
    @Override
    @Deprecated
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (!this.mIsInitializing) {
            doOnNewIntent(intent);
        } else {
            this.mNewIntent = intent;
        }
    }

    /**
     * @deprecated use {@link #doOnActivityResult(int, int, Intent)} instead.
     */
    @Override
    @Deprecated
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (!this.mIsInitializing) {
            doOnActivityResult(requestCode, resultCode, intent);
        } else {
            this.mIsResultWaiting = true;
            this.mRequestCode = requestCode;
            this.mResultCode = resultCode;
            this.mResultData = intent;
        }
        super.onActivityResult(requestCode, resultCode, intent);
    }

    /**
     * @deprecated use {@link #doOnStart()} instead.
     */
    @Override
    @Deprecated
    protected void onStart() {
        super.onStart();
        if (!this.mIsInitializing) {
            doOnStart();
        } else {
            this.mIsStartSkipped = true;
        }
    }

    /**
     * @deprecated use {@link #doDispatchKeyEvent(KeyEvent)} instead.
     */
    @Override
    @Deprecated
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (this.mIsInitializing) {
            return false;
        }
        return doDispatchKeyEvent(event);
    }

    protected boolean doDispatchKeyEvent(KeyEvent event) {
        return super.dispatchKeyEvent(event);
    }

    /**
     * @deprecated use {@link #doOnStop()} instead.
     */
    @Override
    @Deprecated
    protected void onStop() {
        if (!this.mIsInitializing) {
            doOnStop();
        } else {
            this.mIsStartSkipped = false;
        }
        super.onStop();
    }

    /**
     * @deprecated use {@link #doOnResume()} instead.
     */
    @Override
    @Deprecated
    @SuppressWarnings("JavaReflectionMemberAccess")
    @SuppressLint("DiscouragedPrivateApi")
    protected void onResume() {
        try {
            super.onResume();
        } catch (IllegalArgumentException e) {
            try {
                Field mCalled = Activity.class.getDeclaredField("mCalled");
                mCalled.setAccessible(true);
                mCalled.set(this, true);
            } catch (ReflectiveOperationException ignored) {
                throw (NoSuchFieldError) new NoSuchFieldError("set Activity.mCalled failed while super.onResume() throw IllegalArgumentException")
                        .initCause(e);
            }
        } catch (NullPointerException ignored) {
            // i don't know
        }
        this.mIsResume = true;
        if (!this.mIsInitializing) {
            doOnResume();
        }
    }

    /**
     * @deprecated use {@link #doOnPostResume()} instead.
     */
    @Override
    @Deprecated
    protected void onPostResume() {
        super.onPostResume();
        if (!this.mIsInitializing) {
            doOnPostResume();
        }
    }

    /**
     * @deprecated use {@link #doOnConfigurationChanged(Configuration)} instead.
     */
    @Override
    @Deprecated
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        if (!this.mIsInitializing) {
            doOnConfigurationChanged(newConfig);
        }
        super.onConfigurationChanged(newConfig);
    }

    /**
     * @deprecated use {@link #doOnWindowFocusChanged(boolean)} instead.
     */
    @Override
    @Deprecated
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (!this.mIsInitializing) {
            doOnWindowFocusChanged(hasFocus);
        } else {
            this.mWindowFocusState = hasFocus ? 1 : 0;
        }
    }

    /**
     * @deprecated use {@link #doOnBackPressed()} instead.
     */
    @Override
    @Deprecated
    public void onBackPressed() {
        if (!this.mIsInitializing) {
            doOnBackPressed();
        }
    }

    /**
     * @deprecated use {@link #doOnKeyDown(int, KeyEvent)} instead.
     */
    @Override
    @Deprecated
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (this.mIsInitializing) {
            return false;
        }
        return doOnKeyDown(keyCode, event);
    }

    /**
     * @deprecated use {@link #doOnUserLeaveHint()} instead.
     */
    @Override
    @Deprecated
    protected void onUserLeaveHint() {
        if (!this.mIsInitializing) {
            doOnUserLeaveHint();
        }
        super.onUserLeaveHint();
    }

    /**
     * @deprecated use {@link #doOnPause()} instead.
     */
    @Override
    @Deprecated
    protected void onPause() {
        if (!this.mIsInitializing) {
            doOnPause();
        }
        this.mIsResume = false;
        super.onPause();
    }

    @SuppressWarnings("unused")
    protected void doOnPostCreate(Bundle bundle) {
        this.mPostCreateBundle = null;
    }

    @SuppressWarnings("unused")
    protected void doOnRestoreInstanceState(Bundle bundle) {
        // to implement
    }

    protected void doOnSaveInstanceState(Bundle outState) {
        // to implement
    }

    protected void doOnStart() {
        // to implement
    }

    @SuppressWarnings("unused")
    protected void doOnNewIntent(Intent intent) {
        // to implement
    }

    @SuppressWarnings("unused")
    protected void doOnActivityResult(int requestCode, int resultCode, Intent intent) {
        // to implement
    }

    protected void doOnResume() {
        // to implement
    }

    protected void doOnPostResume() {
        // to implement
    }

    @SuppressWarnings("unused")
    protected void doOnWindowFocusChanged(boolean hasFocus) {
        // to implement
    }

    @SuppressWarnings("unused")
    protected void doOnConfigurationChanged(Configuration newConfig) {// to implement
    }

    protected void doOnBackPressed() {
        super.onBackPressed();
    }

    protected boolean doOnKeyDown(int keyCode, KeyEvent event) {
        return super.onKeyDown(keyCode, event);
    }

    protected void doOnUserLeaveHint() {
        // to implement
    }

    protected void doOnPause() {
        // to implement
    }

    protected void doOnStop() {
        // to implement
    }

    protected void doOnDestroy() {
        // to implement
    }

    protected final boolean isResume() {
        return this.mIsResume;
    }

    @Override
    @SuppressWarnings("deprecation")
    public void startActivityForResult(Intent intent, int requestCode, @Nullable Bundle options) {
        if (this instanceof TransientInitActivity) {
            StartupDirector director = MainApplicationImpl.sDirector;
            if (director != null && director.isStartup()) {
                director.setDisableInterception(true);
            }
        }
        super.startActivityForResult(intent, requestCode, options);
    }
}
