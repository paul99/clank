// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import android.app.Activity;
import android.os.Bundle;

import com.google.android.apps.chrome.infobar.InfoBarContainer;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.ChromeView;
import org.chromium.content.browser.ChromeViewClient;

import com.google.common.annotations.VisibleForTesting;

// InstantTab is a ChromeView container that prefetches a page for
// instant.  If the prefetch is committed (e.g. we'll be using it),
// our ChromeView will be given to the currently active tab.  An
// InstantTab is currently never displayed.
class InstantTab {

    private int mNativeInstantTab;
    private String mUrl;  // The URL we have instantly fetched; may be null.
    private Tab mTabToCommitOn;  // The Tab to "replace" if we commit instant.
    private ChromeView mChromeView;
    private InfoBarContainer mInfoBarContainer;
    // Whether the page was fully loaded. Used by Tab when committing to know whether the
    // ON_PAGE_LOAD notification should be sent again (as the one in the InstantTab is dropped).
    private boolean mPageFinishedReceived;

    // |current| is the current active tab.  This is used as a basis
    // for the instant tab (e.g. where to start history from should we
    // commit), and is the one we logically replace if we commit instant.
    InstantTab(Activity activity, Tab current, boolean incognito) {
        mChromeView = new ChromeView(activity, incognito, 0, ChromeView.Personality.BROWSER);
        mChromeView.setChromeViewClient(new ChromeViewClient() {
            @Override
            public void onPageFinished(String url) {
                if (nativePrefetchCanceled(mNativeInstantTab)) {
                    return;
                }
                mPageFinishedReceived = true;
                Bundle b = new Bundle();
                b.putInt("tabId", mTabToCommitOn.getId());
                b.putString("url", (getView() != null) ? getView().getUrl() : "");
                ChromeNotificationCenter.broadcastNotification(
                        ChromeNotificationCenter.INSTANT_ON_PAGE_LOAD_FINISHED, b);
            }
        });
        mInfoBarContainer = new InfoBarContainer(activity, mChromeView, null);
        mTabToCommitOn = current;
        mNativeInstantTab = nativeInit(mTabToCommitOn.getView(), mChromeView);
    }

    public void destroy() {
        if (mChromeView != null) {
            mChromeView.destroy();
            mChromeView = null;
        }
        // It is important to destroy the native InstantTab before the mInfobarContainer in order
        // to avoid a potential DCHECK in infobar_android.cc, triggered by the infobar count not
        // being 0 at that point. This happens when prerendering a page with infobars and an unload
        // handler (ex: maps.google.com). When we destroy mChromeView, since the ChromeView does not
        // own the prerendered TabContents (it's owned by the instant loader of the native
        // InstantTab) it does not delete it, destroying the native InstantTab does. So by
        // destroying the InstantTab here triggers destroying of the InstantLoader which owns the
        // TabContents, and by destroying it remove the infobars. Then when we destroy the native
        // InfoBarContainer below, it does not contain infobars anymore, as expected.
        // Note that if the page does not have an unload handler, deleting the ChromeView triggers
        // the fast termination path, which is to terminate the renderer process which notifies the
        // TabContents and removes the infobars, so it is not that important in that case.
        nativeDestroy(mNativeInstantTab);
        mNativeInstantTab = 0;
        if (mInfoBarContainer != null) {
            mInfoBarContainer.destroy();
            mInfoBarContainer = null;
        }
    }

    public String getPrefetchedUrl() {
        return mUrl;  // could be null
    }

    public ChromeView getView() {
        return mChromeView;
    }

    /**
     * Returns and dissociates the InfoBarContainer this InstantTab has, so that its native
     * counterpart won't be destroyed when this InstantTab is destroyed.
     * @return the InfoBarContainer associated with this InstantTab.
     */
    public InfoBarContainer acquireInfoBarContainer() {
        InfoBarContainer container = mInfoBarContainer;
        mInfoBarContainer = null;
        return container;
    }

    public void prefetchUrl(String url, int pageTransition) {
        if (url.equals(mUrl)) {
            return;  // Already fetched
        }
        if (nativePrefetchUrl(mNativeInstantTab, url, pageTransition)) {
            mUrl = url;
        } else {
            mUrl = null;
        }
    }

    public boolean commitPrefetch() {
        assert mUrl != null;
        if (!nativeCommitPrefetch(mNativeInstantTab)) {
            return false;
        }
        mTabToCommitOn.importPrefetchTab(this);
        // when a Tab imports us, it takes ownership of our ChromeView and our mInfoBarContainer.
        mChromeView = null;
        mInfoBarContainer = null;
        return true;
    }

    // Called by native code when prefetch fails. This typically happens when
    // the server returns an error code.
    @CalledByNative
    private void discardPrefetch() {
        mUrl = null;
    }

    @VisibleForTesting
    InfoBarContainer getInfoBarContainer() {
        return mInfoBarContainer;
    }

    /**
     * Returns whether the instant ChromeView received the onPageFinished notification, meaning the
     * prerendered page has finished loading.
     */
    boolean didReceivePageFinished() {
        return mPageFinishedReceived;
    }

    private native int nativeInit(ChromeView toReplace, ChromeView myChromeView);
    private native void nativeDestroy(int nativeInstantTab);
    private native boolean nativePrefetchUrl(int nativeInstantTab, String url, int pageTransition);
    private native boolean nativeCommitPrefetch(int nativeInstantTab);
    private native boolean nativePrefetchCanceled(int nativeInstantTab);
}
