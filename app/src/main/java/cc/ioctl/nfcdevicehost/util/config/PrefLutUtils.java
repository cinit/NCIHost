package cc.ioctl.nfcdevicehost.util.config;

import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;

public class PrefLutUtils {

    @Nullable
    public static IConfigItem<?> findConfigByFullName(String cfgName) {
        if (cfgName == null) {
            return null;
        }
        String groupName = null;
        String keyName;
        if (cfgName.contains(":")) {
            String[] cfgNameParts = cfgName.split(":");
            groupName = cfgNameParts[0];
            keyName = cfgNameParts[1];
        } else {
            keyName = cfgName;
        }
        if (groupName == null) {
            groupName = "_default";
        }
        Class<?> groupClass = findGroupClassByName(groupName);
        if (groupClass == null) {
            return null;
        }
        return findConfigItemInGroupByName(groupClass, keyName);
    }

    @Nullable
    public static Class<?> findGroupClassByName(String groupName) {
        if (groupName == null) {
            return null;
        }
        if (groupName.equals("core")) {
            return GenericConfig.class;
        }
        return null;
    }

    @Nullable
    public static IConfigItem<?> findConfigItemInGroupByName(@NonNull Class<?> groupClass, @NonNull String keyName) {
        Field field;
        try {
            field = groupClass.getDeclaredField(keyName);
        } catch (NoSuchFieldException e) {
            return null;
        }
        // public static
        if (!field.isSynthetic() && ((field.getModifiers() & (Modifier.PUBLIC | Modifier.STATIC))
                == (Modifier.PUBLIC | Modifier.STATIC))
                && field.getName().equals(keyName)
                && IConfigItem.class.isAssignableFrom(field.getType())) {
            try {
                return (IConfigItem<?>) field.get(null);
            } catch (IllegalAccessException e) {
                // should not happen
                Log.e("PrefLutUtils", "findConfigItemInGroupByName: " + e.getMessage());
            }
        }
        return null;
    }
}
