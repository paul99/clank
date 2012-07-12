// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.infobar;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerCallback;
import android.accounts.AccountManagerFuture;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.google.android.apps.chrome.AccountManagerContainer;
import com.google.android.apps.chrome.AccountManagerHelper;
import com.google.android.apps.chrome.ChromeActivity;
import com.google.android.apps.chrome.ChromeNotificationCenter;
import com.google.android.apps.chrome.R;
import com.google.android.apps.chrome.Tab;
import com.google.android.apps.chrome.sync.SyncStatusHelper;
import com.google.android.apps.chrome.utilities.ViewUtilities;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.ChromeView;
import org.chromium.content.browser.CommandLine;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

/**
 * A container for all the infobars of a specific tab.
 * Note that infobars creation can be initiated from Java of from native code.
 * When initiated from native code, special code is needed to keep the Java and native infobar in
 * sync, see NativeInfoBar below.
 *
 */
public class InfoBarContainer extends LinearLayout {
    private static final String TAG = "InfoBarContainer";

    // Native InfoBarContainer pointer which will be set by nativeInit()
    private int mNativeInfoBarContainer;

    private ViewGroup mChromeView;

    private final Activity mActivity;

    // The tab that contains us.
    private Tab mTab;

    // Whether the infobar are shown on top (below the location bar) or at the bottom of the screen.
    private final boolean mInfoBarsOnTop;

    // The list of all infobars in this container, regardless of whether they've been shown yet.
    private final ArrayList<InfoBar> mInfoBars = new ArrayList<InfoBar>();

    // We only animate adding/removing infobars one at a time.
    // These two lists track the pending infobars to be shown or hidden.
    private final ArrayDeque<InfoBar> mInfoBarsToShow = new ArrayDeque<InfoBar>();
    private final ArrayDeque<InfoBar> mInfoBarsToHide = new ArrayDeque<InfoBar>();

    // True while an infobar is animating onto or off of the screen.
    private boolean mIsAnimatingInfoBar = false;

    // The view that should be clipped to its bounds. Used for animation, see drawChild().
    private View mViewClippedToBounds;

    // True when this container has been emptied and its native counterpart has been destroyed.
    private boolean mDestroyed = false;

    public InfoBarContainer(Activity activity, ChromeView chromeView, Tab tab) {
        super(chromeView.getContext());

        mNativeInfoBarContainer = nativeInit(chromeView);
        mChromeView = chromeView;
        mActivity = activity;
        mTab = tab;

        setOrientation(LinearLayout.VERTICAL);
        // The tablet has the infobars below the location bar. On the phone they are at the bottom.
        mInfoBarsOnTop = ChromeActivity.isTabletUi(activity);
        setGravity(mInfoBarsOnTop ? Gravity.TOP : Gravity.BOTTOM);
    }

    // Returns true if we are the infobar container of an instant tab (which does not show infobars
    // visually).
    private boolean isOwnedByInstantTab() {
        return mTab == null;
    }

