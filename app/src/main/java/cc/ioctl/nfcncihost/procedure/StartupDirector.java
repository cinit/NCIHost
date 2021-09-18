package cc.ioctl.nfcncihost.procedure;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;

import androidx.annotation.UiThread;

import java.util.HashSet;

import cc.ioctl.nfcncihost.activity.BaseActivity;
import cc.ioctl.nfcncihost.activity.splash.SplashActivity;
import cc.ioctl.nfcncihost.util.ThreadManager;

@SuppressWarnings("MapOrSetKeyShouldOverrideHashCodeEquals")
public class StartupDirector implements Runnable {
    private static final String TAG = "StartupDirector";

    private SplashActivity mSplashActivity;
    private final HashSet<BaseActivity> mSuspendedActivities = new HashSet<>();
    private volatile boolean mDisableInterception = false;
    private volatile boolean mBackgroundStepsDone = false;
    private volatile boolean mForegroundStartupFinished = false;
    private volatile boolean mIsShowingSplash = false;
    private volatile boolean mIsForeground = false;
    private volatile boolean mStartSplashRequested = false;

    public static StartupDirector onAppStart() {
        StartupDirector director = new StartupDirector();
        ThreadManager.execute(director);
        return director;
    }

    @SuppressWarnings("unused")
    public boolean onActivityCreate(Activity activity, Intent intent) {
        if (mForegroundStartupFinished || mDisableInterception) {
            return false;
        }
        if (activity instanceof BaseActivity) {
            mIsForeground = true;
            if (activity instanceof SplashActivity) {
                mSplashActivity = (SplashActivity) activity;
                if (mBackgroundStepsDone) {
                    mIsShowingSplash = true;
                    return false;
                } else {
                    mSplashActivity.showSplash();
                    mIsShowingSplash = true;
                }
            } else {
                mSuspendedActivities.add((BaseActivity) activity);
                if (!mIsShowingSplash) {
                    startSplashActivity(activity);
                }
            }
            return true;
        }
        return false;
    }

    @Override
    public void run() {
        if (!mBackgroundStepsDone) {
            int[] stepIds = getTargetSteps();
            for (int id : stepIds) {
                Step step = StepFactory.getStep(id);
                step.step();
            }
            mBackgroundStepsDone = true;
            ThreadManager.post(this::onBackgroundStepsEnd);
        }
    }

    @UiThread
    private void onBackgroundStepsEnd() {
        mBackgroundStepsDone = true;
        if (mIsForeground) {
            if (mSplashActivity != null) {
                mSplashActivity.callOnCreateProc();
            } else {
                if (!mSuspendedActivities.isEmpty()) {
                    startSplashActivity(mSuspendedActivities.iterator().next());
                }
            }
        }
    }

    private int[] getTargetSteps() {
        return new int[]{
                StepFactory.STEP_LOAD_NATIVE_LIBS,
                StepFactory.STEP_LOAD_CONFIG,
        };
    }

    public boolean isStartup() {
        return !mForegroundStartupFinished;
    }

    public void setDisableInterception(boolean disableInterception) {
        this.mDisableInterception = disableInterception;
    }

    private void startSplashActivity(Activity activity) {
        if (!mStartSplashRequested) {
            Intent splash = new Intent(activity, SplashActivity.class);
            splash.putExtra(SplashActivity.TAG_SHADOW_SPLASH, true);
            activity.startActivity(splash);
            mStartSplashRequested = true;
        }
    }

    @UiThread
    public void cancelAllPendingActivity() {
        mStartSplashRequested = false;
        if (!mForegroundStartupFinished) {
            for (BaseActivity activity : mSuspendedActivities) {
                activity.finish();
            }
            mSuspendedActivities.clear();
        }
        mIsShowingSplash = false;
        mIsForeground = false;
    }

    @UiThread
    public void onForegroundStartupFinish() {
        mStartSplashRequested = false;
        if (!mForegroundStartupFinished) {
            Log.d(TAG, "onForegroundStartupFinish: execute");
            mBackgroundStepsDone = true;
            mForegroundStartupFinished = true;
            mSplashActivity = null;
            for (BaseActivity activity : mSuspendedActivities) {
                activity.callOnCreateProc();
            }
            mSuspendedActivities.clear();
        }
    }
}
