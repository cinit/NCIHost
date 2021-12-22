package cc.ioctl.nfcdevicehost.activity.base;

import android.app.Dialog;
import android.os.Bundle;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.core.view.WindowCompat;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.shape.MaterialShapeDrawable;
import com.google.android.material.shape.ShapeAppearanceModel;

import org.jetbrains.annotations.NotNull;

import cc.ioctl.nfcdevicehost.R;
import cc.ioctl.nfcdevicehost.util.ThreadManager;

public abstract class BaseBottomSheetDialogFragment extends BottomSheetDialogFragment {
    // TODO: 2021-12-22 don't use rounded corners if the bottom sheet is expanded to the status bar or the top of window
    private View mRootView = null;
    private Dialog mDialog = null;
    private BottomSheetBehavior mBehavior = null;
    private final BottomSheetBehavior.BottomSheetCallback mBottomSheetCallback = new BottomSheetBehavior.BottomSheetCallback() {
        @Override
        public void onStateChanged(@NonNull View bottomSheet, int newState) {
            if (newState == BottomSheetBehavior.STATE_EXPANDED) {
                // don't use rounded corners if the bottom sheet is expanded to the status bar or the top of the screen
                boolean isExpandedToWindowTop = mBehavior.getPeekHeight() == 0;
                if (!isExpandedToWindowTop) {
                    // In the EXPANDED STATE apply a new MaterialShapeDrawable with rounded corners
                    MaterialShapeDrawable newMaterialShapeDrawable = createMaterialShapeDrawable(bottomSheet);
                    bottomSheet.setBackground(newMaterialShapeDrawable);
                }
            }
        }

        @Override
        public void onSlide(@NonNull View bottomSheet, float slideOffset) {
            // Do nothing
        }
    };

    @NonNull
    @UiThread
    protected abstract View createRootView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                           @Nullable Bundle savedInstanceState);

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        mRootView = createRootView(inflater, container, savedInstanceState);
        return mRootView;
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        mDialog = new BottomSheetDialog(requireContext(), getTheme()) {
            @Override
            public void onAttachedToWindow() {
                super.onAttachedToWindow();
                Window window = getWindow();
                WindowCompat.setDecorFitsSystemWindows(window, false);
                WindowManager.LayoutParams lp = window.getAttributes();
                lp.gravity = Gravity.BOTTOM;
                lp.windowAnimations = R.style.BottomSheetDialogAnimation;
                window.setAttributes(lp);
                // allow the bottom sheet to extend under the system status bar
                View containerView = findViewById(com.google.android.material.R.id.container);
                if (containerView != null) {
                    containerView.setFitsSystemWindows(false);
                }
                View coordinatorView = findViewById(com.google.android.material.R.id.coordinator);
                if (coordinatorView != null) {
                    coordinatorView.setFitsSystemWindows(false);
                }
            }
        };
        return mDialog;
    }

    @Override
    public void onStart() {
        super.onStart();
        if (mBehavior == null) {
            mBehavior = BottomSheetBehavior.from((View) mRootView.getParent());
        }
        mBehavior.addBottomSheetCallback(mBottomSheetCallback);
        // update bottom sheet background with round corners
        // don't use rounded corners if the bottom sheet is expanded to the status bar or the top of the screen
        View bottomSheet = mDialog.getWindow().findViewById(com.google.android.material.R.id.design_bottom_sheet);
        boolean isExpandedToWindowTop = (mBehavior.getState() == BottomSheetBehavior.STATE_EXPANDED)
                && (mBehavior.getPeekHeight() == 0);
        if (!isExpandedToWindowTop) {
            ThreadManager.post(() -> {
                // I don't know why, but this works, it won't work if you call it directly here
                MaterialShapeDrawable newMaterialShapeDrawable = createMaterialShapeDrawable(bottomSheet);
                bottomSheet.setBackground(newMaterialShapeDrawable);
            });
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        if (mBehavior != null) {
            mBehavior.removeBottomSheetCallback(mBottomSheetCallback);
        }
    }

    @Override
    public void onDestroyView() {
        mRootView = null;
        super.onDestroyView();
    }

    @NotNull
    private MaterialShapeDrawable createMaterialShapeDrawable(@NonNull View bottomSheet) {
        // https://stackoverflow.com/questions/43852562/round-corner-for-bottomsheetdialogfragment
        // Create a ShapeAppearanceModel with the same shapeAppearanceOverlay used in the style
        ShapeAppearanceModel shapeAppearanceModel =
                ShapeAppearanceModel.builder(getContext(), 0, R.style.CustomShapeAppearanceBottomSheetDialog)
                        .build();
        //Create a new MaterialShapeDrawable (you can't use the original MaterialShapeDrawable in the BottoSheet)
        MaterialShapeDrawable currentMaterialShapeDrawable = (MaterialShapeDrawable) bottomSheet.getBackground();
        MaterialShapeDrawable newMaterialShapeDrawable = new MaterialShapeDrawable((shapeAppearanceModel));
        //Copy the attributes in the new MaterialShapeDrawable
        newMaterialShapeDrawable.initializeElevationOverlay(getContext());
        newMaterialShapeDrawable.setFillColor(currentMaterialShapeDrawable.getFillColor());
        newMaterialShapeDrawable.setTintList(currentMaterialShapeDrawable.getTintList());
        newMaterialShapeDrawable.setElevation(currentMaterialShapeDrawable.getElevation());
        newMaterialShapeDrawable.setStrokeWidth(currentMaterialShapeDrawable.getStrokeWidth());
        newMaterialShapeDrawable.setStrokeColor(currentMaterialShapeDrawable.getStrokeColor());
        return newMaterialShapeDrawable;
    }

}
