package cc.ioctl.nfcncihost.util;

import android.annotation.SuppressLint;
import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;

import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ThreadManager {
    private static final ExecutorService sExecutor = Executors.newCachedThreadPool();

    public static void execute(@NonNull Runnable r) {
        sExecutor.execute(r);
    }

    private static Handler sHandler;

    private ThreadManager() {
        throw new AssertionError("No instance for you!");
    }

    @SuppressLint("LambdaLast")
    public static void postDelayed(@NonNull Runnable r, long ms) {
        Objects.requireNonNull(r);
        if (sHandler == null) {
            sHandler = new Handler(Looper.getMainLooper());
        }
        sHandler.postDelayed(r, ms);
    }

    //kotlin friendly?
    public static void postDelayed(long ms, @NonNull Runnable r) {
        postDelayed(r, ms);
    }

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
