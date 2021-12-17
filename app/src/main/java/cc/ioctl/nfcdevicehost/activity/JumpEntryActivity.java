package cc.ioctl.nfcdevicehost.activity;

import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Nullable;

import cc.ioctl.nfcdevicehost.activity.base.BaseActivity;

/**
 * External file share/browser springboard entry point.
 * This activity is short-lived and only used to launch a target activity.
 * This is a transient Activity, and will be finished immediately after launching the target activity.
 */
public class JumpEntryActivity extends BaseActivity {

    @Override
    protected boolean doOnCreate(@Nullable Bundle savedInstanceState) {
        Intent intent = getIntent();
        String action = intent.getAction();
        if (Intent.ACTION_VIEW.equals(action)) {
            Intent targetIntent = new Intent(intent);
            targetIntent.setComponent(new ComponentName(this, SidebandHostActivity.class));
            startActivity(targetIntent);
        }
        finish();
        return false;
    }

    @Override
    protected boolean shouldRetainActivitySavedInstanceState() {
        return false;
    }
}
