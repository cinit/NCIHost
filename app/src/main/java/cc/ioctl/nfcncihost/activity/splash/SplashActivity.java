package cc.ioctl.nfcncihost.activity.splash;

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
 * Activity for
 * 1. StartupDirector
 * 2. EULA
 * 3. Ask for permissions.
 * 4. What's new.
 **/
public class SplashActivity extends BaseActivity {

    public static final String TAG_SHADOW_SPLASH = SplashActivity.class.getName() + ".TAG_SHADOW_SPLASH";
    private Iterator<AbsInteractiveStepFragment> steps;
    private Fragment currentFragment;
    private Bundle mSavedFragmentsInstance = null;
    private static final String SAVED_FRAGMENT_DATA = "SAVED_FRAGMENT_DATA";

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

    @UiThread
    private void finishInteractiveStartup() {
        StartupDirector director = MainApplicationImpl.sDirector;
        if (director != null) {
            director.onForegroundStartupFinish();
        }
        Intent intent = getIntent();
        if (!intent.hasExtra(TAG_SHADOW_SPLASH)) {
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

    @SuppressLint("SetTextI18n")
    @UiThread
    public void showSplash() {
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.hide();
        }
        setContentView(R.layout.activity_splash);
        TextView tvVersion = findViewById(R.id.splash_version);
        tvVersion.setText("version " + BuildConfig.VERSION_NAME + "-"
                + (BuildConfig.DEBUG ? "debug" : "release"));
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
        protected SplashActivity activity;

        @Override
        public void onAttach(@NonNull Context context) {
            activity = (SplashActivity) context;
            super.onAttach(context);
        }

        @Override
        public int compareTo(AbsInteractiveStepFragment o) {
            return this.getOrder() - o.getOrder();
        }
    }

}
