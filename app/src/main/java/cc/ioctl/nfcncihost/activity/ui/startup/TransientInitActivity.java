package cc.ioctl.nfcncihost.activity.ui.startup;

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.appcompat.app.ActionBar;
import androidx.fragment.app.Fragment;

import com.tencent.mmkv.MMKV;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;

import cc.ioctl.nfcncihost.BuildConfig;
import cc.ioctl.nfcncihost.R;
import cc.ioctl.nfcncihost.activity.BaseActivity;
import cc.ioctl.nfcncihost.activity.DashboardActivity;
import cc.ioctl.nfcncihost.activity.config.ConfigManager;
import cc.ioctl.nfcncihost.procedure.MainApplicationImpl;
import cc.ioctl.nfcncihost.procedure.StartupDirector;

/**
 * The start-up initialization Activity for
 * 1. StartupDirector
 * 2. EULA
 * 3. Ask for permissions.
 * 4. What's new.
 * <p>
 * Note that this Activity does **NOT** save state, all saved state is removed on onCreate.
 *
 * @author cinit
 **/
public class TransientInitActivity extends BaseActivity {
    public static final String TAG_HAS_ACTIVITY_ON_STACK = TransientInitActivity.class.getName() + ".TAG_HAS_ACTIVITY_ON_STACK";
    private Iterator<AbsInteractiveStepFragment> steps;
    private Fragment currentFragment;
    private Bundle mSavedFragmentsInstance = null;
    private static final String SAVED_FRAGMENT_DATA = TransientInitActivity.class.getName() + ".SAVED_FRAGMENT_DATA";

    /**
     * Called by the {@link StartupDirector} when the background startup is finished.
     * This is the beginning of the interactive startup.
     *
     * @param savedInstanceState the saved instance state
     * @return always true
     */
    @Override
    protected boolean doOnCreate(@Nullable Bundle savedInstanceState) {
        super.doOnCreate(null);
        if (savedInstanceState != null) {
            mSavedFragmentsInstance = savedInstanceState.getBundle(SAVED_FRAGMENT_DATA);
        }
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.show();
        }
        steps = collectInteractiveSteps().iterator();
        if (!steps.hasNext()) {
            finishInteractiveStartup();
            return false;
        }
        setContentView(R.layout.empty_layout);
        switchToNextStep();
        return true;
    }

    @Override
    protected void doOnPostCreate(Bundle bundle) {
        super.doOnPostCreate(bundle);
    }

    @UiThread
    private void finishInteractiveStartup() {
        StartupDirector director = MainApplicationImpl.sDirector;
        if (director != null) {
            director.onForegroundStartupFinish();
        }
        Intent intent = getIntent();
        if (!intent.hasExtra(TAG_HAS_ACTIVITY_ON_STACK)) {
            Intent target = new Intent(intent);
            target.setComponent(new ComponentName(this, DashboardActivity.class));
            startActivity(target);
        }
        finish();
    }

    @Override
    protected void doOnSaveInstanceState(Bundle outState) {
        super.doOnSaveInstanceState(outState);
        if (currentFragment != null) {
            Fragment.SavedState ss = getSupportFragmentManager()
                    .saveFragmentInstanceState(currentFragment);
            if (ss != null) {
                Bundle bundle = new Bundle();
                bundle.putParcelable(currentFragment.getClass().getName(), ss);
                outState.putBundle(SAVED_FRAGMENT_DATA, bundle);
            }
        }
    }

    @Override
    public void onBackPressed() {
        doOnBackPressed();
    }

    @Override
    protected boolean shouldRetainActivitySavedInstanceState() {
        // drop all saved state for framework, we will handle it ourselves on doOnCreate
        return false;
    }

    /**
     * Show the splash screen.
     * Note that this method is called before {@link #doOnCreate(Bundle)} by the {@link StartupDirector}.
     * The splash screen is shown when the {@link StartupDirector} is executing background initialization tasks.
     * When StartupDirector finishes background initialization, it will call {@link #doOnCreate(Bundle)},
     * and this time this activity will run interactive startup.
     */
    @SuppressLint("SetTextI18n")
    @UiThread
    public void showSplash() {
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.hide();
        }
        setContentView(R.layout.activity_splash);
        TextView tvVersion = findViewById(R.id.splash_version);
        tvVersion.setText("version " + BuildConfig.VERSION_NAME + "-" + (BuildConfig.DEBUG ? "debug" : "release"));
    }

    @UiThread
    void switchToNextStep() {
        if (!steps.hasNext()) {
            currentFragment = null;
            finishInteractiveStartup();
        } else {
            currentFragment = steps.next();
            getSupportFragmentManager().beginTransaction()
                    .replace(R.id.emptyLayout_main, currentFragment)
                    .commit();
        }
    }

    private ArrayList<AbsInteractiveStepFragment> collectInteractiveSteps() {
        ArrayList<AbsInteractiveStepFragment> candidates = new ArrayList<>();
//        candidates.add(new InfoFragment());
        candidates.add(new WarnFragment());
        ArrayList<AbsInteractiveStepFragment> fragments = new ArrayList<>();
        for (AbsInteractiveStepFragment f : candidates) {
            if (!f.isDone()) {
                if (mSavedFragmentsInstance != null) {
                    Fragment.SavedState ss = (Fragment.SavedState)
                            mSavedFragmentsInstance.get(f.getClass().getName());
                    if (ss != null) {
                        f.setInitialSavedState(ss);
                    }
                }
                fragments.add(f);
            }
        }
        Collections.sort(fragments, AbsInteractiveStepFragment::compareTo);
        return fragments;
    }

    public void abortInteractiveStartup() {
        StartupDirector director = MainApplicationImpl.sDirector;
        if (director != null) {
            director.cancelAllPendingActivity();
        }
        finish();
    }

    public interface InteractiveStep {
        int getOrder();

        boolean isDone();
    }

    public abstract static class AbsInteractiveStepFragment extends Fragment implements InteractiveStep, Comparable<AbsInteractiveStepFragment> {
        protected MMKV config = ConfigManager.getDefaultConfig();
        protected TransientInitActivity activity;

        @Override
        public void onAttach(@NonNull Context context) {
            activity = (TransientInitActivity) context;
            super.onAttach(context);
        }

        @Override
        public int compareTo(AbsInteractiveStepFragment o) {
            return this.getOrder() - o.getOrder();
        }
    }

}
