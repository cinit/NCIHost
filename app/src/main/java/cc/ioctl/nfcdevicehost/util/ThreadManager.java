package cc.ioctl.nfcdevicehost.util;

import android.annotation.SuppressLint;
import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;

import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class ThreadManager {
    private static final ExecutorService sExecutor = new ThreadPoolExecutor(2, 64, 60, TimeUnit.SECONDS, new SynchronousQueue<>());

    /**
     * Executes the given runnable on a background thread.
     *
     * @param r The runnable to execute.
     */
    public static void execute(@NonNull Runnable r) {
        sExecutor.execute(r);
    }

    /**
     * Executes the given runnable on a background thread.
     *
     * @param r The runnable to execute.
     */
    public static void async(@NonNull Runnable r) {
        sExecutor.execute(r);
    }

    private static Handler sHandler;

    private ThreadManager() {
        throw new AssertionError("No instance for you!");
    }

    /**
     * Post the given runnable to the UI thread.
     *
     * @param r           The runnable to post.
     * @param delayMillis The delay in milliseconds.
     */
    @SuppressLint("LambdaLast")
    public static void postDelayed(@NonNull Runnable r, long delayMillis) {
        Objects.requireNonNull(r);
        if (sHandler == null) {
            sHandler = new Handler(Looper.getMainLooper());
        }
        sHandler.postDelayed(r, delayMillis);
    }

    /**
     * Post the given runnable to the UI thread.
     * Same as {@link #postDelayed(Runnable, long)}
     * Kotlin friendly version.
     */
    public static void postDelayed(long delayMillis, @NonNull Runnable r) {
        postDelayed(r, delayMillis);
    }

    /**
     * Same as {@link android.app.Activity#runOnUiThread(Runnable)}
     *
     * @param r The runnable to run.
     */
    public static void runOnUiThread(@NonNull Runnable r) {
        Objects.requireNonNull(r);
        if (Looper.getMainLooper().getThread() == Thread.currentThread()) {
            r.run();
        } else {
            postDelayed(r, 0);
        }
    }

    /**
     * Post the given runnable to the UI thread.
     *
     * @param r The runnable to post.
     */
    public static void post(@NonNull Runnable r) {
        postDelayed(r, 0L);
    }

    public static void join(@NonNull final Runnable r) {
        Objects.requireNonNull(r);
        if (Looper.myLooper() == Looper.getMainLooper()) {
            r.run();
        } else {
            final Runnable r2 = new Runnable() {
                @Override
                public void run() {
                    synchronized (this) {
                        try {
                            r.run();
                        } finally {
                            this.notifyAll();
                        }
                    }
                }
            };
            synchronized (r2) {
                post(r2);
                try {
                    r2.wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
        }
    }

    public static boolean isUiThread() {
        return Looper.myLooper() == Looper.getMainLooper();
    }

}
