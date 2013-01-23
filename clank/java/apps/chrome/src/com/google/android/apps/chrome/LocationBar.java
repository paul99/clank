// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import static com.google.android.apps.chrome.ToolbarPhone.ALPHA_ANIM_PROPERTY;
import static com.google.android.apps.chrome.ToolbarPhone.URL_FOCUS_CHANGE_ANIMATION_DURATION_MS;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.os.Parcelable;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.text.Spannable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.style.CharacterStyle;
import android.text.style.ForegroundColorSpan;
import android.text.style.StrikethroughSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.util.SparseArray;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.View.OnClickListener;
import android.view.ViewStub;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputConnectionWrapper;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListPopupWindow;
import android.widget.ListView;
import android.widget.PopupWindow.OnDismissListener;
import android.widget.TextView;

import com.google.android.apps.chrome.ChromeNotificationCenter.ChromeNotificationHandler;
import com.google.android.apps.chrome.OmniboxPopupAdapter.OmniboxPopupItem;
import com.google.android.apps.chrome.OmniboxPopupAdapter.OmniboxSuggestionSelectionHandler;
import com.google.android.apps.chrome.bridge.AutocompleteController;
import com.google.android.apps.chrome.bridge.AutocompleteController.OnSuggestionsReceivedListener;
import com.google.android.apps.chrome.glui.GLUIFunctorRenderer;
import com.google.android.apps.chrome.preferences.ChromePreferenceManager;
import com.google.android.apps.chrome.suggestionproviders.VoiceSuggestionProvider.VoiceResult;
import com.google.android.apps.chrome.utilities.FeatureUtilities;
import com.google.android.apps.chrome.utilities.URLUtilities;
import com.google.android.apps.chrome.widget.VerticallyFixedEditText;
import com.google.common.annotations.VisibleForTesting;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.ChromeView;
import org.chromium.content.browser.ChromeViewDebug;
import org.chromium.content.browser.CommandLine;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * This class represents the location bar where the user types in URLs and
 * search terms.
 */
