package cc.ioctl.nfcdevicehost.util;

import android.app.Activity;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Objects;

import cc.ioctl.nfcdevicehost.activity.ShadowSafTransientActivity;

public class SafUtils {

    private SafUtils() {
        throw new AssertionError("No instance for you!");
    }

    public interface SafSelectFileResultCallback {
        void onResult(@NonNull Uri uri);
    }

    public static SaveFileTransaction requestSaveFile(@NonNull Activity activity) {
        return new SaveFileTransaction(activity);
    }

    public static class SaveFileTransaction {
        private final Activity activity;
        private String defaultFileName;
        private String mineType;
        private SafSelectFileResultCallback resultCallback;
        private Runnable cancelCallback;

        private SaveFileTransaction(@NonNull Activity activity) {
            Objects.requireNonNull(activity, "activity");
            this.activity = activity;
        }

        @NonNull
        public SaveFileTransaction setDefaultFileName(@NonNull String fileName) {
            this.defaultFileName = fileName;
            return this;
        }

        @NonNull
        public SaveFileTransaction setMimeType(@NonNull String mimeType) {
            this.mineType = mimeType;
            return this;
        }

        @NonNull
        public SaveFileTransaction onResult(@NonNull SafSelectFileResultCallback callback) {
            Objects.requireNonNull(callback, "callback");
            this.resultCallback = callback;
            return this;
        }

        @NonNull
        public SaveFileTransaction onCancel(@Nullable Runnable callback) {
            this.cancelCallback = callback;
            return this;
        }

        public void commit() {
            Objects.requireNonNull(activity);
            Objects.requireNonNull(resultCallback);
            ShadowSafTransientActivity.startActivityForRequest(activity,
                    ShadowSafTransientActivity.TARGET_ACTION_CREATE_AND_WRITE,
                    mineType, defaultFileName, uri -> {
                        if (uri != null) {
                            resultCallback.onResult(uri);
                        } else {
                            if (cancelCallback != null) {
                                cancelCallback.run();
                            }
                        }
                    });
        }

    }

    public static OpenFileTransaction requestOpenFile(@NonNull Activity activity) {
        return new OpenFileTransaction(activity);
    }

    public static class OpenFileTransaction {
        private final Activity activity;
        private String mimeType;
        private SafSelectFileResultCallback resultCallback;
        private Runnable cancelCallback;

        private OpenFileTransaction(@NonNull Activity activity) {
            Objects.requireNonNull(activity, "activity");
            this.activity = activity;
        }

        @NonNull
        public OpenFileTransaction setMimeType(@NonNull String mimeType) {
            this.mimeType = mimeType;
            return this;
        }

        @NonNull
        public OpenFileTransaction onResult(@NonNull SafSelectFileResultCallback callback) {
            Objects.requireNonNull(callback, "callback");
            this.resultCallback = callback;
            return this;
        }

        @NonNull
        public OpenFileTransaction onCancel(@Nullable Runnable callback) {
            this.cancelCallback = callback;
            return this;
        }

        public void commit() {
            Objects.requireNonNull(activity);
            Objects.requireNonNull(resultCallback);
            ShadowSafTransientActivity.startActivityForRequest(activity,
                    ShadowSafTransientActivity.TARGET_ACTION_READ,
                    mimeType, null, uri -> {
                        if (uri != null) {
                            resultCallback.onResult(uri);
                        } else {
                            if (cancelCallback != null) {
                                cancelCallback.run();
                            }
                        }
                    });
        }

    }
}
