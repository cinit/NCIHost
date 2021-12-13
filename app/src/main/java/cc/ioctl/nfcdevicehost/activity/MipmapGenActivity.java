package cc.ioctl.nfcdevicehost.activity;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.drawable.AdaptiveIconDrawable;
import android.os.Build;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Field;

import cc.ioctl.nfcdevicehost.R;

@RequiresApi(api = Build.VERSION_CODES.O)
public class MipmapGenActivity extends BaseActivity {

    private static final int[] sTargetPx = new int[]{48, 72, 96, 144, 192};
    private static final String[] sTargetName = new String[]{"mdpi", "hdpi", "xhdpi", "xxhdpi", "xxxhdpi",};

    @Override
    protected boolean doOnCreate(@Nullable Bundle savedInstanceState) {
        super.doOnCreate(savedInstanceState);
        setContentView(R.layout.gen_mipmap_activity);
        return true;
    }


    public void onGenBtnClick(View view) throws IOException, NoSuchFieldException, IllegalAccessException {
        AdaptiveIconDrawable iconDrawable = (AdaptiveIconDrawable) getDrawable(R.mipmap.ic_launcher);
        Field fMaks = AdaptiveIconDrawable.class.getDeclaredField("sMask");
        fMaks.setAccessible(true);
        Path path = new Path();
        path.addRoundRect(0, 0, 100, 100, 18, 18, Path.Direction.CW);
//        fMaks.set(iconDrawable, path);
        for (int i = 0; i < sTargetName.length; i++) {
            int px = sTargetPx[i];
            String dirName = "mipmap-" + sTargetName[i];
            iconDrawable.setBounds(0, 0, px, px);
            Bitmap bm = Bitmap.createBitmap(px, px, Bitmap.Config.ARGB_8888);
            iconDrawable.draw(new Canvas(bm));
            File f = new File(getFilesDir(), dirName + "/ic_launcher_round.png");
            if (!f.exists()) {
                File p = f.getParentFile();
                if (!p.exists()) {
                    p.mkdirs();
                }
                f.createNewFile();
            }
            FileOutputStream fout = new FileOutputStream(f);
            bm.compress(Bitmap.CompressFormat.PNG, 0, fout);
            fout.flush();
            fout.close();
        }

    }
}