public class LocationBar extends FrameLayout implements OnClickListener,
        OnSuggestionsReceivedListener, OnNativeLibraryReadyUIListener,
        GLUIFunctorRenderer.Listener {

    private static final String LOG_TAG = LocationBar.class.getCanonicalName();

    private static final int OMNIBOX_SUGGESTIONS_TRANSITION_DURATION_MS = 200;
    // Delay triggering the omnibox results upon key press to allow the location bar to repaint
    // with the new characters.
    private static final long OMNIBOX_SUGGESTION_START_DELAY_MS = 30;
    private static final long OMNIBOX_POPUP_RESIZE_DELAY_MS = 30;

    // The minimum confidence threshold that will result in navigating directly to a voice search
    // response (as opposed to treating it like a typed string in the Omnibox).
    private static final float VOICE_SEARCH_CONFIDENCE_NAVIGATE_THRESHOLD = 0.9f;

    private static final int CONTENT_OVERLAY_COLOR = Color.argb(166, 0, 0, 0);

    private static int[] NOTIFICATIONS = { ChromeNotificationCenter.VOICE_SEARCH_RESULTS };

    protected ImageView mNavigationButton;
    protected ImageButton mSecurityButton;
    protected ImageButton mDeleteButton;
    protected ImageButton mMicButton;
    protected UrlBar mUrlBar;

    private TextView mSnapshotPrefixLabel;

    AutocompleteController mAutocomplete;

    // The toolbar this location bar belongs to.
    protected ToolbarDelegate mToolbar;

    // Value between 0 and 100 that reflects the load progress.
    private int mLoadProgress;

    private int mNativeLocationBar;

    private List<Runnable> mDeferredNativeRunnables = new ArrayList<Runnable>();

    // The type of the navigation button currently showing.
    private NavigationButtonType mNavigationButtonType;

    // The type of the security icon currently active.
    private SecurityLevel mSecurityIconType;

    private OmniboxPopupAdapter mSuggestionListAdapter;
    private SuggestionsPopupWindow mSuggestionListPopup;

    private List<OmniboxPopupItem> mSuggestionItems;

    // Set to true when the URL bar text is modified programmatically. Initially set
    // to true until the old state has been loaded.
    private boolean mIgnoreURLBarModification = true;

    private boolean mLastUrlEditWasDelete = false;

    // The index in the location bar string where the autocomplete string starts.
    private int mAutocompleteIndex = -1;

    // Used to show a quick load progress bar when a page that has been fully prefetched is shown.
    private LoadProgressSimulator mLoadProgressSimulator;

    private Handler mUiHandler;

    private Runnable mRequestSuggestions;
    private Runnable mInstantTrigger;
    private Runnable mForceResizeSuggestionPopupRunnable;
    private boolean mClearSuggestionsOnDismiss = true;

    private final ArrayList<CharacterStyle> mSpanList = new ArrayList<CharacterStyle>();

    private View mContentOverlay;
    private Drawable mUrlFocusContentOverlay;
    private boolean mSuggestionsShown;
    protected boolean mUrlHasFocus;

    private boolean mSecurityButtonShown;

    private AnimatorSet mLocationBarIconActiveAnimator;
    private AnimatorSet mSecurityButtonShowAnimator;
    private AnimatorSet mNavigationIconShowAnimator;

    private final int mStartSchemeDefaultColor;
    private final int mStartSchemeSecurityWarningColor;
    private final int mStartSchemeSecurityErrorColor;
    private final int mStartSchemeEvSecureColor;
    private final int mStartSchemeSecureColor;
    private final int mSchemeToDomainColor;
    private final int mDomainAndRegistryColor;
    private final int mTrailingUrlColor;

    private ArrayList<TextChangeListener> mTextChangeListeners;

    /**
     * Specifies the types of buttons shown to signify different types of navigation elements.
     */
    protected enum NavigationButtonType {
        GLOBE,
        MAGNIFIER,
    }

    // Copied over from toolbar_model.h.
    private enum SecurityLevel {
        NONE,              // HTTP/no URL/user is editing
        EV_SECURE,         // HTTPS with valid EV cert
        SECURE,            // HTTPS (non-EV)
        SECURITY_WARNING,  // HTTPS, but unable to check certificate revocation
                           // status or with insecure content on the page
        SECURITY_ERROR,    // Attempted HTTPS and failed, page not authenticated
    }

    // We need to subclass EditText so we can intercept BACK key presses.
    static class UrlBar extends VerticallyFixedEditText {
        private LocationBar mLocationBar;

        /** The contents of the URL that precede the path/query after being simplified. */
        private String mSimplifiedUrlLocation;
        /** The contents of the URL that precede the path/query before simplification. */
        private String mOriginalUrlLocation;
        /** The last visible omnibox suggestions prior them to being dismissed. */
        private ArrayList<OmniboxPopupItem> mDismissedSuggestionItems
                = new ArrayList<OmniboxPopupItem>();
        private TextWatcher mLocationBarTextWatcher;

        // This gesture detector is used to tell when the url bar
        // is being scrolled, and turns focusable off, so that the url
        // bar isn't focused as a response to a scroll. b/5906051
        private GestureDetector mScrollingGestureDetector;

        public UrlBar(Context context, AttributeSet attrs) {
            super(context, attrs);

            setTextColor(context.getResources().getColor(R.color.locationbar_default_text));

            mScrollingGestureDetector = new GestureDetector(
                    getContext(), new GestureDetector.SimpleOnGestureListener() {
                @Override
                public boolean onDown(MotionEvent e) {
                    setFocusableInTouchMode(true);
                    return super.onDown(e);
                }

                @Override
                public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX,
                        float distanceY) {
                    setFocusableInTouchMode(false);
                    return super.onScroll(e1, e2, distanceX, distanceY);
                }
            });
        }

        @Override
        public View focusSearch(int direction) {
            if (direction == View.FOCUS_BACKWARD
                    && mLocationBar.mToolbar.getCurrentView() != null) {
                return mLocationBar.mToolbar.getCurrentView();
            } else {
                return super.focusSearch(direction);
            }
        }

        @Override
        public boolean onKeyPreIme(int keyCode, KeyEvent event) {
            if (keyCode == KeyEvent.KEYCODE_BACK) {
                if (event.getAction() == KeyEvent.ACTION_DOWN
                        && event.getRepeatCount() == 0) {
                    // Tell the framework to start tracking this event.
                    getKeyDispatcherState().startTracking(event, this);
                    return true;
                } else if (event.getAction() == KeyEvent.ACTION_UP) {
                    getKeyDispatcherState().handleUpEvent(event);
                    if (event.isTracking() && !event.isCanceled()) {
                        mLocationBar.backKeyPressed();
                        return true;
                    }
                }
            }
            return super.onKeyPreIme(keyCode, event);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            mScrollingGestureDetector.onTouchEvent(event);
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                if (mLocationBar.mDeleteButton.getVisibility() == VISIBLE) {
                    float eventX = getLeft() + event.getX();
                    if ((getRight() - eventX) < 15) {
                        // This touch event is happening close to the right edge of
                        // the URL bar, let's simulate a click on the delete button.
                        mLocationBar.onClick(mLocationBar.mDeleteButton);
                        return true;
                    }
                }

                // Make sure to hide the current ChromeView ActionBar.
                ChromeView view = mLocationBar.mToolbar.getCurrentView();
                if (view != null) ChromeViewDebug.hideContextualActionBar(view);
            }
            return super.onTouchEvent(event);
        }

        void setLocationBar(LocationBar locationBar) {
            mLocationBar = locationBar;
        }

        @Override
        public boolean onTextContextMenuItem(int id) {
            if (mOriginalUrlLocation == null || mSimplifiedUrlLocation == null) {
                return super.onTextContextMenuItem(id);
            }

            String currentUrl = getText().toString();
            int selectedStartIndex = getSelectionStart();
            int selectedEndIndex = getSelectionEnd();

            // Calling super.onTextContextMenuItem will delete the text when a "cut" is being
            // performed.  So, we get the current text before calling the parent method to ensure
            // it's available below.
            String currentText = getText().toString();
            boolean returnVal = super.onTextContextMenuItem(id);

            if (selectedStartIndex == 0) {
                if (id == android.R.id.cut || id == android.R.id.copy) {
                    if (currentText.startsWith(mSimplifiedUrlLocation)) {
                        currentText = mOriginalUrlLocation
                                + currentText.substring(mSimplifiedUrlLocation.length());
                    }
                    ClipboardManager clipboard = (ClipboardManager) getContext().
                            getSystemService(Context.CLIPBOARD_SERVICE);
                    clipboard.setPrimaryClip(ClipData.newPlainText(
                            null, currentText));
                }
            }

            return returnVal;
        }

        /**
         * Sets the text content of the URL bar.
         *
         * @param url The URL (or generic text) to be set in the location bar
         * @param simplify Whether the passed in URL should be simplified for better user
         *         readibility.
         * @return Whether the visible text has changed.
         */
        public boolean setUrl(String url, boolean simplify) {
            String simplifiedUrl = url;
            if (simplify) {
                simplifiedUrl = simplifyUrlForDisplay(url, false);
                try {
                    URL javaUrl = new URL(url);
                    mSimplifiedUrlLocation =
                            getUrlContentsPrePath(simplifiedUrl, javaUrl.getHost());
                    mOriginalUrlLocation =
                            getUrlContentsPrePath(url, javaUrl.getHost());
                } catch (MalformedURLException mue) {
                    mOriginalUrlLocation = null;
                    mSimplifiedUrlLocation = null;
                }
            } else {
                mOriginalUrlLocation = null;
                mSimplifiedUrlLocation = null;
            }

            Editable previousText = getEditableText();
            setText(simplifiedUrl);

            if (!isFocused()) scrollToTLD();

            return !TextUtils.equals(previousText, getEditableText());
        }

        /**
         * Autocompletes the text on the url bar and selects the text that was not entered by the
         * user. Using append() instead of setText() to preserve the soft-keyboard layout.
         * @param userText user The text entered by the user.
         * @param inlineAutocompleteText The suggested autocompletion for the user's text.
         */
        public void setAutocompleteText(String userText, String inlineAutocompleteText) {
            mLocationBar.mAutocompleteIndex = userText.length();
            if (TextUtils.equals(getText(), userText + inlineAutocompleteText)) return;
            mLocationBar.mIgnoreURLBarModification = true;
            append(inlineAutocompleteText);
            mLocationBar.mSpanList.clear();
            mLocationBar.updateSnapshotLabelVisibility();
            setSelection(mLocationBar.mAutocompleteIndex, getText().length());
            mLocationBar.mIgnoreURLBarModification = false;
        }

        private void scrollToTLD() {
            Editable url = getText();
            if (url == null || url.length() < 1) return;
            String urlString = url.toString();
            URL javaUrl;
            try {
                javaUrl = new URL(urlString);
            } catch (MalformedURLException mue) {
                return;
            }
            String host = javaUrl.getHost();
            if (host == null || host.isEmpty()) return;
            int hostStart = urlString.indexOf(host);
            int hostEnd = hostStart + host.length();
            setSelection(hostEnd);
        }

        @Override
        public void setText(CharSequence text, BufferType type) {
            // Avoid setting the same text to the URL bar as it will mess up the scroll/cursor
            // position.
            // Setting the text is also quite expensive, so only do it when the text has changed
            // (since we apply spans when the URL is not focused, we only optimize this when the
            // URL is being edited).
            if (TextUtils.equals(getEditableText(), text)) return;
            super.setText(text, type);
        }

        /**
         * Returns the portion of the URL that precedes the path/query section of the URL.
         *
         * @param url The url to be used to find the preceding portion.
         * @param host The host to be located in the URL to determine the location of the path.
         * @return The URL contents that precede the path (or the passed in URL if the host is
         *         not found).
         */
        private static String getUrlContentsPrePath(String url, String host) {
            String urlPrePath = url;
            int hostIndex = url.indexOf(host);
            if (hostIndex >= 0) {
                int pathIndex = url.indexOf('/', hostIndex);
                if (pathIndex > 0) {
                    urlPrePath = url.substring(0, pathIndex);
                } else {
                    urlPrePath = url;
                }
            }
            return urlPrePath;
        }

        InputConnectionWrapper mInputConnection = new InputConnectionWrapper(null, true) {
            private char[] mTempSelectionChar = new char[1];

            @Override
            public boolean commitText(CharSequence text, int newCursorPosition) {
                Editable currentText = getEditableText();
                if (currentText == null) return super.commitText(text, newCursorPosition);

                int selectionStart = Selection.getSelectionStart(currentText);
                int selectionEnd = Selection.getSelectionEnd(currentText);
                // If the text being committed is a single character that matches the next character
                // in the selection (assumed to be the autocomplete text), we only move the text
                // selection instead clearing the autocomplete text causing flickering as the
                // autocomplete text will appear once the next suggestions are received.
                //
                // To be confident that the selection is an autocomplete, we ensure the selection
                // is at least one character and the end of the selection is the end of the
                // currently entered text.
                if (newCursorPosition == 1 && selectionStart > 0 && selectionStart != selectionEnd
                        && selectionEnd >= currentText.length()
                        && mLocationBar.mAutocompleteIndex == selectionStart
                        && text.length() == 1) {
                    currentText.getChars(selectionStart, selectionStart + 1, mTempSelectionChar, 0);
                    if (mTempSelectionChar[0] == text.charAt(0)) {
                        if (mLocationBarTextWatcher != null) {
                            mLocationBarTextWatcher.beforeTextChanged(currentText, 0, 0, 0);
                        }
                        UrlBar.this.setSelection(selectionStart + 1, selectionEnd);
                        if (mLocationBarTextWatcher != null) {
                            mLocationBarTextWatcher.afterTextChanged(currentText);
                        }
                        mLocationBar.mAutocompleteIndex = selectionStart + 1;

                        return true;
                    }
                }
                return super.commitText(text, newCursorPosition);
            }
        };

        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            mInputConnection.setTarget(super.onCreateInputConnection(outAttrs));
            return mInputConnection;
        }
    }


    /**
     * @return The UrlBar for this LocationBar.
     */
    UrlBar getUrlBar() {
        return mUrlBar;
    }

    static class LoadProgressSimulator implements Runnable {
        private static int PROGRESS_INCREMENT = 10;
        private static int PROGRESS_INCREMENT_DELAY_MS = 10;

        private int mProgress = 0;
        private LocationBar mLocationBar;

        public LoadProgressSimulator(LocationBar locationBar) {
            mLocationBar = locationBar;
        }

        public void start() {
            mProgress = 0;
            run();
        }

        void cancel() {
            // Setting the progress to more than 100 causes run() to return
            // immediately and stops the setLoadProgress calls.
            mProgress = 101;
        }

        @Override
        public void run() {
            if (mProgress > 100) {
                return;
            }
            mProgress += PROGRESS_INCREMENT;
            mLocationBar.setLoadProgress(mProgress > 100 ? 100 : mProgress);
            mLocationBar.mUiHandler.postDelayed(this, PROGRESS_INCREMENT_DELAY_MS);
        }
    }

    /**
     * A listener that gets notified when the text is changed in the location bar as a result of
     * an user action.
     */
    public interface TextChangeListener {
        public void locationBarTextChanged(String text);
    }

    /**
     * A popup window designed for showing a list of omnibox suggestions.
     */
    class SuggestionsPopupWindow extends ListPopupWindow {
        private int mSuggestionHeight;

        private int mListItemCount;
        private int[] mAnchorPosition = new int[2];
        private int mPreviousAnchorYPosition;
        private Rect mPreviousAppSize = new Rect();
        private Rect mTempAppSize = new Rect();

        /**
         * Constructs a new popup window designed for containing omnibox suggestions.
         * @param context Context used for contained views.
         */
        public SuggestionsPopupWindow(Context context) {
            super(context, null, android.R.attr.autoCompleteTextViewStyle);
            mSuggestionHeight = context.getResources().getDimensionPixelOffset(
                    R.dimen.omnibox_suggestion_height);
        }

        @Override
        public void show() {
            if (!isShowing()) {
                mListItemCount = -1;
            } else if (mListItemCount == getListView().getCount()) {
                // If the list has not changed, as determined by the following not changing:
                //   * The number of items in the list.
                //   * The visible application viewport.
                //   * The position of the anchor view.
                // Calling show is unnecessary and does a lot of calculations and inflates
                // views unnecessarily.
                ((Activity) getContext()).getWindow().getDecorView()
                        .getWindowVisibleDisplayFrame(mTempAppSize);
                getAnchorView().getLocationInWindow(mAnchorPosition);
                if (mPreviousAnchorYPosition == mAnchorPosition[1]
                        && mTempAppSize.equals(mPreviousAppSize)) {

                    // Ensure that the list is using all the necessary/available space for
                    // displaying before exiting early.
                    //
                    // Without this check, the list would occasionally not resize correctly.  This
                    // was manifesting itself when you selected the omnibox on the phone and
                    // triggered a voice input intent.  The list contained three elements, but the
                    // popup was only sized to show a single element.
                    int availableViewport = mTempAppSize.bottom
                            - mAnchorPosition[1] - getAnchorView().getMeasuredHeight();
                    int minimumFullListSize = mListItemCount * mSuggestionHeight;
                    if (getListView().getHeight() >= minimumFullListSize
                            || getListView().getHeight() >= availableViewport) {
                        return;
                    }
                }
            }
            // The list popup will attempt to resize and expand over the keyboard.  We want
            // to prevent that from happening, so we force the popup to always assume that a
            // keyboard is necessary and that it should be drawn behind it.
            setInputMethodMode(INPUT_METHOD_NEEDED);
            super.show();
            mListItemCount = getListView().getCount();
            ((Activity) getContext()).getWindow().getDecorView()
                    .getWindowVisibleDisplayFrame(mPreviousAppSize);

            getAnchorView().getLocationInWindow(mAnchorPosition);
            mPreviousAnchorYPosition = mAnchorPosition[1];
        }

        /**
         * Invalidates all of the suggestion views in the popup.  Only applicable when the popup
         * is shown.
         */
        public void invalidateSuggestionViews() {
            if (!isShowing()) return;
            ListView suggestionsList = mSuggestionListPopup.getListView();
            for (int i = 0; i < suggestionsList.getChildCount(); i++) {
                if (suggestionsList.getChildAt(i) instanceof SuggestionView) {
                    suggestionsList.getChildAt(i).invalidate();
                }
            }
        }
    }

    private ChromeNotificationHandler mNotificationHandler = new ChromeNotificationHandler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what != ChromeNotificationCenter.VOICE_SEARCH_RESULTS
                    || mAutocomplete == null) return;
            VoiceResult topResult = mAutocomplete.onVoiceResults(msg.getData());
            if (topResult != null && !TextUtils.isEmpty(topResult.getMatch())) {
                String topResultQuery = topResult.getMatch();
                if (topResult.getConfidence() >= VOICE_SEARCH_CONFIDENCE_NAVIGATE_THRESHOLD) {
                    String url =
                            AutocompleteController.nativeQualifyPartialURLQuery(topResultQuery);
                    if (url == null) url = getUrlForSearchQuery(topResultQuery);
                    loadUrl(url, ChromeView.PAGE_TRANSITION_TYPED);
                } else {
                    setSearchQuery(topResultQuery);
                }
            }
        }
    };

    /**
     * Finds the default search engine for the current profile and returns the url query
     * {@link String} for {@code query}.
     * @param query The {@link String} that represents the text query the search url should
     *         represent.
     * @return A {@link String} that contains the url of the default search engine with
     *         {@code query} inserted as the search parameter.
     */
    public static String getUrlForSearchQuery(String query) {
        return nativeGetUrlForSearchQuery(query);
    }

    public LocationBar(Context context, AttributeSet attrs) {
        super(context, attrs);

        mStartSchemeDefaultColor = context.getResources().getColor(
                R.color.locationbar_start_scheme_default);
        mStartSchemeSecurityWarningColor = context.getResources().getColor(
                R.color.locationbar_start_scheme_security_warning);
        mStartSchemeSecurityErrorColor = context.getResources().getColor(
                R.color.locationbar_start_scheme_security_error);
        mStartSchemeEvSecureColor = context.getResources().getColor(
                R.color.locationbar_start_scheme_ev_secure);
        mStartSchemeSecureColor = context.getResources().getColor(
                R.color.locationbar_start_scheme_secure);
        mSchemeToDomainColor = context.getResources().getColor(
                R.color.locationbar_scheme_to_domain);
        mDomainAndRegistryColor = context.getResources().getColor(
                R.color.locationbar_domain_and_registry);
        mTrailingUrlColor = context.getResources().getColor(R.color.locationbar_trailing_url);

        mLoadProgress = 0;
        LayoutInflater.from(context).inflate(R.layout.location_bar, this, true);
        mNavigationButton = (ImageView) findViewById(R.id.navigation_button);
        assert mNavigationButton != null : "Missing navigation type view.";
        mNavigationButtonType = NavigationButtonType.GLOBE;

        mSecurityButton = (ImageButton) findViewById(R.id.security_button);
        mSecurityIconType = SecurityLevel.NONE;

        mSnapshotPrefixLabel = (TextView) findViewById(R.id.snapshot_prefix_label);
        assert mSnapshotPrefixLabel != null : "Missing snapshot prefix view.";

        mDeleteButton = (ImageButton) findViewById(R.id.delete_button);

        mUrlBar = (UrlBar) findViewById(R.id.url_bar);
        mUrlBar.setHint(R.string.type_to_search);
        // The HTC Sense IME will attempt to autocomplete words in the Omnibox when Prediction is
        // enabled.  We want to disable this feature and rely on the Omnibox's implementation.
        // Their IME does not respect ~TYPE_TEXT_FLAG_AUTO_COMPLETE nor any of the other InputType
        // options I tried, but setting the filter variation prevents it.  Sadly, it also removes
        // the .com button, but the prediction was buggy as it would autocomplete words even when
        // typing at the beginning of the omnibox text when other content was present (messing up
        // what was previously there).  See bug: http://b/issue?id=6200071
        if (Build.MANUFACTURER.equalsIgnoreCase("htc")) {
            mUrlBar.setInputType(mUrlBar.getInputType()
                    | InputType.TYPE_TEXT_VARIATION_FILTER);
        }
        mUrlBar.setLocationBar(this);

        mSuggestionItems = new ArrayList<OmniboxPopupItem>();
        mSuggestionListAdapter = new OmniboxPopupAdapter(getContext(), this, mSuggestionItems);

        mMicButton = (ImageButton) findViewById(R.id.mic_button);

        mUiHandler = new Handler();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mUrlBar.setCursorVisible(false);
        mNavigationButton.setVisibility(VISIBLE);
        mSecurityButton.setVisibility(INVISIBLE);

        setLayoutTransition(null);

        AnimatorListenerAdapter iconChangeAnimatorListener = new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                if (animation == mSecurityButtonShowAnimator) {
                    mNavigationButton.setVisibility(INVISIBLE);
                } else if (animation == mNavigationIconShowAnimator) {
                    mSecurityButton.setVisibility(INVISIBLE);
                }
            }

            @Override
            public void onAnimationStart(Animator animation) {
                if (animation == mSecurityButtonShowAnimator) {
                    mSecurityButton.setVisibility(VISIBLE);
                } else if (animation == mNavigationIconShowAnimator) {
                    mNavigationButton.setVisibility(VISIBLE);
                }
            }
        };

        mSecurityButtonShowAnimator = new AnimatorSet();
        mSecurityButtonShowAnimator.playTogether(
                ObjectAnimator.ofFloat(mNavigationButton, ALPHA_ANIM_PROPERTY, 0),
                ObjectAnimator.ofFloat(mSecurityButton, ALPHA_ANIM_PROPERTY, 1));
        mSecurityButtonShowAnimator.setDuration(URL_FOCUS_CHANGE_ANIMATION_DURATION_MS);
        mSecurityButtonShowAnimator.addListener(iconChangeAnimatorListener);

        mNavigationIconShowAnimator = new AnimatorSet();
        mNavigationIconShowAnimator.playTogether(
                ObjectAnimator.ofFloat(mNavigationButton, ALPHA_ANIM_PROPERTY, 1),
                ObjectAnimator.ofFloat(mSecurityButton, ALPHA_ANIM_PROPERTY, 0));
        mNavigationIconShowAnimator.setDuration(URL_FOCUS_CHANGE_ANIMATION_DURATION_MS);
        mNavigationIconShowAnimator.addListener(iconChangeAnimatorListener);
    }

    @Override
    protected void onDetachedFromWindow() {
        ChromeNotificationCenter.unregisterForNotifications(NOTIFICATIONS, mNotificationHandler);
    }

    @Override
    public void onNativeLibraryReady() {
        mNativeLocationBar = nativeInit();

        mSecurityButton.setOnClickListener(this);
        mMicButton.setOnClickListener(this);
        mDeleteButton.setOnClickListener(this);

        mUrlBar.setOnKeyListener(new OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (KeyNavigationUtil.isGoDown(event)
                        && mSuggestionListPopup != null
                        && mSuggestionListPopup.isShowing()) {
                    if (mSuggestionListPopup.getListView().getSelectedItemPosition()
                            == ListView.INVALID_POSITION) {
                        // When clearing the selection after a text change, state is not reset
                        // correctly so hitting down again will cause it to start from the previous
                        // selection point. We still have to send the key down event to let the list
                        // view items take focus, but then we select the first item explicitly.
                        boolean result = mSuggestionListPopup.onKeyDown(keyCode, event);
                        mSuggestionListPopup.setSelection(0);
                        return result;
                    } else {
                        return mSuggestionListPopup.onKeyDown(keyCode, event);
                    }
                } else if (KeyNavigationUtil.isGoUp(event)
                        && mSuggestionListPopup != null
                        && mSuggestionListPopup.isShowing()) {
                    return mSuggestionListPopup.onKeyDown(keyCode, event);
                } else if (KeyNavigationUtil.isGoRight(event)
                        && mSuggestionListPopup != null
                        && mSuggestionListPopup.isShowing()
                        && mSuggestionListPopup.getListView().getSelectedItemPosition()
                                != ListView.INVALID_POSITION) {
                    OmniboxPopupItem selectedItem =
                            (OmniboxPopupItem) mSuggestionListAdapter.getItem(
                                    mSuggestionListPopup.getListView().getSelectedItemPosition());
                    // Set the UrlBar text to empty, so that it will trigger a text change when we
                    // set the text to the suggestion again.
                    setUrlBarText("", false);
                    mUrlBar.setText(selectedItem.getSuggestion().getDisplayText());
                    mSuggestionListPopup.clearListSelection();
                    mUrlBar.setSelection(mUrlBar.getText().length());
                    return true;
                }

                if (LocationBar.this.getVisibility() != VISIBLE
                        || !KeyNavigationUtil.isEnter(event)) {
                    return false;
                }
                String urlText = mUrlBar.getText().toString();

                String suggestionMatchUrl = null;
                int suggestionMatchTransition = ChromeView.PAGE_TRANSITION_TYPED;
                if (mSuggestionListPopup != null
                        && mSuggestionListPopup.isShowing()
                        && mSuggestionListPopup.getListView().getSelectedItemPosition()
                        != ListView.INVALID_POSITION) {
                    OmniboxPopupItem selectedItem =
                            (OmniboxPopupItem) mSuggestionListAdapter.getItem(
                                    mSuggestionListPopup.getListView().getSelectedItemPosition());
                    suggestionMatchUrl = selectedItem.getSuggestion().getUrl();
                    suggestionMatchTransition = selectedItem.getSuggestion().getTransition();
                } else {
                    OmniboxSuggestion suggestionMatch =
                            findMatchingSuggestion(urlText, mSuggestionItems);
                    if (suggestionMatch == null) {
                        suggestionMatch = findMatchingSuggestion(
                                urlText, mUrlBar.mDismissedSuggestionItems);
                    }
                    if (suggestionMatch != null) {
                        // It's important to use the page transition from the suggestion or we might
                        // end up saving generated URLs as typed URLs, which would then pollute the
                        // subsequent omnibox results.
                        suggestionMatchUrl = suggestionMatch.getUrl();
                        suggestionMatchTransition = suggestionMatch.getTransition();
                    }
                    if (suggestionMatchUrl == null &&
                            ((mUrlBar.mSimplifiedUrlLocation != null
                                    && urlText.startsWith(mUrlBar.mSimplifiedUrlLocation)) ||
                             (mUrlBar.mOriginalUrlLocation != null
                                    && urlText.startsWith(mUrlBar.mOriginalUrlLocation)))) {
                        suggestionMatchUrl = urlText;
                    }
                    if (suggestionMatchUrl == null && getCurrentTab() != null
                            && TextUtils.equals(urlText, getCurrentTab().getUrl())) {
                        suggestionMatchUrl = urlText;
                    }
                    if (suggestionMatchUrl != null) {
                        if (!suggestionMatchUrl.isEmpty()) {
                            suggestionMatchUrl = URLUtilities.fixUrl(suggestionMatchUrl);
                        }
                    } else {
                        suggestionMatchUrl = nativeGetUrlForSearchQuery(urlText);
                    }
                }
                AndroidUtils.hideKeyboard(mUrlBar);
                loadUrl(suggestionMatchUrl, suggestionMatchTransition);
                return true;
            }

            /**
             * Iterates through a list of omnibox suggestions and attempts to find a suggestion
             * that matches the currently entered URL text.
             *
             * @param urlText The currently entered URL text.
             * @param suggestionItems The suggestion items to search through.
             * @return A matching suggestion from one of the suggestions or null if one is not
             *         found.
             */
            private OmniboxSuggestion findMatchingSuggestion(String urlText,
                    List<OmniboxPopupItem> suggestionItems) {
                // Sometimes the best suggestion does not actually match the user typed text
                // (i.e. about:version -> chrome://version), so if the suggestions are valid for
                // the user typed text and the first entry is a URL, we take it before iterating
                // through the rest.
                if (!suggestionItems.isEmpty()
                        && shouldAutocomplete()
                        && TextUtils.equals(urlText, suggestionItems.get(0).getMatchedQuery())) {
                    return suggestionItems.get(0).getSuggestion();
                }
                // For search terms and non URLs, we have to retrieve the actual URL
                // we should navigate to from the suggestion.
                // If autocomplete is off (because !shouldAutocomplete()), the match may not be
                // for the first suggestion. So we check them all.
                for (OmniboxPopupItem item : suggestionItems) {
                    OmniboxSuggestion suggestion = item.getSuggestion();
                    if (urlTextMatchesSuggestion(urlText, suggestion)) {
                        return suggestion;
                    }
                }
                return null;
            }

            /**
             * Determines whether the entered URL text applies to a specific omnibox suggestion.
             * @param urlText The currently entered URL text.
             * @param suggestion The omnibox suggestion to match against.
             * @return Whether the URL text matches the specified omnibox suggestion.
             */
            private boolean urlTextMatchesSuggestion(String urlText, OmniboxSuggestion suggestion) {
                Set<String> urlCandidates = new HashSet<String>();
                populatePotentialUrlCandidates(urlCandidates, urlText);

                Set<String> suggestionCandidates = new HashSet<String>();
                if (suggestion.isUrlSuggestion()) {
                    populatePotentialUrlCandidates(
                            suggestionCandidates, suggestion.getDisplayText());
                    populatePotentialUrlCandidates(suggestionCandidates, suggestion.getUrl());
                } else {
                    suggestionCandidates.add(suggestion.getDisplayText());
                }

                for (String urlCandidate : urlCandidates) {
                    if (suggestionCandidates.contains(urlCandidate)) return true;
                }
                return false;
            }

            /**
             * Populate the candidates set with all possible permutations of the text that we might
             * expect.
             * @param candidates The candidates to be populated.
             * @param url The URL to base the permutations on.
             */
            private void populatePotentialUrlCandidates(Set<String> candidates, String url) {
                if (url == null) return;
                candidates.add(url);
                candidates.add(simplifyUrlForDisplay(url, false));
                candidates.add(simplifyUrlForDisplay(url, true));
            }
        });
        TextWatcher textWatcher = new TextWatcher() {
            @Override
            public void afterTextChanged(Editable editableText) {
                updateDeleteButtonVisibility();
                updateNavigationButton();

                if (mIgnoreURLBarModification) return;

                if (!isInTouchMode() && mSuggestionListPopup != null) {
                    mSuggestionListPopup.clearListSelection();
                }

                final String text = (editableText == null) ? null : editableText.toString();
                notifyTextChanged(text);

                // Since the text has changed, the autocomplete is now most likely invalid.
                mAutocompleteIndex = -1;

                mAutocomplete.stop(false);
                if (TextUtils.isEmpty(text)) {
                    hideSuggestions();
                } else {
                    assert mRequestSuggestions == null : "Multiple omnibox requests in flight.";
                    mRequestSuggestions = new Runnable() {
                        @Override
                        public void run() {
                            mAutocomplete.start(text, mLastUrlEditWasDelete);
                            mRequestSuggestions = null;
                        }
                    };
                    postDelayed(mRequestSuggestions, OMNIBOX_SUGGESTION_START_DELAY_MS);
                }

                // Occasionally, was seeing the selection in the URL not being cleared during
                // very rapid editing.  This is here to hopefully force a selection reset during
                // deletes.
                if (mLastUrlEditWasDelete) mUrlBar.setSelection(mUrlBar.getSelectionStart());
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                if (mRequestSuggestions != null) {
                    removeCallbacks(mRequestSuggestions);
                    mRequestSuggestions = null;
                }
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // We need to determine whether the text change was triggered by a delete (so we
                // don't autocomplete of the entered text in that case). With soft-keyboard, there
                // is no way to know that the delete button was pressed, so we track text removal
                // changes.
                mLastUrlEditWasDelete = (count == 0);
            }
        };
        mUrlBar.addTextChangedListener(textWatcher);
        mUrlBar.mLocationBarTextWatcher = textWatcher;
        mUrlBar.setSelectAllOnFocus(true);
        mUrlBar.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, final boolean hasFocus) {
                onUrlFocusChange(hasFocus);
            }
        });

        mAutocomplete = new AutocompleteController(this);

        ChromeNotificationCenter.registerForNotifications(NOTIFICATIONS, mNotificationHandler);

        for (Runnable deferredRunnable : mDeferredNativeRunnables) {
            post(deferredRunnable);
        }
        mDeferredNativeRunnables.clear();
    }

    /**
     * @return Whether or not to animate icon changes.
     */
    protected boolean shouldAnimateIconChanges() {
        return mUrlHasFocus;
    }

    /**
     * Sets the autocomplete controller fo the location bar.  If one was previously set, destroy
     * will be called on it before setting the new one.
     *
     * @param controller The controller that will handle autocomplete/omnibox suggestions.
     */
    @VisibleForTesting
    public void setAutocompleteController(AutocompleteController controller) {
        if (mAutocomplete != null) {
            mAutocomplete.destroy();
        }
        mAutocomplete = controller;
    }

    private void changeLocationBarIcon(boolean showSecurityButton) {
        if (mLocationBarIconActiveAnimator != null && mLocationBarIconActiveAnimator.isRunning()) {
            mLocationBarIconActiveAnimator.cancel();
        }
        View viewToBeShown = showSecurityButton ? mSecurityButton : mNavigationButton;
        if (viewToBeShown.getVisibility() == VISIBLE && viewToBeShown.getAlpha() == 1) {
            return;
        }
        if (showSecurityButton) {
            mLocationBarIconActiveAnimator = mSecurityButtonShowAnimator;
        } else {
            mLocationBarIconActiveAnimator = mNavigationIconShowAnimator;
        }
        if (shouldAnimateIconChanges()) {
            mLocationBarIconActiveAnimator.setDuration(URL_FOCUS_CHANGE_ANIMATION_DURATION_MS);
        } else {
            mLocationBarIconActiveAnimator.setDuration(0);
        }
        mLocationBarIconActiveAnimator.start();
    }

    /**
     * Triggered when the URL input field has gained or lost focus.
     * @param hasFocus Whether the URL field has gained focus.
     */
    protected void onUrlFocusChange(boolean hasFocus) {
        mUrlHasFocus = hasFocus;
        updateDeleteButtonVisibility();
        updateSnapshotLabelVisibility();
        if (hasFocus) {
            deEmphasizeUrl();
        } else {
            hideSuggestions();

            // Focus change caused by a close-tab may result in an invalid current tab.
            if (getCurrentTab() != null) {
                setUrlToPageUrl();
                emphasizeUrl();
            }
        }

        if (mToolbar != null) mToolbar.onUrlFocusChange(hasFocus);

        changeLocationBarIcon(!hasFocus && isSecurityButtonShown());
        mUrlBar.setCursorVisible(hasFocus);

        updateContentOverlay();
    }

    /**
     * Requests focus for the URL input field and shows the keyboard.
     */
    public void requestUrlFocus() {
        mUrlBar.requestFocus();
    }

    void setToolbar(ToolbarDelegate toolbar) {
        mToolbar = toolbar;
    }

    private static NavigationButtonType suggestionTypeToNavigationButtonType(
            OmniboxSuggestion.Type suggestionType) {
        switch (suggestionType) {
            case NAVSUGGEST:
            case HISTORY_URL:
                return NavigationButtonType.GLOBE;
            case URL_WHAT_YOU_TYPED:
            case SEARCH_WHAT_YOU_TYPED:
            case SEARCH_HISTORY:
            case SEARCH_SUGGEST:
            case VOICE_SUGGEST:
            case SEARCH_OTHER_ENGINE:
            case OPEN_HISTORY_PAGE:
            // TODO(jcivelli): we should have a clock icon for history types.
            case HISTORY_TITLE:
            case HISTORY_BODY:
            case HISTORY_KEYWORD:
                return NavigationButtonType.MAGNIFIER;
            default:
                assert false;
                return NavigationButtonType.MAGNIFIER;
        }
    }

    // Updates the navigation button based on the URL string
    private void updateNavigationButton() {
        // If there are suggestions showing, show the icon for the default suggestion.
        if (!mSuggestionItems.isEmpty()) {
            setNavigationButtonType(suggestionTypeToNavigationButtonType(
                    mSuggestionItems.get(0).getSuggestion().getType()));
            return;
        }
        // Default icon is globe.
        // TODO(jcivelli): change that behavior to whatever is the final expected behavior once
        //                 I'll have talked with Roma.
        setNavigationButtonType(NavigationButtonType.GLOBE);
    }

    protected void updateSecurityIcon(SecurityLevel s) {
        if (mSecurityIconType == s) return;
        mSecurityIconType = s;
        switch (s) {
            case NONE:
                mSecurityButton.setImageBitmap(null);
                updateSecurityButton(false);
                break;
            case SECURITY_WARNING:
                mSecurityButton.setImageResource(R.drawable.omnibox_https_warning);
                break;
            case SECURITY_ERROR:
                mSecurityButton.setImageResource(R.drawable.omnibox_https_invalid);
                break;
            case SECURE:
            case EV_SECURE:
                mSecurityButton.setImageResource(R.drawable.omnibox_https_valid);
                break;
            default:
                assert false;
        }

        if (s != SecurityLevel.NONE) {
            updateSecurityButton(true);
        }
    }

    /**
     * Updates the display of the security button.
     * @param enabled Whether the security button should be displayed.
     */
    protected void updateSecurityButton(boolean enabled) {
        changeLocationBarIcon(enabled && !mUrlHasFocus);
        mSecurityButtonShown = enabled;
        mToolbar.getToolbarView().requestLayout();
    }

    /**
     * @return Whether the security button is currently being displayed.
     */
    protected boolean isSecurityButtonShown() {
        return mSecurityButtonShown;
    }

    /**
     * @return The type of the currently visible navigation button.
     */
    protected NavigationButtonType getNavigationButtonType() {
        return mNavigationButtonType;
    }

    /**
     * Sets the type of the current navigation type and updates the UI to match it.
     * @param buttonType The type of navigation button to be shown.
     */
    protected void setNavigationButtonType(NavigationButtonType buttonType) {
        if (mNavigationButtonType == buttonType) {
            return;
        }
        switch (buttonType) {
            case GLOBE:
                mNavigationButton.setImageResource(R.drawable.ic_suggestion_globe);
                break;
            case MAGNIFIER:
                mNavigationButton.setImageResource(R.drawable.ic_suggestion_magnifier);
                break;
            default:
                assert false;
        }

        if (mNavigationButton.getVisibility() != VISIBLE) {
            mNavigationButton.setVisibility(VISIBLE);
        }
        mNavigationButtonType = buttonType;
    }

    // Accessed via JNI.
    @SuppressWarnings("unused")
    private void setSecurityIcon(Bitmap bitmap) {
        mSecurityButton.setImageBitmap(bitmap);
    }

    private void updateDeleteButtonVisibility() {
        // Show the delete button on the right when the bar has focus and has some text.
        updateDeleteButton(mUrlBar.hasFocus() && (mUrlBar.getText().length() != 0));
    }

    private void updateSnapshotLabelVisibility() {
        int snapshotVisibility = mSnapshotPrefixLabel.getVisibility();
        LayoutParams layoutParams = (LayoutParams) mUrlBar.getLayoutParams();
        if (!mUrlHasFocus && getCurrentTab() != null && getCurrentTab().isShowingSnapshot()
                && mUrlBar.getText().length() > 0) {
            if (snapshotVisibility == GONE) {
                mSnapshotPrefixLabel.measure(
                        MeasureSpec.makeMeasureSpec(getMeasuredWidth(), MeasureSpec.AT_MOST),
                        MeasureSpec.makeMeasureSpec(getMeasuredHeight(), MeasureSpec.AT_MOST));
                layoutParams.leftMargin = layoutParams.leftMargin +
                        mSnapshotPrefixLabel.getMeasuredWidth();
                mUrlBar.setLayoutParams(layoutParams);
                mSnapshotPrefixLabel.setVisibility(View.VISIBLE);
            }
        } else {
            if (snapshotVisibility == VISIBLE) {
                layoutParams.leftMargin = layoutParams.leftMargin -
                        mSnapshotPrefixLabel.getMeasuredWidth();
                mUrlBar.setLayoutParams(layoutParams);
                mSnapshotPrefixLabel.setVisibility(View.GONE);
            }
        }
    }

    /**
     * Updates the display of the delete URL content button.
     * @param enabled Whether the delete button is to be enabled.
     */
    protected void updateDeleteButton(boolean enabled) {
        mDeleteButton.setVisibility(enabled ? VISIBLE : GONE);
    }

    /**
     * @return The suggestion list popup containing the omnibox results (or
     *         null if it has not yet been created).
     * @VisibleForTesting
     */
    protected SuggestionsPopupWindow getSuggestionListPopup() {
        return mSuggestionListPopup;
    }

    /**
     * Initiates the mSuggestionListPopup.  Done on demand to not slow down
     * the initial inflation of the location bar.
     */
    private void initSuggestionList() {
        if (mSuggestionListPopup != null) {
            return;
        }

        OnLayoutChangeListener suggestionListResizer = new OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                if (mSuggestionListPopup.isShowing()) {
                    updateSuggestionPopupPosition();
                }
            }
        };
        getRootView().findViewById(R.id.control_container)
                .addOnLayoutChangeListener(suggestionListResizer);
        mSuggestionListPopup = new SuggestionsPopupWindow(getContext());
        mSuggestionListPopup.setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss() {
                if (!mClearSuggestionsOnDismiss) return;

                // The popup window is auto-hidden when clicking in the location bar.  We need to
                // update our state to reflect the suggestion list being hidden.
                hideSuggestions();
            }
        });
        mSuggestionListPopup.setListSelector(null);
        mSuggestionListPopup.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        mSuggestionListPopup.setBackgroundDrawable(getSuggestionPopupBackground());
        updateSuggestionPopupPosition();
        mSuggestionListPopup.setAdapter(mSuggestionListAdapter);
        mSuggestionListAdapter.setSuggestionSelectionHandler(
                new OmniboxSuggestionSelectionHandler() {
            @Override
            public void onSelection(OmniboxSuggestion suggestion) {
                loadUrl(suggestion.getUrl(), suggestion.getTransition());
                hideSuggestions();
                AndroidUtils.hideKeyboard(mUrlBar);
            }

            @Override
            public void onRefineSuggestion(OmniboxSuggestion suggestion) {
                mUrlBar.setText(suggestion.getDisplayText());
                Selection.setSelection(mUrlBar.getText(), suggestion.getDisplayText().length());
            }

            @Override
            public void onSetUrlToSuggestion(OmniboxSuggestion suggestion) {
                setUrlBarText(suggestion.getDisplayText(), false);
                mUrlBar.setSelection(mUrlBar.getText().length());
            }
        });
    }

    /**
     * @return The view that the suggestion popup should be anchored below.
     */
    protected View getSuggestionPopupAnchorView() {
        return this;
    }

    /**
     * @return The background for the omnibox suggestions popup.
     */
    protected Drawable getSuggestionPopupBackground() {
        return getContext().getResources().getDrawable(R.drawable.textbox);
    }

    private void updateSuggestionPopupPosition() {
        if (mSuggestionListPopup == null) {
            Log.w(getClass().getSimpleName(),
                    "Calling positionSuggestionView before creating the popup.");
            return;
        }

        positionSuggestionPopup(mSuggestionListPopup);
        if (mSuggestionListPopup.isShowing()) {
            mSuggestionListPopup.show();
            mSuggestionListPopup.invalidateSuggestionViews();

            if (mForceResizeSuggestionPopupRunnable != null) {
                removeCallbacks(mForceResizeSuggestionPopupRunnable);
                mForceResizeSuggestionPopupRunnable = null;
            }

            mForceResizeSuggestionPopupRunnable = new Runnable() {
                @Override
                public void run() {
                    mForceResizeSuggestionPopupRunnable = null;

                    if (!mSuggestionListPopup.isShowing()) return;
                    if (mSuggestionListPopup.getListView().getMeasuredWidth()
                            <= mSuggestionListPopup.getWidth()) {
                        return;
                    }

                    // In some cases during rotation from landscape to portrait, the list
                    // is not properly resizing despite any number of invalidate and
                    // relayouts.  By dismissing and reshowing the popup, the list gets
                    // regenerated with the correct size.
                    mClearSuggestionsOnDismiss = false;
                    mSuggestionListPopup.dismiss();
                    mClearSuggestionsOnDismiss = true;
                    mSuggestionListPopup.show();
                }
            };
            postDelayed(mForceResizeSuggestionPopupRunnable, OMNIBOX_POPUP_RESIZE_DELAY_MS);
        }
    }

    /**
     * Position the suggestion popup.
     * @param suggestionPopup The popup to be positioned.
     */
    protected void positionSuggestionPopup(ListPopupWindow suggestionPopup) {
        suggestionPopup.setWidth(getWidth());
    }

    /**
     * Handles showing/hiding the suggestions list.
     * @param visible Whether the suggestions list should be visible.
     */
    protected void setSuggestionsListVisibility(final boolean visible) {
        mSuggestionsShown = visible;
        if (mSuggestionListPopup != null) {
            final boolean isShowing = mSuggestionListPopup.isShowing();
            if (visible && !isShowing) {
                mSuggestionListPopup.setInputMethodMode(ListPopupWindow.INPUT_METHOD_NEEDED);
                mSuggestionListPopup.setAnchorView(getSuggestionPopupAnchorView());
                updateSuggestionPopupPosition();
                mSuggestionListPopup.show();
                mSuggestionListPopup.getListView().setDivider(null);
            } else if (!visible && isShowing) {
                mSuggestionListPopup.dismiss();
            }
        }
        updateContentOverlay();
    }

    // Emphasize the TLD and second domain of the URL.
    private void emphasizeUrl() {
        if (!mSpanList.isEmpty() || mUrlBar.hasFocus()) {
            return;
        }

        Editable url = mUrlBar.getText();
        if (url.length() < 1) {
            return;
        }

        String urlString = url.toString();

        if (urlString.startsWith(Tab.CHROME_SCHEME)) {
            ForegroundColorSpan chromeSpan = new ForegroundColorSpan(mSchemeToDomainColor);
            mSpanList.add(chromeSpan);
            url.setSpan(chromeSpan, 0, Tab.CHROME_SCHEME.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

            ForegroundColorSpan domainSpan = new ForegroundColorSpan(mDomainAndRegistryColor);
            mSpanList.add(domainSpan);
            url.setSpan(domainSpan, 9, urlString.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            return;
        }

        // We retrieve the domain and registry from the full URL (the url bar shows a simplified
        // version of the URL).
        Tab currentTab = getCurrentTab();
        String tabUrl;
        if (currentTab.isShowingSnapshot() && !TextUtils.isEmpty(currentTab.getBaseUrl())) {
            tabUrl = currentTab.getBaseUrl();
        } else {
            tabUrl = currentTab.getUrl();
        }
        String domainAndRegistry = URLUtilities.getDomainAndRegistry(tabUrl);
        if (domainAndRegistry == null || "".equals(domainAndRegistry)) {
            // Could be a file URL or an invalid URL. Nothing to emphasize.
            return;
        }

        int startSchemeIndex = urlString.startsWith("https") ? 5 : 0;
        int startDomainIndex = urlString.indexOf(domainAndRegistry);
        if (startDomainIndex == -1) {
            // This can happen on a redirect causing us to update the security icon.
            // We are trying to emphasize the domain of the redirect url while the text
            // in the url bar is not updated yet.
            return;
        }
        int stopIndex = startDomainIndex + domainAndRegistry.length();
        ForegroundColorSpan span;
        if (startSchemeIndex > 0) {
            int color = mStartSchemeDefaultColor;
            SecurityLevel s = SecurityLevel.values()[nativeGetSecurityLevel(mNativeLocationBar)];
            switch (s) {
                case NONE:
                case SECURITY_WARNING:
                    color = mStartSchemeSecurityWarningColor;
                    break;
                case SECURITY_ERROR:
                    color = mStartSchemeSecurityErrorColor;
                    StrikethroughSpan ss = new StrikethroughSpan();
                    mSpanList.add(ss);
                    url.setSpan(ss, 0, startSchemeIndex,
                                Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                    break;
                case EV_SECURE:
                    color = mStartSchemeEvSecureColor;
                    break;
                case SECURE:
                    color = mStartSchemeSecureColor;
                    break;
                default:
                    assert false;
            }
            span = new ForegroundColorSpan(color);
            mSpanList.add(span);
            url.setSpan(span, 0, startSchemeIndex,
                        Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        if (startDomainIndex != 0) {
            span = new ForegroundColorSpan(mSchemeToDomainColor);
            mSpanList.add(span);
            url.setSpan(span, startSchemeIndex, startDomainIndex,
                        Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        span = new ForegroundColorSpan(mDomainAndRegistryColor);
        mSpanList.add(span);
        url.setSpan(span, startDomainIndex, stopIndex,
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        if (stopIndex < urlString.length()) {
            span = new ForegroundColorSpan(mTrailingUrlColor);
            mSpanList.add(span);
            url.setSpan(span, stopIndex, urlString.length(),
                        Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
    }

    private void deEmphasizeUrl() {
        if (mSpanList.isEmpty()) {
            return;
        }
        // Ideally we would simply call mUrlBar.getText().clearSpans() and would not have to keep
        // tracks of the spans. Sadly this triggers a bug where the cursor is not visible and the
        // EditText does not respond to key press events anymore.
        // See http://code.google.com/p/android/issues/detail?id=13101
        for (CharacterStyle span : mSpanList) {
            mUrlBar.getText().removeSpan(span);
        }
        mSpanList.clear();
    }

    private void clearSuggestions(boolean notifyChange) {
        if (mUrlBar != null && mSuggestionItems.size() > 0) {
            mUrlBar.mDismissedSuggestionItems.clear();
            mUrlBar.mDismissedSuggestionItems.addAll(mSuggestionItems);
        }
        mSuggestionItems.clear();
        // Make sure to notify the adapter. If the ListView becomes out of sync
        // with its adapter and it has not been notified, it will throw an
        // exception when some UI events are propagated.
        if (notifyChange) mSuggestionListAdapter.notifyDataSetChanged();
    }

    /**
     * Hides the omnibox suggestion popup.
     *
     * <p>
     * Signals the autocomplete controller to stop generating omnibox suggestions.
     *
     * @see AutocompleteController#stop(boolean)
     */
    public void hideSuggestions() {
        if (mAutocomplete == null) return;

        mAutocomplete.stop(true);
        mAutocompleteIndex = -1;

        setSuggestionsListVisibility(false);
        mToolbar.endPrefetch();
        clearSuggestions(true);
        updateNavigationButton();
    }

    @Override
    protected void dispatchRestoreInstanceState(SparseArray<Parcelable> container) {
        // Don't restore the state of the location bar, it can lead to all kind of bad states with
        // the popup.
        // When we restore tabs, we focus the selected tab so the URL of the page shows.
    }

    public void destroy() {
        if (mNativeLocationBar != 0) {
            nativeDestroy(mNativeLocationBar);
            mNativeLocationBar = 0;
        }
    }

    /**
     * Converts the text version of the URL to a simplified, user friendly version.
     *
     * @param url The URL to be simplified as a simple string.
     * @param removeWww Whether to remove the leading www.
     * @return The simplified version of the URL for displaying to the user.
     */
    public static String simplifyUrlForDisplay(String url, boolean removeWww) {
        if (TextUtils.isEmpty(url)) return url;

        URL javaUrl;
        try {
            javaUrl = new URL(url);
        } catch (MalformedURLException mue) {
            // We probably don't know that scheme, just use the whole URL.
            return url;
        }
        // TODO(jcivelli): remove the user info if any.
        // Remove the HTTP if there is one.
        if ("http".equals(javaUrl.getProtocol())) {
            url = url.substring(7);
        }
        if (removeWww && javaUrl.getHost().startsWith("www.")) {
            int index = url.indexOf("www.");
            url = url.substring(0, index) + url.substring(index + 4);
        }
        String query = javaUrl.getQuery();
        String ref = javaUrl.getRef();
        // Remove the trailing / on bare host names.
        if (!"file".equals(javaUrl.getProtocol()) &&
            (javaUrl.getQuery() == null || javaUrl.getQuery().isEmpty()) &&
            (javaUrl.getRef() == null || javaUrl.getRef().isEmpty()) &&
            "/".equals(javaUrl.getPath())) {
            url = url.substring(0, url.length() - 1);
        }
        return url;
    }

    /**
     * Performs a search query on the current {@link Tab}.  This calls
     * {@link #getUrlForSearchQuery(String)} to get a url based on {@code query} and loads that url
     * in the current {@link Tab}.
     * @param query The {@link String} that represents the text query that should be searched for.
     */
    public void performSearchQuery(String query) {
        if (TextUtils.isEmpty(query)) return;

        String queryUrl = getUrlForSearchQuery(query);

        if (!TextUtils.isEmpty(queryUrl)) {
            loadUrl(queryUrl, ChromeView.PAGE_TRANSITION_TYPED);
        } else {
            setSearchQuery(query);
        }
    }

    /**
     * Sets the query string in the omnibox (ensuring the URL bar has focus and triggering
     * autocomplete for the specified query) as if the user typed it.
     *
     * @param query The query to be set in the omnibox.
     */
    public void setSearchQuery(final String query) {
        if (TextUtils.isEmpty(query)) return;

        if (mNativeLocationBar == 0) {
            mDeferredNativeRunnables.add(new Runnable() {
                @Override
                public void run() {
                    setSearchQuery(query);
                }
            });
            return;
        }

        setUrlBarText(query, false);
        mUrlBar.setSelection(0, mUrlBar.getText().length());
        mUrlBar.requestFocus();
        mAutocomplete.stop(false);
        mAutocomplete.start(query, false);
        post(new Runnable() {
            @Override
            public void run() {
                AndroidUtils.showKeyboard(mUrlBar);
            }
        });
    }

    @Override
    public void onClick(View v) {
        if (v == mDeleteButton) {
            setUrlBarText("", false);
            hideSuggestions();
            updateDeleteButtonVisibility();
            return;
        } else if (v == mSecurityButton) {
            nativeOnSecurityButtonClicked(mNativeLocationBar, getContext());
        } else if (v == mMicButton) {
            UmaRecordAction.omniboxVoiceSearch();
            startVoiceRecognition();
        }
    }

    // If there is a composition (e.g. while using the Japanese IME), we must not
    // autocomplete or we'll destroy the composition.
    private boolean shouldAutocomplete() {
        Editable text = mUrlBar.getText();

        return BaseInputConnection.getComposingSpanEnd(text)
                == BaseInputConnection.getComposingSpanStart(text);
    }

    // Prerendering (also referred to as instant, and incorrectly as prefetch) makes navigations
    // faster but also uses bandwith and consumes resources making typing in the omnibox feel slow.
    // For that reason, we only enable it:
    // - when it's not explicitly disabled (via command line or bandwidth preferences).
    // - when not in incognito mode
    // - when the suggestion is a navigation (this avoids constantly prerendering search pages when
    //   the user is typing something).
    // - when Accessibility isn't turned on (http://b/6623120).
    private void conditionallyPrefetch(OmniboxSuggestion defaultSuggestion) {
        if (!CommandLine.getInstance().hasSwitch(CommandLine.DISABLE_INSTANT) &&
                !getChromeView().isIncognito() &&
                ChromePreferenceManager.getInstance(getContext()).allowPrefetch() &&
                defaultSuggestion.isUrlSuggestion() &&
                !getChromeView().isInjectingAccessibilityScript()) {
            mToolbar.prefetchUrl(defaultSuggestion.getUrl(), defaultSuggestion.getTransition());
        }
    }

    @Override
    public void onSuggestionsReceived(OmniboxSuggestion[] suggestions,
            String inlineAutocompleteText) {
        String userText = mUrlBar.getText().toString();
        if (mAutocompleteIndex != -1) {
            if (userText.length() >= mAutocompleteIndex) {
                // We need to pass to the suggestion adapter only the user text,
                // without the autocomplete text.
                userText = userText.substring(0, mAutocompleteIndex);
            } else {
                // Autocomplete text is out of sync with the user input, so discard the inline text
                // for now.
                Log.d(LOG_TAG, String.format("Invalid autocomplete index: %d, for url text: '%s'.",
                        mAutocompleteIndex, userText));
                mAutocompleteIndex = -1;
                inlineAutocompleteText = "";
                mUrlBar.setSelection(userText.length());
            }
        }

        boolean itemsChanged = false;
        boolean itemCountChanged = false;
        // If the length of the incoming suggestions matches that of those currently being shown,
        // replace them inline to allow transient entries to retain their proper highlighting.
        if (mSuggestionItems.size() == suggestions.length) {
            for (int i = 0; i < suggestions.length; i++) {
                OmniboxPopupItem suggestionItem = mSuggestionItems.get(i);
                OmniboxSuggestion suggestion = suggestionItem.getSuggestion();
                // If the suggestion has the same position and display text, then only update the
                // matched query if it is still applicable.  This is required because when the
                // user enters a new character, the previous suggestions will still be briefly
                // present until the new items are calculated.  During this time, these previous
                // items may not actually match the new search term, which would cause their
                // highlighting to be removed resulting in a flickering in the UI.  To overcome
                // this, we only update their matched query string if it is still a valid prefix
                // for the suggestion.
                if (suggestion.equals(suggestions[i])) {
                    if (suggestionItem.getMatchedQuery().equals(userText)) {
                        continue;
                    } else if (suggestion.getDisplayText().startsWith(userText) ||
                            suggestion.getUrl().contains(userText)) {
                        mSuggestionItems.set(i, new OmniboxPopupItem(suggestions[i], userText));
                        itemsChanged = true;
                    }
                } else {
                    mSuggestionItems.set(i, new OmniboxPopupItem(suggestions[i], userText));
                    itemsChanged = true;
                }
            }
        } else {
            itemsChanged = true;
            itemCountChanged = true;
            clearSuggestions(false);
            for (int i = 0; i < suggestions.length; i++) {
                mSuggestionItems.add(new OmniboxPopupItem(suggestions[i], userText));
            }
        }

        if (mSuggestionItems.isEmpty()) {
            hideSuggestions();
            return;
        }

        // Autocomplete the location bar text.
        if (!mLastUrlEditWasDelete && getChromeView() != null && shouldAutocomplete()) {
            if (mInstantTrigger != null) {
                removeCallbacks(mInstantTrigger);
                mInstantTrigger = null;
            }
            final OmniboxSuggestion instantSuggestion = suggestions[0];
            mInstantTrigger = new Runnable() {
                @Override
                public void run() {
                    mInstantTrigger = null;
                    conditionallyPrefetch(instantSuggestion);
                }
            };
            // Triggering instant can take 50-100ms, so we delay this a bit to ensure the
            // suggestions are displayed first.
            postDelayed(mInstantTrigger, OMNIBOX_SUGGESTION_START_DELAY_MS);
            if (!"".equals(inlineAutocompleteText)) {
                mUrlBar.setAutocompleteText(userText, inlineAutocompleteText);
            }
        }

        // Show the suggestion list.
        initSuggestionList();  // It may not have been initialized yet.

        if (itemsChanged) mSuggestionListAdapter.notifySuggestionsChanged();
        if (mUrlBar.hasFocus()) {
            setSuggestionsListVisibility(true);
            if (itemCountChanged) {
                // When the number of items in the list changes, sometimes the popup does not
                // actually update it's size appropriately.  So if the number of items changed,
                // forcibly trigger a resize.  It was necessary to delay the request as it was not
                // working otherwise.
                mSuggestionListPopup.getListView().postInvalidateDelayed(
                        OMNIBOX_POPUP_RESIZE_DELAY_MS);
            }
        }

        // Update the navigation button to show the default suggestion's icon.
        updateNavigationButton();
    }

    void backKeyPressed() {
        hideSuggestions();
        AndroidUtils.hideKeyboard(mUrlBar);
        // Revert the URL to match the current page.
        setUrlToPageUrl();
        // Focus the page.
        if (getChromeView() != null) getChromeView().requestFocus();
    }

    /**
     * Sets the displayed URL to be the URL of the page currently showing.
     *
     * <p>The URL is converted to the most user friendly format (removing HTTP:// for example).
     *
     * <p>If the current tab is null, the URL text will be cleared.
     */
    public void setUrlToPageUrl() {
        // If the URL is currently focused, do not replace the text they have entered with the URL.
        // Once they stop editing the URL, the current tab's URL will automatically be filled in.
        if (mUrlBar.hasFocus()) return;

        if (getCurrentTab() == null) {
            setUrlBarText("", false);
            return;
        }

        String url = getCurrentTab().getUrl().trim();
        if (url.startsWith(ChromeViewListAdapter.NTP_URL)
                || url.startsWith(ChromeViewListAdapter.WELCOME_URL)) {
            // Don't show anything for Chrome URLs.
            setUrlBarText("", false);
            return;
        }

        // Snapshots are a special case. We want to display the original page URL, which the base
        // URL is set to by WebKit.
        if (getCurrentTab().isShowingSnapshot()) {
            String baseUrl = getCurrentTab().getBaseUrl();
            // Note that we'll use the file URL if the base URL is empty. The rationale is that if
            // we load a malformed MHTML file, it is better to show the file URL than nothing.
            if (!TextUtils.isEmpty(baseUrl)) {
                url = baseUrl;
            } else if (getCurrentTab().isLoading()) {
                // If we do not have the base URL yet, it means the frame has not been committed.
                // We'll get the right URL when that happens.
                setUrlBarText("", false);
                return;
            }
        }

        setUrlBarText(url, true);
    }

    // Changes the text on the url bar. This function shouldn't be called externally other than for
    // Main in startup optimizations.
    void setUrlBarText(String text, boolean simplify) {
        mIgnoreURLBarModification = true;
        if (mUrlBar.setUrl(text, simplify)) {
            mSpanList.clear();
            updateSnapshotLabelVisibility();
        }
        mIgnoreURLBarModification = false;
    }

    public void setIgnoreURLBarModification(boolean value) {
        mIgnoreURLBarModification = value;
    }

    // Loads the |url| in the current ChromeView and gives focus to the ChromeView.
    private void loadUrl(String url, int transition) {
        // Passing in an empty string should not do anything unless the user is at the NTP.  Since
        // the NTP has no url, pressing enter while clicking on the URL bar should refresh the page
        // as it does when you click and press enter on any other site.
        if (url.isEmpty() && getChromeView() != null &&
                getChromeView().getUrl().startsWith(ChromeViewListAdapter.NTP_URL)) {
            url = getChromeView().getUrl();
        }

        boolean prefetched = mToolbar.commitPrefetchUrl(url);
        // Prevent any upcoming omnibox suggestions from showing.  We
        // must do this after commitPrefetchUrl().  The "stop"
        // call implicitly calls hideSuggestions(), which cancels
        // an outstanding instant load.
        mAutocomplete.stop(true);

        Tab currentTab = getCurrentTab();

        if (!url.isEmpty()) {
            // If we prefetched successfully we do not need an explicit load.
            if (!prefetched) {
                if (currentTab == null) {
                    assert false;
                    return;
                }
                currentTab.loadUrl(url, transition);
                setUrlToPageUrl();
            }
            UmaRecordAction.omniboxSearch();
        } else {
            setUrlToPageUrl();
        }

        ChromeView chromeView = getChromeView();
        if (chromeView != null) {
            chromeView.requestFocus();
        }
    }

    @CalledByNative
    public void updateLoadingState(boolean isLoading, boolean updateUrl) {
        if (updateUrl && !mUrlBar.hasFocus()) {  // Don't change the text if the user is editing.
            setUrlToPageUrl();
        }
        if (isLoading) {
            emphasizeUrl();
            return;
        }
        updateNavigationButton();

        SecurityLevel s = SecurityLevel.values()[nativeGetSecurityLevel(mNativeLocationBar)];
        updateSecurityIcon(s);
        if (s != SecurityLevel.NONE) {
            // Since we emphasize the schema of the URL based on the security type, we need to
            // refresh the emphasis.
            deEmphasizeUrl();
        }
        emphasizeUrl();
    }

    @CalledByNative
    public void refreshUrlStar() {
        if (mToolbar != null) mToolbar.updateButtonStatus();
    }

    /**
     * @return The load progress of the current tab (between 0 and 100).
     */
    public int getLoadProgress() {
        return mLoadProgress;
    }

    public void updateLoadProgress(int progress) {
        if (mLoadProgressSimulator != null) {
            mLoadProgressSimulator.cancel();
        }
        setLoadProgress(progress);
    }

    /**
     * Sets the load progress of the current tab.
     * @param progress The load progress between 0 and 100.
     */
    protected void setLoadProgress(int progress) {
        if (progress == mLoadProgress)
            return;

        assert progress >= 0;
        assert progress <= 100;
        mLoadProgress = progress;

        if (mLoadProgress == 100) {
            // Delay clearing the progress.
            mUiHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        if (mLoadProgress != 100) {
                            // We received a progress notification, don't clear the background.
                            return;
                        }
                        setLoadProgress(0);
                    }
                }, 200);
        }
    }

    public void clearLoadProgressIndicator() {
        setLoadProgress(0);
    }

    public void simulatePageLoadProgress() {
        if (mLoadProgressSimulator == null) {
            mLoadProgressSimulator = new LoadProgressSimulator(this);
        }
        mLoadProgressSimulator.start();
    }

    // Returns the ChromeView currently showing.
    protected Tab getCurrentTab() {
        if (mToolbar == null) return null;
        return mToolbar.getCurrentTab();
    }

    /**
     * Get the currently active ChromeView.
     *
     * <p>
     * This method is non-private to allow JNI access via subclasses.
     *
     * @return The ChromeView currently showing.
     */
    @CalledByNative
    protected ChromeView getChromeView() {
        if (mToolbar == null) return null;
        return mToolbar.getCurrentView();
    }

    /**
     * This determines which Drawable will be used to draw the content overlay.  It can be
     * overridden to change the behavior of the content overlay depending on the state of the
     * location bar and whether or not suggestions are shown.
     *
     * @param suggestionsShown Whether or not suggestions are currently being shown.
     * @return The Drawable that will be used to draw the content overlay.
     */
    protected Drawable getContentOverlayBackgroundDrawable(boolean suggestionsShown) {
        if (mUrlFocusContentOverlay == null) {
            mUrlFocusContentOverlay = new ColorDrawable(CONTENT_OVERLAY_COLOR);
        }
        return mUrlFocusContentOverlay;
    }

    /**
     * Listens for when the {@code GLUIFunctorView} underneath it in the View hierarchy has drawn
     * itself. If the ContentOverlay is visible, then it must be redrawn at the same time to prevent
     * white flashing.  http://b/issue?id=5825243
     */
    @Override
    public void onFrameRendered() {
        if (mContentOverlay != null && mContentOverlay.getVisibility() == VISIBLE) {
            mContentOverlay.invalidate();
        }
    }

    private void updateContentOverlay() {
        if (mSuggestionsShown || mUrlHasFocus) {
            if (mContentOverlay == null) {
                ViewStub overlayStub =
                        (ViewStub) getRootView().findViewById(R.id.content_overlay_stub);
                mContentOverlay = overlayStub.inflate();
                // Prevent touch events from propagating down to the chrome view.
                mContentOverlay.setOnTouchListener(new OnTouchListener() {
                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        int action = event.getActionMasked();
                        if (action == MotionEvent.ACTION_CANCEL
                                || action == MotionEvent.ACTION_UP) {
                            mUrlBar.clearFocus();
                            mContentOverlay.setVisibility(INVISIBLE);
                        }
                        return true;
                    }
                });
            }

            mContentOverlay.setBackgroundDrawable(
                    getContentOverlayBackgroundDrawable(mSuggestionsShown));

            mContentOverlay.setVisibility(VISIBLE);
        } else {
            if (mContentOverlay != null) mContentOverlay.setVisibility(INVISIBLE);
        }
    }

    public void addTextChangeListener(TextChangeListener listener) {
        if (mTextChangeListeners == null) {
            mTextChangeListeners = new ArrayList<TextChangeListener>();
        } else {
            assert(!mTextChangeListeners.contains(listener));
        }
        mTextChangeListeners.add(listener);
    }

    public void removeTextChangeListener(TextChangeListener listener) {
        assert(mTextChangeListeners.contains(listener));
        mTextChangeListeners.remove(listener);
        if (mTextChangeListeners.isEmpty()) {
            mTextChangeListeners = null;
        }
    }

    public void notifyTextChanged(String text) {
        if (mTextChangeListeners == null) return;

        for (int i = 0; mTextChangeListeners != null && i < mTextChangeListeners.size(); i++) {
            mTextChangeListeners.get(i).locationBarTextChanged(text);
        }
    }

    /**
     * @return Whether voice search is supported in the current browser configuration.
     */
    protected boolean isVoiceSearchEnabled() {
        return FeatureUtilities.isRecognitionIntentPresent(getContext());
    }

    /**
     * Call to notify the location bar that the state of the voice search microphone button may
     * need to be updated.
     */
    protected void updateMicButtonState() {
        mMicButton.setVisibility(isVoiceSearchEnabled() ? View.VISIBLE : View.GONE);
    }

    /**
     * Triggers a voice recognition intent to allow the user to specify a search query.
     */
    private void startVoiceRecognition() {
        ChromeNotificationCenter.broadcastImmediateNotification(
                ChromeNotificationCenter.VOICE_SEARCH_START_REQUEST);
    }

    private native int nativeInit();
    private native void nativeDestroy(int nativeLocationBar);

    // Triggers the showing of suggestions based on the text entered.
    private native void nativeOnSecurityButtonClicked(int nativeLocationBar, Context context);

    private native int nativeGetSecurityLevel(int nativeLocationBar);
    private static native String nativeGetUrlForSearchQuery(String query);
}