    @Override
    protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
        if (!mInfoBarsOnTop || child != mViewClippedToBounds) {
            return super.drawChild(canvas, child, drawingTime);
        }
        // When infobars are on top, the new infobar Z-order is greater than the previous infobar,
        // which means it shows on top during the animation. We cannot change the Z-order in the
        // linear layout, it is driven by the insertion index.
        // So we simply clip the children to their bounds to make sure the new infobar does not
        // paint over.
        boolean retVal;
        canvas.save();
        canvas.clipRect(child.getLeft(), child.getTop(), child.getRight(), child.getBottom());
        retVal = super.drawChild(canvas, child, drawingTime);
        canvas.restore();
        return retVal;
    }

    private void broadcast(int notification, InfoBar infoBar) {
        Bundle b = new Bundle();
        b.putInt("tabId", mTab.getId());
        b.putInt("infoBarId", infoBar.getId());
        ChromeNotificationCenter.broadcastNotification(notification, b);
    }

    public void addInfoBar(InfoBar infoBar) {
        assert !mDestroyed;

        if (mInfoBars.contains(infoBar)) {
            Log.w(TAG, "Trying to add an info bar that has already been added.");
            assert false;
            return;
        }

        // We add the infobar immediately to mInfoBars but we wait for the animation to end to
        // notify it's been added, as tests rely on this notification but expects the infobar view
        // to be available when they get the notification.
        mInfoBars.add(infoBar);
        infoBar.setInfoBarContainer(this);
        mInfoBarsToShow.add(infoBar);
        if (!isOwnedByInstantTab() && !mIsAnimatingInfoBar) {
            mIsAnimatingInfoBar = true;
            processPendingInfoBars();
        }
    }

    public void removeInfoBar(InfoBar infoBar) {
        assert !mDestroyed;

        if (!mInfoBars.remove(infoBar)) {
            Log.w(TAG, "Trying to remove an info bar that is not in this container.");
            assert false;
            return;
        }

        // The infobar might not have been added to the view hierarchy yet (in which case it is in
        // mInfoBarsToShow).  In that case we shouldn't attempt to remove it visually.
        if (!mInfoBarsToShow.remove(infoBar)) {
            // There are no visible or prior infobars in the instant case, so we can only remove
            // an infobar that is pending to show in that case.
            // So we should never get here in the instant case.
            assert !isOwnedByInstantTab();
            mInfoBarsToHide.add(infoBar);
            if (!mIsAnimatingInfoBar) {
                mIsAnimatingInfoBar = true;
                processPendingInfoBars();
            }
        }
    }

    // TODO(jcivelli): http://b/5276658 when revisiting the instant infobar related code, get rid
    //                 of this method and make mTab final again.
    public void instantCommited(Tab tab, List<InfoBar> previousInfoBars) {
        assert !mDestroyed;
        mTab = tab;
        for (InfoBar infoBar : previousInfoBars) {
            assert infoBar.getInfoBarContainer().hasBeenDestroyed();
            if (infoBar.shouldExpire(tab.getUrl())) {
                infoBar.dismiss();
                // Dismissing an infobar from a destroyed container does not trigger the removed
                // notification (it is needed for tests).
                broadcast(ChromeNotificationCenter.INFOBAR_REMOVED, infoBar);
            } else {
                addInfoBar(infoBar);
            }
        }
        if (!mIsAnimatingInfoBar) {
            mIsAnimatingInfoBar = true;
            processPendingInfoBars();
        }
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        // Hide the infobars when the keyboard is showing.
        boolean isShowing = (getVisibility() == View.VISIBLE);
        if (ViewUtilities.isKeyboardShowing(mActivity)) {
            if (isShowing) {
                setVisibility(View.INVISIBLE);
            }
        } else {
            if (!isShowing) {
                setVisibility(View.VISIBLE);
            }
        }
        super.onLayout(changed, l, t, r, b);
    }

    /**
     * @return True when this container has been emptied and its native counterpart has been
     *         destroyed.
     */
    public boolean hasBeenDestroyed() {
        return mDestroyed;
    }

    private void processPendingInfoBars() {
        assert mIsAnimatingInfoBar;
        if (!mInfoBarsToHide.isEmpty()) {
            removeAndFadeNextInfoBar();
        } else if (!mInfoBarsToShow.isEmpty()) {
            showAndAnimateNextInfoBar();
        } else {
            mIsAnimatingInfoBar = false;
        }
    }

    private void showAndAnimateNextInfoBar() {
        assert mIsAnimatingInfoBar;
        assert !mInfoBarsToShow.isEmpty();

        final InfoBar infoBar = mInfoBarsToShow.remove();

        // In order to animate the addition of the infobar, we need a lay-out first (so the new
        // infobar is properly sized -but still invisible).
        final View infoBarView = addInfoBarToViewHierarchy(infoBar, !mInfoBarsOnTop);
        infoBarView.setVisibility(INVISIBLE);

        // We'll continue the animation after the layout has happened.
        post(new Runnable() {
            @Override
            public void run() {
                if (!mInfoBars.contains(infoBar)) return;

                int translationY = infoBarView.getHeight();
                if (mInfoBarsOnTop) translationY *= -1;
                infoBarView.setTranslationY(translationY);
                ObjectAnimator animation = ObjectAnimator.ofFloat(infoBarView, "translationY", 0);
                animation.addListener(new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        mViewClippedToBounds = null;
                        broadcast(ChromeNotificationCenter.INFOBAR_ADDED, infoBar);
                        processPendingInfoBars();
                    }
                });
                animation.setDuration(500);
                infoBarView.setVisibility(VISIBLE);
                if (mInfoBarsOnTop) {
                    // We need to clip this view to its bounds while it is animated as it is on top
                    // of the previous toolbar.
                    mViewClippedToBounds = infoBarView;
                }
                animation.start();
            }
        });
    }

    private View addInfoBarToViewHierarchy(InfoBar infoBar, boolean addOnTop) {
        if (mChromeView.indexOfChild(this) == -1) {
            mChromeView.addView(this);
        }
        View infoBarView = infoBar.getContent(mActivity);
        addView(infoBarView, addOnTop ? 0 : getChildCount(),
                new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));

        return infoBarView;
    }

    private void removeAndFadeNextInfoBar() {
        assert mIsAnimatingInfoBar;
        assert !mInfoBarsToHide.isEmpty();

        final InfoBar infoBar = mInfoBarsToHide.remove();

        // We animate the removed infobar and as a result we also need to animate infobars
        // above/below it (depending on phone/tablet).
        final View infoBarView = infoBar.getContent(mActivity, false);

        int infoBarViewIndex = indexOfChild(infoBarView);

        final Runnable cleanUpInfobarRunnable = new Runnable() {
            @Override
            public void run() {
                mViewClippedToBounds = null;
                if (infoBarView != null) removeView(infoBarView);
                broadcast(ChromeNotificationCenter.INFOBAR_REMOVED, infoBar);

                // If there are no infobar shown, there is no need to keep the infobar container in
                // the view hierarchy.
                if (InfoBarContainer.this.getChildCount() == 0 &&
                        mTab != null && mTab.getView() != null) {
                    mTab.getView().removeView(InfoBarContainer.this);
                }
                processPendingInfoBars();
            }
        };

        if (infoBarViewIndex == -1 || infoBarView == null) {
            Log.w(TAG, "Trying to fade infobar (" + (infoBarView != null) + ") with index "
                    + infoBarViewIndex);

            // Assert here to get a break on non-release builds.
            assert false;

            // Somehow we don't have a valid info bar view/index to remove.  Try to process any
            // other pending infobars and return.
            post(cleanUpInfobarRunnable);
            return;
        }

        ArrayList<Animator> animators = new ArrayList<Animator>();
        if (mInfoBarsOnTop) {
            // This will confirm that the index is within the bounds of the possible children of
            // the container and that the container has at least one child (which it should if we
            // are here).
            assert 0 <= infoBarViewIndex && infoBarViewIndex < getChildCount();
            PropertyValuesHolder translateValue =
                    PropertyValuesHolder.ofFloat("translationY", -infoBarView.getHeight());
            for (int i = Math.max(0, infoBarViewIndex); i < getChildCount(); ++i) {
                View v = getChildAt(i);
                assert v != null;
                animators.add(ObjectAnimator.ofPropertyValuesHolder(v, translateValue));
            }
        } else {
            PropertyValuesHolder translateValue =
                    PropertyValuesHolder.ofFloat("translationY", infoBarView.getHeight());
            for (int i = 0; i <= infoBarViewIndex && i < getChildCount(); ++i) {
                View v = getChildAt(i);
                assert v != null;
                animators.add(ObjectAnimator.ofPropertyValuesHolder(v, translateValue));
            }
        }

        final AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animator) {
                // Reset translationY for the shown infobars so they show correctly after layout.
                for (Animator childAnimator : animatorSet.getChildAnimations()) {
                    ((View) ((ObjectAnimator) childAnimator).getTarget()).setTranslationY(0);
                }

                cleanUpInfobarRunnable.run();
            }
        });
        animatorSet.setDuration(500);
        animatorSet.playTogether(animators);
        if (mInfoBarsOnTop) {
            // We need to clip this view to its bounds while it is animated as it is on top
            // of the previous toolbar.
            mViewClippedToBounds = infoBarView;
        }
        animatorSet.start();
    }

    // Called by the tab when it has started loading a new page.
    public void onPageStarted(String url) {
        LinkedList<InfoBar> barsToRemove = new LinkedList<InfoBar>();

        for (InfoBar infoBar : mInfoBars) {
            if (infoBar.shouldExpire(url)) {
                barsToRemove.add(infoBar);
            }
        }

        for (InfoBar infoBar : barsToRemove) {
            infoBar.dismiss();
        }
    }

    /**
     * Return the total number of visible infobars in this container.
     * @return count of visible infobars
     */
    public int getInfoBarCount() {
        return mChromeView.getChildCount();
    }

    /**
     * Returns the tab we are associated with.
     */
    public Tab getTab() {
        return mTab;
    }

    // In cases where the native code triggers an infobar (geolocation infobar for example), the
    // native side has a InfoBarDelegate and the Java side an InfoBar. This class links the 2 so
    // that if any of them is closed, the other is notified/cleaned-up.
    static class NativeInfoBar implements InfoBarDismissedListener {
        public NativeInfoBar(int nativeInfoBar) {
            assert nativeInfoBar != 0;
            mNativeInfoBarPtr = nativeInfoBar;
        }

        public void setInfoBar(InfoBar infoBar) {
            mInfoBar = infoBar;
            // The native code takes care of expiring infobars on navigations.
            mInfoBar.setExpireOnNavigation(false);
        }

        public InfoBar getInfoBar() {
            return mInfoBar;
        }

        @CalledByNative("NativeInfoBar")
        public void dismiss() {
            // We are being closed from C++.
            mInfoBar.dismiss();
        }

        @Override
        public void onInfoBarDismissed(InfoBar infoBar) {
            // TODO(jcivelli): figure-out how this can be called more than once.
            if (mNativeInfoBarPtr == 0) return;

            // We are being closed from Java, notify C++.
            mInfoBar.getInfoBarContainer().nativeOnInfoBarClosed(mNativeInfoBarPtr);
            // The native object is deleted.
            mNativeInfoBarPtr = 0;
        }

        private InfoBar mInfoBar;
        protected int mNativeInfoBarPtr;
    }

    // NativeConfirmInfoBar is used to link a ConfirmInfoBar and its native counterpart.
    static class NativeConfirmInfoBar extends NativeInfoBar implements ConfirmInfoBarListener {
        public NativeConfirmInfoBar(int nativeInfoBar, String buttonOk, String buttonCancel,
                String title, Bitmap icon) {
            super(nativeInfoBar);
            setInfoBar(new ConfirmInfoBar(this, buttonOk, buttonCancel, title, icon));
        }

        @Override
        public void onConfirmInfoBarButtonClicked(ConfirmInfoBar infoBar, boolean confirm) {
            if (mNativeInfoBarPtr != 0) {
                infoBar.getInfoBarContainer().nativeOnConfirmClicked(mNativeInfoBarPtr, confirm);
            }
        }
    }

    // Called by the native code to show a confirmation InfoBar.
    // If you need to display a confirm infobar that is not associated with native code, simply
    // create a ConfirmInfoBar and add it to the InfoBarContainer. See ConfirmInfoBar.java.
    @CalledByNative
    NativeInfoBar showConfirmInfoBar(int nativeInfoBar, String buttonOk,
                                     String buttonCancel, String title, Bitmap icon) {
        NativeConfirmInfoBar nativeConfirmInfoBar =
                new NativeConfirmInfoBar(nativeInfoBar, buttonOk, buttonCancel, title, icon);
        addInfoBar(nativeConfirmInfoBar.getInfoBar());
        return nativeConfirmInfoBar;
    }

    // Returns null if there are no accounts to display.
    @CalledByNative
    NativeInfoBar showAutoLoginInfoBar(int nativeInfoBar, String realm, String account, String args,
                                       String okButtonText, String cancelButtonText) {
        NativeInfoBar nativeAutoLoginInfoBar = new NativeInfoBar(nativeInfoBar);
        AutoLoginInfoBar infoBar = new AutoLoginInfoBar(nativeAutoLoginInfoBar, mActivity,
                realm, account, args, okButtonText, cancelButtonText);
        // Don't bother displaying the infobar if we have no accounts to show.
        Log.i("InfoBarContainer", "infoBar.hasAccount(): " + infoBar.hasAccount());
        if (!infoBar.hasAccount())
            return null;
        nativeAutoLoginInfoBar.setInfoBar(infoBar);
        addInfoBar(infoBar);
        return nativeAutoLoginInfoBar;
    }

    public void destroy() {
        mDestroyed = true;
        removeAllViews();
        if (mNativeInfoBarContainer != 0) {
            nativeDestroy(mNativeInfoBarContainer);
        }
        mInfoBarsToShow.clear();
        mInfoBarsToHide.clear();
    }

    public List<InfoBar> getInfoBars() {
        return mInfoBars;
    }

    /**
     * Dismisses all {@link AutoLoginInfoBar}s in this {@link InfoBarContainer} that are for
     * {@code accountName} and {@code authToken}.  This also resets all {@link InfoBar}s that are
     * for a different request.
     * @param accountName The name of the account request is being accessed for.  This can be found
     *                    by querying {@link AutoLoginInfoBar#getAccountName()}.
     * @param authToken The authentication token access is being requested for.  This can be found
     *                  by querying {@link AutoLoginInfoBar#getAuthToken()}.
     * @param success Whether or not the authentication attempt was successful.
     * @param result The resulting token for the auto login request (ignored if {@code success} is
     *               {@code false}.
     */
    public void dismissAutoLogins(String accountName, String authToken, boolean success,
            String result) {
        // We can't dismiss an infobar while looping over mInfoBars because that causes a
        // modification to mInfoBars: http://b/6357426.  Instead we loop over a copy of mInfoBars.
        for (InfoBar infoBar : new ArrayList<InfoBar>(mInfoBars)) {
            if (infoBar instanceof AutoLoginInfoBar) {
                AutoLoginInfoBar aib = (AutoLoginInfoBar) infoBar;

                String aibAccount = aib.getAccountName();
                String aibAuth = aib.getAuthToken();

                if (aibAccount != null && aibAccount.equals(accountName) &&
                        aibAuth != null && aibAuth.equals(authToken)) {
                    if (success) {
                        // This will dismiss the info bar.
                        aib.loginSucceeded(result);
                    } else {
                        // This will dismiss the info bar.
                        aib.loginFailed();
                    }
                } else {
                    aib.setButtonsEnabled(true);
                }
            }
        }
    }

    /**
     * Dismisses all {@link AutoLoginInfoBar}s in this {@link InfoBarContainer}.
     */
    public void dismissAutoLogins() {
        // Iterate over a copy of mInfoBars to avoid modifying mInfoBars (via dismiss()) during
        // iteration.
        for (InfoBar infoBar : new ArrayList<InfoBar>(mInfoBars)) {
            if (infoBar instanceof AutoLoginInfoBar) {
                infoBar.dismiss();
            }
        }
    }

    private class AutoLoginInfoBar extends InfoBar implements AccountManagerCallback<Bundle>,
            View.OnClickListener {
        private Button mLoginButton;
        private Button mCancelButton;
        private AccountManagerContainer mAccountManagerContainer;
        private Activity mActivity;
        private NativeInfoBar mNativeInfoBar;
        private Account mAccount;      // May be null, if user signs out as the infobar is created.
        private boolean mIsSingleLine;
        private String mOkButtonText;
        private String mCancelButtonText;
        private String mMessage;

        AutoLoginInfoBar(NativeInfoBar nativeInfoBar, Activity activity,
                String realm, String account, String args,
                String okButtonText, String cancelButtonText) {
            super(nativeInfoBar);
            mNativeInfoBar = nativeInfoBar;
            mActivity = activity;
            mAccountManagerContainer = new AccountManagerContainer(activity, realm, account, args);

            Account extraAccount = newExtraAccount();
            mAccount = (extraAccount != null) ? extraAccount :
                SyncStatusHelper.get(activity).getSignedInUser();

            String accountName = (mAccount == null) ? "" : mAccount.name;
            mMessage = nativeGetAutoLoginMessage(mNativeInfoBar.mNativeInfoBarPtr, accountName);
            mOkButtonText = okButtonText;
            mCancelButtonText = cancelButtonText;
        }

        // Returns a fake account for testing of infobar layout and failure case, if one has been
        // specified on the command line.  Otherwise, returns null.
        private Account newExtraAccount() {
            String name = CommandLine.getInstance().getSwitchValue(
                    CommandLine.AUTO_LOGIN_EXTRA_ACCOUNT);
            return (name == null) ? null : AccountManagerHelper.createAccountFromName(name);
        }

        @Override
        protected View createContent(Context context) {
            View content = LayoutInflater.from(context).inflate(
                    R.layout.infobar_text_and_two_buttons, null);

            TextView message = (TextView) content.findViewById(R.id.infobar_message);
            message.setText(mMessage);

            mLoginButton = (Button) content.findViewById(R.id.button_ok);
            mLoginButton.setText(mOkButtonText);
            mLoginButton.setOnClickListener(this);

            // The behavior attached to the cancel button differs from desktop Chrome: "cancel" on
            // Clank vs. "never save" on desktop.
            mCancelButton = (Button) content.findViewById(R.id.button_cancel);
            mCancelButton.setText(mCancelButtonText);
            mCancelButton.setOnClickListener(this);

            // On tablet, the text and buttons all fit on one line. On phone, use a two-line layout.
            mIsSingleLine = ChromeActivity.isTabletUi(context);

            return content;
        }

        @Override
        protected boolean shouldCenterIcon() {
            return mIsSingleLine;
        }

        @Override
        protected Integer getIconResourceId() {
            return R.drawable.infobar_autologin;
        }

        @Override
        public void onClick(View v) {
            if (v == mLoginButton) {
                Log.i("InfoBar", "auto-login requested for "
                        + (mAccount != null ? mAccount.toString() : "?"));

                Account currentAccount = SyncStatusHelper.get(mActivity).getSignedInUser();
                if (mAccount == null || !mAccount.equals(currentAccount)) {
                    Log.i("InfoBar", "auto-login failed because account is no longer valid");
                    loginFailed();
                    return;
                }

                // The callback for this request comes in on a non-UI thread.
                mAccountManagerContainer.getAuthToken(mAccount, this);
                // TODO(jrg): This might be a good time to provide feedback to the user that
                //            something interesting is happening.  For the moment we leave the
                //            infobar up but disable all controls.  If the account manager
                //            doesn't respond fast enough, better feedback will be needed.
                setButtonsEnabled(false);
            } else if (v == mCancelButton) {
                dismiss();
            } else {
                assert false;
            }
        }

        // Returns the number of accounts displayed in the spinner.
        public boolean hasAccount() {
            return mAccount != null;
        }

        // Returns true if an automatic login was triggered for an exact account match.
        public boolean exactMatch() {
            return mAccountManagerContainer.exactMatch();
        }

        /**
         * @return The auth token access is being requested for.
         */
        String getAuthToken() {
            return mAccountManagerContainer.getAuthToken();
        }

        /**
         * @return The account request is being accessed for.
         */
        String getAccountName() {
            return mAccountManagerContainer.getAccountName();
        }

        @Override
        public void run(AccountManagerFuture<Bundle> value) {
            String result = null;
            try {
                result = value.getResult().getString(AccountManager.KEY_AUTHTOKEN);
            } catch (Exception e) {
                result = null;
            }

            final boolean success = result != null;
            final String finalResult = result;
            // Can't rely on the Bundle's auth token or account name as they might be null
            // if this was a failed attempt.
            final String finalAuthToken = mAccountManagerContainer.getAuthToken();
            final String finalAccountName = mAccountManagerContainer.getAccountName();

            mActivity.runOnUiThread(new java.lang.Runnable() {
                @Override
                public void run() {
                    Bundle b = new Bundle();
                    b.putString("accountName", finalAccountName);
                    b.putString("authToken", finalAuthToken);
                    b.putBoolean("success", success);
                    b.putString("result", finalResult);
                    ChromeNotificationCenter.broadcastImmediateNotification(
                            ChromeNotificationCenter.AUTO_LOGIN_RESULT, b);
                }
            });
        }

        /**
         * Enables or disables the infobar's Ok/Cancel buttons.
         */
        void setButtonsEnabled(boolean isEnabled) {
            if (mLoginButton != null) {
                mLoginButton.setEnabled(isEnabled);
            }
            if (mCancelButton != null) {
                mCancelButton.setEnabled(isEnabled);
            }
        }

        /**
         * Called when login was a success and this info bar should be removed.
         * @param result The result auth token url.
         */
        private void loginSucceeded(final String result) {
            if (mNativeInfoBar.mNativeInfoBarPtr != 0) {
                nativeOnAutoLogin(mNativeInfoBar.mNativeInfoBarPtr, result);
            }
            dismiss();
        }

        /**
         * Called when login was a failure and this info bar should be dismissed and replaced by
         * a login failure info bar.
         */
        private void loginFailed() {
            // Perhaps an account given to us by the Account Manager no longer has an
            // autologin token because it was deleted while the infobar was up?
            dismiss();
            String message = mActivity.getString(R.string.auto_login_failed);
            addInfoBar(MessageInfoBar.createWarningInfoBar(message));
        }
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private static boolean shouldShowAutoLogin(ChromeView chromeView,
            String realm, String account, String args) {
        AccountManagerContainer accountManagerContainer =
            new AccountManagerContainer((Activity) chromeView.getContext(), realm, account, args);
        return accountManagerContainer.getAccount(null) != null;
    }

    private native int nativeInit(ChromeView chromeView);
    private native void nativeDestroy(int nativeInfoBarContainerAndroid);
    private native void nativeOnInfoBarClosed(int nativeInfoBar);
    private native void nativeOnConfirmClicked(int nativeInfoBar, boolean confirm);
    private native void nativeOnAutoLogin(int nativeInfoBar, String url);
    private native String nativeGetAutoLoginMessage(int nativeInfoBar, String account);
}
