// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import android.accounts.Account;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.database.Cursor;
import android.location.LocationManager;
import android.net.Uri;
import android.util.Log;

import com.google.android.apps.chrome.preferences.ChromeNativePreferences;
import com.google.android.apps.chrome.sync.SyncStatusHelper;

import org.chromium.base.CalledByNative;

/**
 * Helper class to read and write the 'Use My Location' setting used by Google Apps (e.g. GoogleQSB,
 * VoiceSearch).
 *
 * This class duplicates a small amount of functionality from GSF (Google Services Framework) to
 * allow the open source Settings app to interface to the 'Use My Location' setting owned by GSF.
 */
public final class GoogleLocationSettingsHelper {

    private static final String TAG = "GoogleLocationSettingHelper";

    /**
     * User has disagreed to use location for Google services.
     */
    private static final int USE_LOCATION_FOR_SERVICES_OFF = 0;

    /**
     * User has agreed to use location for Google services.
     */
    private static final int USE_LOCATION_FOR_SERVICES_ON = 1;

    /**
     * The user has neither agreed nor disagreed to use location for Google services yet.
     */
    private static final int USE_LOCATION_FOR_SERVICES_NOT_SET = 2;

    private static final String GOOGLE_SETTINGS_AUTHORITY = "com.google.settings";
    private static final Uri GOOGLE_SETTINGS_CONTENT_URI =
            Uri.parse("content://" + GOOGLE_SETTINGS_AUTHORITY + "/partner");
    private static String NAME = "name";
    private static String VALUE = "value";
    private static final String USE_LOCATION_FOR_SERVICES = "use_location_for_services";

    private static final String ACTION_GOOGLE_LOCATION_SETTINGS =
            "com.google.android.gsf.GOOGLE_LOCATION_SETTINGS";
    private static final String ACTION_GOOGLE_APPS_LOCATION_SETTINGS =
            "com.google.android.gsf.GOOGLE_APPS_LOCATION_SETTINGS";

    private static Context sApplicationContext;

    /**
     * Determine if Google apps need to conform to the USE_LOCATION_FOR_SERVICES setting.
     */
    public static boolean isEnforceable() {
        ResolveInfo ri = sApplicationContext.getPackageManager().resolveActivity(
                new Intent(ACTION_GOOGLE_APPS_LOCATION_SETTINGS),
                PackageManager.MATCH_DEFAULT_ONLY);
        return ri != null;
    }

    /**
     * Determine if the 'Use My Location' setting is applicable on this device, i.e. if the
     * activity used to enabled/disable it is present.
     */
    private static boolean isGoogleLocationSettingsActionIntentAvailable() {
        ResolveInfo ri = sApplicationContext.getPackageManager().resolveActivity(
                new Intent(ACTION_GOOGLE_LOCATION_SETTINGS), PackageManager.MATCH_DEFAULT_ONLY);
        return ri != null;
    }

    /**
     * Returns the intent to start the Google Location Settings actvity.
     * @return Intent
     */
    public static Intent getGoogleLocationSettingsIntent() {
        Intent i = new Intent(ACTION_GOOGLE_LOCATION_SETTINGS);
        i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Account account = SyncStatusHelper.get(sApplicationContext).getSignedInUser();
        if (account != null) {
            i.putExtra("account", account.name);
        }
        return i;
    }
    /**
     * Start the Google Location Settings Activity from the infobar.
     */
    @CalledByNative
    public static void startGoogleLocationSettingsActivity() {
        sApplicationContext.startActivity(getGoogleLocationSettingsIntent());
    }

    /**
     * Get the current value for the 'Use value for location' setting.
     * @return One of {@link #USE_LOCATION_FOR_SERVICES_NOT_SET},
     *      {@link #USE_LOCATION_FOR_SERVICES_OFF} or {@link #USE_LOCATION_FOR_SERVICES_ON}.
     */
    private static int getUseLocationForServices() {
        ContentResolver resolver = sApplicationContext.getContentResolver();
        Cursor c = null;
        String stringValue = null;
        try {
            c = resolver.query(GOOGLE_SETTINGS_CONTENT_URI, new String[] { VALUE }, NAME + "=?",
                    new String[] { USE_LOCATION_FOR_SERVICES }, null);
            if (c != null && c.moveToNext()) {
                stringValue = c.getString(0);
            }
        } catch (RuntimeException e) {
            Log.w(TAG, "Failed to get 'Use My Location' setting", e);
        } finally {
            if (c != null) {
                c.close();
            }
        }
        if (stringValue == null) {
            return USE_LOCATION_FOR_SERVICES_NOT_SET;
        }
        try {
            return Integer.parseInt(stringValue);
        } catch (NumberFormatException nfe) {
            return USE_LOCATION_FOR_SERVICES_NOT_SET;
        }
    }

    /**
     * Returns if the Master System Location setting is enabled.
     * @return boolean value of the Master System Location setting.
     */
    private static boolean isMasterLocationProviderEnabled() {
        LocationManager locationManager =
                (LocationManager) sApplicationContext.getSystemService(Context.LOCATION_SERVICE);
        return (locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER)
                || locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER));
    }

    /**
     * Set the application context static variable.
     * @param context Current context
     */
    public static void initialize(Context context) {
        sApplicationContext = context.getApplicationContext();
    }

    /**
     * Update the enable location boolean value based on the current system master location setting
     * on Main Activity onResume().
     */
    public static void onMainActivityResume() {
        if (!isGoogleLocationSettingsAvailable()) return;
        boolean enableLocation = isMasterLocationProviderEnabled();
        ChromeNativePreferences.getInstance().setAllowLocationEnabled(enableLocation);
    }

    /**
     * Return the infobar button text - Allow or Google Location Settings.
     * @return infobar accept button text.
     */
    @CalledByNative
    public static String getInfoBarAllowTextFromLocationSetting() {
        if (isGoogleLocationSettingsAvailable() &&
                isMasterLocationProviderEnabled() &&
                getUseLocationForServices() == USE_LOCATION_FOR_SERVICES_OFF) {
            return sApplicationContext.getResources().getString(R.string.preferences);
        }
        return sApplicationContext.getResources().getString(R.string.allow_exception_button);
    }

    @CalledByNative
    public static boolean isGoogleAppsLocationSettingEnabled() {
        if (isGoogleLocationSettingsAvailable()) {
            return getUseLocationForServices() == USE_LOCATION_FOR_SERVICES_ON;
        }
        return true;
    }

    /**
     * Return if the google location setting is available.
     * @return boolean
     */
    public static boolean isGoogleLocationSettingsAvailable() {
        if (isEnforceable() && isGoogleLocationSettingsActionIntentAvailable() &&
                getUseLocationForServices() != USE_LOCATION_FOR_SERVICES_NOT_SET) {
            return true;
        }
        return false;
    }
}
