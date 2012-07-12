// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.sync;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Activity;
import android.net.Uri;
import android.net.http.AndroidHttpClient;
import android.util.Log;

import com.google.android.apps.chrome.AccountManagerHelper;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.util.EntityUtils;

/**
 * This class handles pre login'ing a user into Google services.
 *
 * For more information, see:
 * {@link https://wiki.corp.google.com/twiki/bin/view/Main/GaiaProgrammaticLoginV2}
 */
public class GoogleAutoLogin implements Runnable {

    private static final String TAG = GoogleAutoLogin.class.getName();

    private static final Uri ISSUE_AUTH_TOKEN_URL = Uri.parse(
            "https://www.google.com/accounts/IssueAuthToken?service=gaia&Session=false");

    private static final Uri TOKEN_AUTH_URL = Uri.parse(
            "https://www.google.com/accounts/TokenAuth");

    private final Activity mActivity;
    private final Account mAccount;
    private String mSid;
    private String mLsid;
    private boolean mRetried;

    public GoogleAutoLogin(Activity activity, Account account) {
        mActivity = activity;
        mAccount = account;
    }

    public void start() {
        getAutoLoginSID();
    }

    private void getAutoLoginSID() {
        AccountManagerHelper.get(mActivity).getAuthTokenFromForeground(
                mActivity, mAccount, "SID",
                new AccountManagerHelper.GetAuthTokenCallback() {
                    @Override
                    public void tokenAvailable(String authToken) {
                        mSid = authToken;
                        getAutoLoginLSID();
                    }
                });
    }

    private void getAutoLoginLSID() {
        AccountManagerHelper.get(mActivity).getAuthTokenFromForeground(
                mActivity, mAccount, "LSID",
                new AccountManagerHelper.GetAuthTokenCallback() {
                    @Override
                    public void tokenAvailable(String authToken) {
                        mLsid = authToken;
                        doLogin();
                    }
                });
    }

    private void retry() {
        AccountManager am = AccountManager.get(mActivity);
        if (mSid != null) {
            am.invalidateAuthToken("com.google", mSid);
        }
        if (mLsid != null) {
            am.invalidateAuthToken("com.google", mLsid);
        }

        // Allow only one retry
        if (!mRetried) {
            mRetried = true;
            start();
        }
    }

    private void doLogin() {
        if (mSid != null && mLsid != null) {
            new Thread(this, "auto-login").start();
        }
    }

    private static String getAppAndVersion(Activity context) {
        try {
            String versionName = context.getPackageManager().
                    getPackageInfo(context.getPackageName(), 0).versionName;
            return "Chrome/" + versionName;
        } catch (Exception e) {
            Log.e(TAG, "Unable to find package when generating version name", e);
            return "Chrome/unknown";
        }
    }

    @Override
    public void run() {
        // TODO(jgreenwald): consider extracting this to a separate class to remove confusion
        // about this class having a start() method and not being a Thread.
        // TODO(jgreenwald): see if we can use the Chrome network stack for everything

        String url = ISSUE_AUTH_TOKEN_URL.buildUpon()
                .appendQueryParameter("SID", mSid)
                .appendQueryParameter("LSID", mLsid)
                .build().toString();

        AndroidHttpClient client = AndroidHttpClient.newInstance(getAppAndVersion(mActivity));
        HttpPost request = new HttpPost(url);

        String result = null;
        try {
            HttpResponse response = client.execute(request);

            int status = response.getStatusLine().getStatusCode();
            if (status != HttpStatus.SC_OK) {
                Log.d(TAG, "LOGIN_FAIL: Bad status from auth url "
                      + status + ": "
                      + response.getStatusLine().getReasonPhrase());
                if (status == HttpStatus.SC_FORBIDDEN) {
                    // This seems to occur relatively often. Retrying usually
                    // works.
                    Log.d(TAG, "LOGIN_FAIL: Invalidating tokens...");
                    retry();
                    return;
                }
                return;
            }

            HttpEntity entity = response.getEntity();
            if (entity == null) {
                Log.d(TAG, "LOGIN_FAIL: Null entity in response");
                return;
            }
            result = EntityUtils.toString(entity, "UTF-8");
            Log.d(TAG, "LOGIN_SUCCESS");

        } catch (Exception e) {
            Log.d(TAG, "LOGIN_FAIL: Exception acquiring uber token " + e);
            request.abort();
            return;
        } finally {
            client.close();
        }

        // "source" matches gaia_constants.cc
        final String newUrl = TOKEN_AUTH_URL.buildUpon()
                .appendQueryParameter("source", "ChromiumBrowser")
                .appendQueryParameter("auth", result)
                .appendQueryParameter("continue", "http://www.google.com")
                .build().toString();

        mActivity.runOnUiThread(new Runnable() {
            @Override public void run() {
                nativeLogIn(nativeInit(), newUrl);
            }
        });
    }

    private native int nativeInit();
    private native void nativeLogIn(int nativeGoogleAutoLogin, String url);

}
