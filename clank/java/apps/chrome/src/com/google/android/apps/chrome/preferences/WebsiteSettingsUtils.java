/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.android.apps.chrome.preferences;

import org.chromium.base.CalledByNative;

import com.google.android.apps.chrome.R;

import android.content.Context;
import android.util.Log;

import java.io.Serializable;
import java.util.ArrayList;
import java.text.DecimalFormat;
import java.util.HashMap;
import java.util.List;

/**
 * Utility class that interacts with native to retrieve and set website settings.
 */
public abstract class WebsiteSettingsUtils {
    private static final String LOG_TAG = "WebsiteSettingsUtils";

    public static class GeolocationInfo implements Serializable {
        private String mOrigin;
        private String mEmbedder;

        @CalledByNative("GeolocationInfo")
        public void setInfo(String origin, String embedder) {
            mOrigin = origin;
            mEmbedder = embedder;
        }

        public String getOrigin() {
            return mOrigin;
        }

        public String getEmbedder() {
            return mEmbedder;
        }

        private String getEmbedderSafe() {
            return mEmbedder != null ? mEmbedder : mOrigin;
        }

        public Boolean getAllowed() {
            return WebsiteSettingsUtils.nativeGetGeolocationSettingForOrigin(
                mOrigin,getEmbedderSafe());
        }

        public void setAllowed(Boolean value) {
            WebsiteSettingsUtils.nativeSetGeolocationSettingForOrigin(
                mOrigin, getEmbedderSafe(), value);
        }
    }

    public static class PopupExceptionInfo implements Serializable {
        private String mPattern;

        public PopupExceptionInfo(String pattern) {
            mPattern = pattern;
        }

        public String getPattern() {
            return mPattern;
        }

        public Boolean getAllowed() {
            String setting =
                    ChromeNativePreferences.getInstance().getPopupExceptionSettingFromPattern(mPattern);
            Boolean allowed = null;
            if (setting.equals(ChromeNativePreferences.EXCEPTION_SETTING_ALLOW))
                allowed = true;
            else if (setting.equals(ChromeNativePreferences.EXCEPTION_SETTING_BLOCK))
                allowed = false;
            return allowed;
        }

        public void setAllowed(Boolean value) {
            if (value != null)
                ChromeNativePreferences.getInstance().setPopupException(mPattern, value.booleanValue());
            else
                ChromeNativePreferences.getInstance().removePopupException(mPattern);
        }
    }

    public static class LocalStorageInfo implements Serializable {
        private String mFilePath;
        private long mSize;

        @CalledByNative("LocalStorageInfo")
        public void setInfo(String filePath, long size) {
            mFilePath = filePath;
            mSize = size;
        }

        public void clear() {
            WebsiteSettingsUtils.nativeClearLocalStorageData(mFilePath);
        }

        public long getSize() {
            return mSize;
        }
    }

    public static class StorageInfo implements Serializable {
        private String mHost;
        private int mType;
        private long mSize;

        @CalledByNative("StorageInfo")
        public void setInfo(String host, int type, long size) {
            mHost = host;
            mType = type;
            mSize = size;
        }

        public String getHost() {
            return mHost;
        }

        public void clear(StorageInfoClearedCallback callback) {
            WebsiteSettingsUtils.nativeClearStorageData(mHost, mType, callback);
        }

        public long getSize() {
            return mSize;
        }
    }

    public interface LocalStorageInfoReadyCallback {
        @CalledByNative("LocalStorageInfoReadyCallback")
        public void onLocalStorageInfoReady(HashMap map);
    }

    public interface StorageInfoReadyCallback {
        @CalledByNative("StorageInfoReadyCallback")
        public void onStorageInfoReady(ArrayList array);
    }

    public interface StorageInfoClearedCallback {
        @CalledByNative("StorageInfoClearedCallback")
        public void onStorageInfoCleared();
    }

    @SuppressWarnings("unchecked")
    public static List<GeolocationInfo> getGeolocationInfo() {
        return nativeGetGeolocationOrigins();
    }

    public static List<PopupExceptionInfo> getPopupExceptionInfo() {
        HashMap<String, String>[] origins = ChromeNativePreferences.getInstance().getPopupExceptions();
        List<PopupExceptionInfo> infos = new ArrayList<PopupExceptionInfo>();
        if (origins != null) {
            for (HashMap<String, String> exception : origins) {
                infos.add(new PopupExceptionInfo(
                    exception.get(ChromeNativePreferences.EXCEPTION_DISPLAY_PATTERN)));
            }
        }
        return infos;
    }

    public static void fetchLocalStorageInfo(LocalStorageInfoReadyCallback callback) {
        nativeFetchLocalStorageInfo(callback);
    }

    public static void fetchStorageInfo(StorageInfoReadyCallback callback) {
        nativeFetchStorageInfo(callback);
    }

    /**
     * Converts storage usage into level 0..2 according to the following scale:
     * [0..0.1MB] -> 0
     * (0.1..5MB] -> 1`
     * (5MB..     -> 2
     */
    public static int getStorageUsageLevel(long usageInBytes) {
        float usageInMegabytes = usageInBytes / (1024.0F * 1024.0F);
        if (usageInMegabytes <= 0.1) {
            return 0;
        } else if (usageInMegabytes <= 5) {
            return 1;
        }
        return 2;
    }

    public static String sizeValueToString(Context context, long bytes) {
        final String label[] = {
          context.getString(R.string.origin_settings_storage_bytes),
          context.getString(R.string.origin_settings_storage_kbytes),
          context.getString(R.string.origin_settings_storage_mbytes),
          context.getString(R.string.origin_settings_storage_gbytes),
          context.getString(R.string.origin_settings_storage_tbytes),
        };


        if (bytes <= 0) {
            Log.e(LOG_TAG, "sizeValueToString called with non-positive value: " + bytes);
            return "0" + label[0];
        }

        int i = 0;
        float size = bytes;
        for (i=0; i<label.length; ++i) {
            if (size < 1024 || i == label.length - 1)
                break;
            size /= 1024.0F;
        }

        DecimalFormat formatter = new DecimalFormat("#.##");
        return formatter.format(size) + label[i];
    }

    private static native ArrayList nativeGetGeolocationOrigins();
    private static native Boolean nativeGetGeolocationSettingForOrigin(String origin, String embedder);
    private static native void nativeSetGeolocationSettingForOrigin(String origin, String embedder, Boolean value);
    private static native void nativeClearLocalStorageData(String path);
    private static native void nativeClearStorageData(String origin, int type, Object callback);
    private static native void nativeFetchLocalStorageInfo(Object callback);
    private static native void nativeFetchStorageInfo(Object callback);
}
