package cc.ioctl.nfcncihost.procedure;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;

import androidx.annotation.UiThread;

import java.util.HashSet;

import cc.ioctl.nfcncihost.activity.BaseActivity;
import cc.ioctl.nfcncihost.activity.ui.startup.TransientInitActivity;
import cc.ioctl.nfcncihost.util.ThreadManager;

/**
 * The startup procedure of the application.
 * The {@link TransientInitActivity} is started in the startup procedure.
 * Startup procedures are executed in the following order:
 * 1. The background tasks, eg. loading the shared library, initializing the MMKV, etc.
 * 2. The foreground tasks, eg. showing the @{@link cc.ioctl.nfcncihost.activity.ui.startup.WarnFragment}
 * After the startup procedure is finished, the application will start the target activity which the intent requires.
 * If no activity is required, the application will start the default activity like android.intent.action.MAIN.
 *
 * @author cinit
 */
@SuppressWarnings("MapOrSetKeyShouldOverrideHashCodeEquals")
public class StartupDirector implements Runnable {
    private static final String TAG = "StartupDirector";

    private TransientInitActivity mTransientInitActivity;
    private final HashSet<BaseActivity> mSuspendedActivities = new HashSet<>();
    private volatile boolean mDisableInterception = false;
    private volatile boolean mBackgroundStepsDone = false;
    private volatile boolean mForegroundStartupFinished = false;
    private volatile boolean mIsInitActivityStarted = false;
    private volatile boolean mIsForeground = false;
    private volatile boolean mStartInitActivityRequested = false;

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
            if (activity instanceof TransientInitActivity) {
                mTransientInitActivity = (TransientInitActivity) activity;
                if (mBackgroundStepsDone) {
                    mIsInitActivityStarted = true;
                    return false;
                } else {
                    mTransientInitActivity.showSplash();
                    mIsInitActivityStarted = true;
                }
            } else {
                mSuspendedActivities.add((BaseActivity) activity);
                if (!mIsInitActivityStarted) {
                    startInitActivity(activity);
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
            if (mTransientInitActivity != null) {
                mTransientInitActivity.callOnCreateProcInternal();
            } else {
                if (!mSuspendedActivities.isEmpty()) {
                    startInitActivity(mSuspendedActivities.iterator().next());
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

    private void startInitActivity(Activity activity) {
        if (!mStartInitActivityRequested) {
            Intent intent = new Intent(activity, TransientInitActivity.class);
            intent.putExtra(TransientInitActivity.TAG_HAS_ACTIVITY_ON_STACK, true);
            activity.startActivity(intent);
            mStartInitActivityRequested = true;
        }
    }

    @UiThread
    public void cancelAllPendingActivity() {
        mStartInitActivityRequested = false;
        if (!mForegroundStartupFinished) {
            for (BaseActivity activity : mSuspendedActivities) {
                activity.finish();
            }
            mSuspendedActivities.clear();
        }
        mIsInitActivityStarted = false;
        mIsForeground = false;
    }

    @UiThread
    public void onForegroundStartupFinish() {
        mStartInitActivityRequested = false;
        if (!mForegroundStartupFinished) {
            Log.d(TAG, "onForegroundStartupFinish: execute");
            mBackgroundStepsDone = true;
            mForegroundStartupFinished = true;
            mTransientInitActivity = null;
            for (BaseActivity activity : mSuspendedActivities) {
                activity.callOnCreateProcInternal();
            }
            mSuspendedActivities.clear();
        }
    }
}
