// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.ActionBar;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.SurfaceTexture;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ResultReceiver;
import android.os.SystemClock;
import android.text.Layout.Alignment;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.view.ActionMode;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowManager;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;

import org.chromium.base.AccessedByNative;
import org.chromium.base.CalledByNative;
import org.chromium.content.browser.legacy.DownloadListener;
import org.chromium.content.browser.legacy.PluginList;
import org.chromium.content.browser.third_party.GestureDetector;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.nio.ByteBuffer;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.HashMap;
import java.util.Map;

// TODO(mkosiba): These imports shouldn't be needed after moving WebView client getters and setters
// to ChromeWebViewImpl.
import android.webkit.WebChromeClient;
import android.webkit.WebViewClient;

// Please think carefully before adding to this class any non-private methods
// which do not match exactly the legacy WebView API.

//TODO: Change this to extend AbsoluteLayout, rather than FrameLayout.
public class ChromeView extends FrameLayout implements
        ViewTreeObserver.OnGlobalFocusChangeListener,
        ViewGroup.OnHierarchyChangeListener {

    // The following constants match the ones in chrome/common/page_transition_types.h.
    // Add more if you need them.
    public static final int PAGE_TRANSITION_LINK = 0;
    public static final int PAGE_TRANSITION_TYPED = 1;
    public static final int PAGE_TRANSITION_AUTO_BOOKMARK = 2;
    public static final int PAGE_TRANSITION_START_PAGE = 6;

    // This mapping is mirrored in chrome_view.cc.  Please ensure they stay in sync.
    private static final int IDS_SAD_TAB_MESSAGE = 0;
    private static final int IDS_SAD_TAB_TITLE = 1;
    private static final int EVENT_FORWARDED_TO_NATIVE = 0;
    private static final int EVENT_NOT_FORWARDED_TO_NATIVE = 1;
    private static final int EVENT_CONVERTED_TO_CANCEL = 2;

    private static final int SCROLL_DIRECTION_UP = 0;
    private static final int SCROLL_DIRECTION_DOWN = 1;

    private static final String TAG = "ChromeView";

    /**
     * The required page load percentage for the page to be considered ready assuming the
     * TextureView is also ready.
     */
    private static final int CONSIDERED_READY_LOAD_PERCENTAGE = 100;

    // Personality of the ChromeView. This setting affects the default security policy.
    public enum Personality {
        WEB_VIEW,
        BROWSER,
    }

    // Our shared sad tab image.
    private static int gSadTabResourceId;

    // chrome_view_client.cc depends on ChromeView.java holding a ref to the current client
    // instance since the native side only holds a weak pointer to the client. We chose this
    // solution over the managed object owning the C++ object's memory since it's a lot simpler
    // in terms of clean up.
    private ChromeViewClient mChromeViewClient;

    private WebSettings mWebSettings;

    // Native pointer to C++ ChromeView object which will be set by nativeInit()
    @AccessedByNative
    private int mNativeChromeView = 0;

    private boolean mAttachedToWindow = false;

    // While in the hardware acceleration mode, we use TextureView to display the content.
    private TextureView mTextureView;

    // Default value for 60fps refresh rate.
    private long mRefreshRateInUSec = 1000000 / 60;

    // Time of next estimated VSync (when not using Choreographer).
    private long mNextEstimatedVSyncUSec;
    private VSyncTimer mVSyncTimer;

    private boolean mInVSync = false;

    // To save battery, we don't run vsync more than this time unless there is content change.
    private static final int VSYNC_RUN_PERIOD = 3000;
    private static final long MICROSECONDS_PER_SECOND = 1000000;
    private static final long MICROSECONDS_PER_MILLISECOND = 1000;
    private static final long NANOSECONDS_PER_MICROSECOND = 1000;
    private VSyncMonitor mVSyncMonitor;

    // Choreographer is used to detect vsync on >= JB. We use reflection to stay compatible with
    // earlier versions.
    private Object mChoreographer;
    private Method mChoreographerPostFrameCallback;
    private Object mFrameCallback;

    // The Choreographer gives the timebase for the actual vsync, but we don't want render until
    // we have had a chance for input events to propagate to the compositor thread. This takes
    // 3 ms typically, so we adjust the vsync timestamps forward by a bit to give input events a
    // chance to arrive. Note that this is not done when using the vsync timer, because the timer
    // timebase already includes a comparable amount of delay with respect to the real vsync.
    private static final long INPUT_EVENT_LAG_FROM_VSYNC_NSEC = 3200 * NANOSECONDS_PER_MICROSECOND;

    // TextureView is backed by SurfaceTexture. The content is not ready until the first time
    // onSurfaceTextureUpdated is called.
    private enum TextureViewStatus {
        INITIALIZING,
        READY,
        RESIZING,
        ATTACHEDINNATIVE,
    }

    // While waiting for the native surface destroyed, onDetachedFromWindow will
    // temporarily remove the mTextureView from the system hierarchy.
    private boolean mWaitForNativeSurfaceDestroy = false;

    private TextureViewStatus mTextureViewStatus = TextureViewStatus.INITIALIZING;
    // Most recent size acknowledgement by the compositor.
    private int mCompositorWidth = 0;
    private int mCompositorHeight = 0;
    // Special timestamp constant that specifies a time infinitely far in the future.
    private static final long TIMESTAMP_PENDING = Long.MAX_VALUE;
    // Timestamp for the most recent size change notification from the compositor.
    private long mCompositorResizeTimestamp = TIMESTAMP_PENDING;
    // Timestamp for the most recent frame produced by the compositor.
    private long mCompositorFrameTimestamp = 0;
    // The PlaceholderView is shown while the TextureView contents are not ready.
    private PlaceholderView mPlaceholderView;

    // Objects used in the draw path.  Essentially these are local objects,
    // but because the draw functions are called at high frequency, we put
    // them here to avoid garbage collection.
    //
    // Used in bitmapDrawImpl().
    private Paint mContentPaint;
    private Paint mBgColorPaint;
    private Region mBgColorRegion;
    private Path mBgColorPath;
    private Rect mTileRect;

    private GestureDetectorProxy mGestureDetectorProxy;
    private ScaleGestureDetector mScaleGestureDetector;
    boolean mIgnoreScaleGestureDetectorEvents = false;

    // Currently ChromeView's scrolling is handled by the native side. We keep a cached copy of the
    // scroll offset and content size so that we can display the scrollbar correctly. In the future,
    // we may switch to tile rendering and handle the scrolling in the Java level.

    // Cached scroll offset from the native
    private int mNativeScrollX;
    private int mNativeScrollY;

    // Cached content size from the native
    private int mContentWidth;
    private int mContentHeight;

    // Cached page scale factor from native
    private float mNativePageScaleFactor = 1.0f;
    private float mNativeMinimumScale = 1.0f;
    private float mNativeMaximumScale = 1.0f;

    // To avoid checkerboard, we clamp the fling velocity based on the maximum number of tiles
    // should be allowed to upload per 100ms.
    private static int MAX_NUM_UPLOAD_TILES = 12;

    // Remember whether onShowPress() is called. If it is not, in onSingleTapConfirmed()
    // we will first show the press state, then trigger the click.
    private boolean mShowPressIsCalled;

    // TODO(klobag): this is to avoid a bug in GestureDetector. With multi-touch,
    // mAlwaysInTapRegion is not reset. So when the last finger is up, onSingleTapUp()
    // will be mistakenly fired.
    private boolean mIgnoreSingleTap;

    // Does native think we are scrolling?  True from right before we
    // send the first scroll event until the last finger is raised, or
    // until after the follow-up fling has finished.  Call
    // nativeScrollBegin() when setting this to true, and use
    // tellNativeScrollingHasEnded() to set it to false.
    private boolean mNativeScrolling;

    private PopupZoomer mPopupZoomer;

    // Whether any pinch zoom event has been sent to native.
    private boolean mPinchEventSent;

    // Queue of pending motion events. If the offeredToNative() value is EVENT_FORWARDED_TO_NATIVE,
    // it means that the event has been offered to the native side but not yet acknowledged. If the
    // value is EVENT_NOT_FORWARDED_TO_NATIVE, it means the touch event has not been offered
    // to the native side and can be immediately processed. If the value is
    // EVENT_CONVERTED_TO_CANCEL, it means the native side sent a touch cancel event instead
    // of this event.
    private final Deque<PendingMotionEvent> mPendingMotionEvents =
        new ArrayDeque<PendingMotionEvent>();

    // Give up on WebKit handling of touch events when this timeout expires.
    private static final long WEBKIT_TIMEOUT_MILLIS = 200;

    // The timeout time for the most recent pending events.
    private long mWebKitTimeoutTime;

    // number of pending touch acks we should ignore.
    private int mPendingTouchAcksToIgnore = 0;

    // Disable the WebKit timeout strategy if webkit is handling the touch events.
    private boolean mDisableWebKitTimeout = false;

    // WebKit timeout handler.
    private WebKitTimeoutRunnable mWebKitTimeoutRunnable = new WebKitTimeoutRunnable();

    // Skip sending the touch events to native.
    private boolean mSkipSendToNative = false;

    // Has WebKit told us the current page requires touch events.
    private boolean mNeedTouchEvents = false;

    private static final int DOUBLE_TAP_TIMEOUT = ViewConfiguration.getDoubleTapTimeout();

    //On single tap this will store the x, y coordinates of the touch.
    private int mSingleTapX;
    private int mSingleTapY;

    // Used to track the last rawX/Y coordinates for moves.  This gives absolute scroll distance.
    // Useful for full screen tracking.
    private float mLastRawX = 0;
    private float mLastRawY = 0;

    // Cache of square of the scaled touch slop so we don't have to calculate it on every touch.
    private int mScaledTouchSlopSquare;

    // Used to track the accumulated scroll error over time. This is used to remove the
    // rounding error we introduced by passing integers to webkit.
    private float mAccumulatedScrollErrorX = 0;
    private float mAccumulatedScrollErrorY = 0;

    private Runnable mFakeMouseMoveRunnable = null;

    private FindResultReceivedListener mFindResultReceivedListener = null;

    // Only valid when focused on a text / password field.
    private final ImeAdapter mImeAdapter;

    private SelectionHandleController mSelectionHandleController;
    // These offsets in document space with page scale normalized to 1.0.
    private final PointF mStartHandleNormalizedPoint = new PointF();
    private final PointF mEndHandleNormalizedPoint = new PointF();
    private InsertionHandleController mInsertionHandleController;
    private final PointF mInsertionHandleNormalizedPoint = new PointF();
    private boolean mHideHandleTemporarilyForScroll = false;
    private boolean mHideHandleTemporarilyForPinch = false;
    private Runnable mDeferredUndoHideHandleRunnable;
    private boolean mDeferredUndoHideHandleRunnableScheduled = false;

    // Tracks whether a selection is currently active.  When applied to selected text, indicates
    // whether the last selected text is still highlighted.
    private boolean mHasSelection;
    private String mLastSelectedText;
    private boolean mSelectionEditable;
    private ActionMode mActionMode;

    private ChromeViewContextMenuInfo mContextMenuInfo;

    private DownloadListener mDownloadListener;
    private DownloadListener2 mDownloadListener2;

    private boolean mIncognito = false;

    // The following are used to dump update fps
    private boolean mLogFPS = false;
    private long mFPSCount = 0;
    private long mFPSLastTime = SystemClock.uptimeMillis();
    private boolean mProfileFPS = false;
    private long mProfileFPSCount;
    private long mProfileFPSLastTime;

    private final Handler mHandler = new Handler(Looper.getMainLooper());

    // Used for showing a temporary bitmap while the actual texture is being drawn.
    private final ArrayList<SurfaceTextureUpdatedListener> mSurfaceTextureUpdatedListeners =
            new ArrayList<SurfaceTextureUpdatedListener>();

    // Whether a physical keyboard is connected.
    private boolean mKeyboardConnected;

    private long mLastDownTime;

    /**
     * Allow a callback to be notified when the SurfaceTexture of the TextureView has been
     * updated.
     *
     * @hide Used by platform browser only.
     */
    public static interface SurfaceTextureUpdatedListener {
        /**
         * Called when the {@link SurfaceTexture} of the {@link TextureView} held in this
         * ChromeView has been updated.
         *
         * @param view The ChromeView that was updated.
         */
        public void onSurfaceTextureUpdated(ChromeView view);

        /**
         * Called when the SurfaceTexture owned by a {@link ExternalSurfaceTextureOwner} has an
         * available frame. This is not triggering if the {@link SurfaceTexture} is owned by
         * the {@link TextureView}.
         * This is a good place to trigger {@link SurfaceTexture#updateTexImage()} then
         * call {@link #onExternalSurfaceTextureUpdated()}.
         */
        public void onFrameAvailable(ChromeView view);
    }

    /** The SurfaceTexture currently being wired to the native producer. */
    private SurfaceTexture mSurfaceTextureWiredToProducer;

    /**
    * Allow an external object to own and display the {@link SurfaceTexture} instead of the
    * {@link TextureView}. The TextureView is still used to create the {@link SurfaceTexture}.
    */
    public static interface ExternalSurfaceTextureOwner {
        /**
        * Callback called when the {@link TextureView} issue a new {@link SurfaceTexture} bound
        * to the Producer.
        * @param surfaceTexture The new {@link SurfaceTexture}. It may be null when the
        *                       {@link SurfaceTexture} is not fed by the producer anymore.
        * @param isReady        Whether or not {@code surfaceTexture} is ready to be drawn.  If
        *                       {@code false}, expect a call to
        *                       {@link ExternalSurfaceTextureOwner#onSurfaceTextureIsReady()} when
        *                       {@code surfaceTexture} has a valid frame from the Producer.
        */
        public void onNewSurfaceTexture(SurfaceTexture surfaceTexture, boolean isReady);

        /**
         * Callback called when the {@link SurfaceTexture} has had a valid frame from the Producer.
         */
        public void onSurfaceTextureIsReady();
    }

    /**
    * The {@link ExternalSurfaceTextureOwner}, When set, it will hi-jack the
    * {@link SurfaceTexture} from the {@link TextureView}.
    */
    private ExternalSurfaceTextureOwner mExternalSurfaceTextureOwner;

    /*
     * Here is the snap align logic:
     * 1. If it starts nearly horizontally or vertically, snap align;
     * 2. If there is a dramatic direction change, let it go;
     *
     * Adjustable parameters. Angle is the radians on a unit circle, limited
     * to quadrant 1. Values range from 0f (horizontal) to PI/2 (vertical)
     */
    private static final float HSLOPE_TO_START_SNAP = .25f;
    private static final float HSLOPE_TO_BREAK_SNAP = .6f;
    private static final float VSLOPE_TO_START_SNAP = 1.25f;
    private static final float VSLOPE_TO_BREAK_SNAP = .6f;

    /*
     * These values are used to influence the average angle when entering
     * snap mode. If it is the first movement entering snap, we set the average
     * to the appropriate ideal. If the user is entering into snap after the
     * first movement, then we average the average angle with these values.
     */
    private static final float ANGLE_VERT = (float)(Math.PI / 2.0);
    private static final float ANGLE_HORIZ = 0f;

    /*
     *  The modified moving average weight.
     *  Formula: MAV[t]=MAV[t-1] + (P[t]-MAV[t-1])/n
     */
    private static final float MMA_WEIGHT_N = 5;

    private static final int SNAP_NONE = 0;
    private static final int SNAP_HORIZ = 1;
    private static final int SNAP_VERT = 2;
    private int mSnapScrollMode = SNAP_NONE;
    private float mAverageAngle;
    private boolean mSeenFirstScroll;

    private final AccessibilityInjector mAccessibilityInjector;
    private AccessibilityManager mAccessibilityManager;

    // Temporary notification to tell onSizeChanged to focus a form element,
    // because the OSK was just brought up.
    private boolean mFocusOnNextSizeChanged = false;
    private boolean mUnfocusOnNextSizeChanged = false;

    private boolean mNeedUpdateOrientationChanged;

    // Used to keep track of whether we should try to undo the last zoom-to-textfield operation.
    private boolean mScrolledAndZoomedFocusedEditableNode = false;

    // Helper object used to implement methods from the legacy WebView that we support only to
    // maintain backwards compatibility.
    private WebViewLegacy mWebViewLegacy;

    /**
     * Automatically decide the number of renderer processes to use based on device memory class.
     * */
    public static final int MAX_RENDERERS_AUTOMATIC = BrowserProcessMain.MAX_RENDERERS_AUTOMATIC;
    /**
     * Use single-process mode that runs the renderer on a separate thread in the main application.
     */
    public static final int MAX_RENDERERS_SINGLE_PROCESS =
            BrowserProcessMain.MAX_RENDERERS_SINGLE_PROCESS;
    /**
     * Cap on the maximum number of renderer processes that can be requested.
     */
    public static final int MAX_RENDERERS_LIMIT = BrowserProcessMain.MAX_RENDERERS_LIMIT;

    /**
     * Enable multi-process ChromeView. This should be called by the application before constructing
     * any ChromeView instances. If enabled, ChromeView will run renderers in separate processes up
     * to the number of processes specified by maxRenderProcesses. If this is not called then
     * the default is to run the renderer in the main application on a separate thread.
     *
     * @param context Context used to obtain the application context.
     * @param maxRendererProcesses Limit on the number of renderers to use. Each tab runs in its own
     * process until the maximum number of processes is reached. The special value of
     * MAX_RENDERERS_SINGLE_PROCESS requests single-process mode where the renderer will run in the
     * application process in a separate thread. If the special value MAX_RENDERERS_AUTOMATIC is
     * used then the number of renderers will be determined based on the device memory class. The
     * maximum number of allowed renderers is capped by MAX_RENDERERS_LIMIT.
     */
    public static void enableMultiProcess(Context context, int maxRendererProcesses) {
        BrowserProcessMain.initChromeViewProcess(context, maxRendererProcesses);
    }

    /**
     * Initialize the process as the platform browser. This must be called before accessing
     * ChromeView in order to treat this as a Chromium browser process.
     *
     * @param context Context used to obtain the application context.
     * @param maxRendererProcesses Same as ChromeView.enableMultiProcess()
     * @hide Only used by the platform browser.
     */
    public static void initChromiumBrowserProcess(Context context, int maxRendererProcesses) {
        BrowserProcessMain.initChromiumBrowserProcess(context, maxRendererProcesses);
    }

    // Area of last press, in document space normalized to 1.0 scale.  Used to
    // position the autofill window.
    private RectF mLastPressRect;

    private AutofillWindow mAutofillWindow;

    // Whether the renderer was crashed intentionally and we should auto-reload the page.
    private boolean mReloadOnCrash = false;

    public void setReloadOnCrash(boolean reload) {
        mReloadOnCrash = reload;
    }

    public static void registerSadTabResourceId(int id) {
        gSadTabResourceId = id;
    }

    public static void registerPopupOverlayResourceId(int id) {
        PopupZoomer.setOverlayResourceId(id);
    }

    public static void registerPopupOverlayCornerRadius(float r) {
        PopupZoomer.setOverlayCornerRadius(r);
    }

    private Bitmap getSadTabBitmap() {
        return BitmapFactory.decodeResource(getContext().getResources(), gSadTabResourceId);
    }

    // TODO(jrg): this is a hack we use since we don't have yet access to resources in the
    //            ChromeView. Remove this when we do.
    private static int mAutofillResourceId;
    private static int mAutofillNameTextViewResourceId;
    private static int mAutofillLabelTextViewResourceId;
    public static void registerAutofillResourceIDs(int resourceId,
                                                   int nameTextViewResourceId,
                                                   int labelTextViewResourceId) {
        mAutofillResourceId = resourceId;
        mAutofillNameTextViewResourceId = nameTextViewResourceId;
        mAutofillLabelTextViewResourceId = labelTextViewResourceId;
    }

    // TODO(olilan): A generic resource hack, for any further resources needed until we
    // go into framework.
    private static Map<String, Integer> mResourceIdMap = new HashMap<String, Integer>();
    public static void registerResourceId(String name, int resourceId) {
        mResourceIdMap.put(name, resourceId);
    }
    public static int getResourceId(String name) {
        return mResourceIdMap.get(name);
    }

    public ChromeView(Context context, boolean incognito, int nativeWebContents,
            Personality personality) {
        this(context, null, android.R.attr.webViewStyle, incognito, nativeWebContents,
                personality);
    }

    public ChromeView(Context context) {
        this(context, null);
    }

    public ChromeView(Context context, AttributeSet attrs) {
        // TODO(klobag): use the WebViewStyle as the default style for now. It enables scrollbar.
        // When ChromeView is moved to framework, we can define its own style in the res.
        this(context, attrs, android.R.attr.webViewStyle);
    }

    public ChromeView(Context context, AttributeSet attrs, int defStyle) {
        this(context, attrs, defStyle, false, 0, Personality.WEB_VIEW);
    }

    private ChromeView(Context context, AttributeSet attrs, int defStyle, boolean incognito,
            int nativeWebContents, Personality personality) {
        super(context, attrs, defStyle);

        WeakContext.initializeWeakContext(context);
        // By default, ChromeView will initialize single process mode. The call to
        // initChromeViewProcess below is ignored if either the ChromeView host called
        // enableMultiProcess() or the platform browser called initChromiumBrowserProcess().
        BrowserProcessMain.initChromeViewProcess(context, MAX_RENDERERS_SINGLE_PROCESS);

        initialize(context, incognito, nativeWebContents, personality);
        mImeAdapter = createImeAdapter(context);

        mAccessibilityInjector = new AccessibilityInjector(this);
        mAccessibilityInjector.addOrRemoveAccessibilityApisIfNecessary();

        mAccessibilityManager =
                (AccessibilityManager) getContext().getSystemService(Context.ACCESSIBILITY_SERVICE);
    }

    private ImeAdapter createImeAdapter(Context context) {
        return new ImeAdapter(context, getSelectionHandleController(),
                getInsertionHandleController(),
                new ImeAdapter.ViewEmbedder() {
                    @Override
                    public void onImeEvent(boolean isFinish) {
                        getChromeViewClient().onImeEvent();
                        mPopupZoomer.hide(false);
                        if (!isFinish) {
                            undoScrollFocusedEditableNodeIntoViewIfNeeded(false);
                        }
                    }

                    @Override
                    public void onSetFieldValue() {
                        scrollFocusedEditableNodeIntoView();
                    }

                    @Override
                    public void onDismissInput() {
                        closeAutofillPopup();
                    }

                    @Override
                    public View getAttachedView() {
                        return ChromeView.this;
                    }

                    @Override
                    public ResultReceiver getNewShowKeyboardReceiver() {
                        return new ResultReceiver(new Handler()) {
                            @Override
                            public void onReceiveResult(int resultCode, Bundle resultData) {
                                if (resultCode == InputMethodManager.RESULT_SHOWN) {
                                    // If OSK is newly shown, delay the form focus until
                                    // the onSizeChanged (in order to adjust relative to the
                                    // new size).
                                    mFocusOnNextSizeChanged = true;
                                } else if (resultCode ==
                                        InputMethodManager.RESULT_UNCHANGED_SHOWN) {
                                    // If the OSK was already there, focus the form immediately.
                                    scrollFocusedEditableNodeIntoView();
                                } else {
                                    undoScrollFocusedEditableNodeIntoViewIfNeeded(false);
                                }
                            }
                        };
                    }
                }
        );
    }

    /**
     * Call this before instantiating a ChromeView to restore session cookies
     * from the previous session. This should typically be done in the host
     * activity's onCreate() or onRestoreInstanceState() method. If it is not
     * called, the first ChromeView created will clear out old session cookies.
     */
    public static void restoreSessionCookies() {
        nativeRestoreSessionCookies();
    }

    public static void destroyIncognitoProfile() {
        nativeDestroyIncognitoProfile();
    }

    /**
     * Allows an external source to listen to SurfaceTexture updates.
     *
     * @hide Used by platform browser only.
     * @param listener
     */
    public void registerSurfaceTextureListener(SurfaceTextureUpdatedListener listener) {
        if (!mSurfaceTextureUpdatedListeners.contains(listener)) {
            mSurfaceTextureUpdatedListeners.add(listener);
        }
    }

    /**
     * Unregisters the current external listener that waits for SurfaceTexture updates.
     *
     * @hide Used by platform browser only.
     */
    public void unregisterSurfaceTextureListener(SurfaceTextureUpdatedListener listener) {
        mSurfaceTextureUpdatedListeners.remove(listener);
    }

    /**
     * Destroy the internal state of the WebView. This method may only be called
     * after the WebView has been removed from the view system. No other methods
     * may be called on this WebView after this method has been called.
     */
    public void destroy() {
        hidePopupDialog();
        stopVSync();
        if (mNativeChromeView != 0) {
            nativeDestroy(mNativeChromeView);
            mNativeChromeView = 0;
        }
        if (mWebSettings != null) {
            mWebSettings.destroy();
            mWebSettings = null;
        }
    }

    /**
     * Returns true initially, false after destroy() has been called.
     * It is illegal to call any other public method after destroy().
     */
    public boolean isAlive() {
        return mNativeChromeView != 0;
    }

    /**
     * For internal use. Throws IllegalStateException if mNativeChromeView is 0.
     * Use this to ensure we get a useful Java stack trace, rather than a native
     * crash dump, from use-after-destroy bugs in Java code.
     */
    void checkIsAlive() throws IllegalStateException {
        if (!isAlive()) {
            throw new IllegalStateException("ChromeView used after destroy() was called");
        }
    }

    public void setChromeViewClient(ChromeViewClient client) {
        if (client == null) {
            throw new IllegalArgumentException("The client can't be null.");
        }
        mChromeViewClient = client;
        if (mNativeChromeView != 0) {
            nativeSetClient(mNativeChromeView, mChromeViewClient);
        }
    }

    ChromeViewClient getChromeViewClient() {
        if (mChromeViewClient == null) {
            // We use the Null Object pattern to avoid having to perform a null check in this class.
            // We create it lazily because most of the time a client will be set almost immediately
            // after ChromeView is created.
            mChromeViewClient = new ChromeViewClient();
            // We don't set the native ChromeViewClient pointer here on purpose. The native
            // implementation doesn't mind a null delegate and using one is better than passing a
            // Null Object, since we cut down on the number of JNI calls.
        }
        return mChromeViewClient;
    }

    public int getBackgroundColor() {
        if (mNativeChromeView != 0) {
            return nativeGetBackgroundColor(mNativeChromeView);
        }
        return Color.WHITE;
    }

    /**
     * Inform the WebView of the network state. This is used to set the
     * JavaScript property window.navigator.isOnline and generates the
     * online/offline event as specified in HTML5, sec. 5.7.7
     *
     * @param networkUp True if network is available
     */
    public void setNetworkAvailable(boolean networkUp) {
        if (mNativeChromeView != 0) {
            nativeSetNetworkAvailable(mNativeChromeView, networkUp);
        }
    }

    /**
     * Load url without fixing up the url string. Calls from Chrome should be not
     * be using this, but should use Tab.loadUrl instead.
     * @param url The url to load.
     */
    public void loadUrlWithoutUrlSanitization(String url) {
        loadUrlWithoutUrlSanitization(url, PAGE_TRANSITION_TYPED);
    }

    /**
     * Load url without fixing up the url string. Calls from Chrome should be not
     * be using this, but should use Tab.loadUrl instead.
     * @param url The url to load.
     * @param pageTransition Page transition id that describes the action that led to this
     *        navigation. It is important for ranking URLs in the history so the omnibox can report
     *        suggestions correctly.
     */
    public void loadUrlWithoutUrlSanitization(String url, int pageTransition) {
        mAccessibilityInjector.addOrRemoveAccessibilityApisIfNecessary();
        if (mNativeChromeView != 0) {
            nativeLoadUrlWithoutUrlSanitization(mNativeChromeView, url, pageTransition);
        }
    }

    public void stopLoading() {
        if (mNativeChromeView != 0) {
            nativeStopLoading(mNativeChromeView);
        }
    }

    /**
     * Get the URL of the current page.
     *
     * @return The URL of the current page.
     */
    public String getUrl() {
        if (mNativeChromeView != 0) {
            return nativeGetURL(mNativeChromeView);
        }
        return null;
    }

    /**
     * Get the title of the current page.
     *
     * @return The title of the current page.
     */
    public String getTitle() {
        if (mNativeChromeView != 0) {
            return nativeGetTitle(mNativeChromeView);
        }
        return null;
    }

    public int getProgress() {
        if (mNativeChromeView != 0) {
            return (int) (100.0 * nativeGetLoadProgress(mNativeChromeView));
        }
        return 100;
    }

    public Bitmap getBitmap() {
        return getBitmap(getWidth(), getHeight());
    }

    public Bitmap getBitmap(int width, int height) {
        return getBitmap(width, height, Bitmap.Config.ARGB_8888, true);
    }

    public Bitmap getBitmap(int width, int height, boolean drawTextureViewOverlay) {
        return getBitmap(width, height, Bitmap.Config.ARGB_8888, drawTextureViewOverlay);
    }

    public Bitmap getBitmap(int width, int height, Bitmap.Config config,
            boolean drawTextureViewOverlay) {
        if (getWidth() == 0 || getHeight() == 0) {
            return null;
        }
        if (mTextureView != null) {
            if (!mTextureView.isAvailable()) {
                return null;
            }

            Bitmap b = Bitmap.createBitmap(width, height, config);
            // getBitmap may fail and cannot be predicted or checked. Setting the initial color
            // to the background color is the least disturbing in case of failure.
            // Note: eraseColor is fast (30x cheaper than getBitmap)
            b.eraseColor(getBackgroundColor());
            mTextureView.getBitmap(b);
            if (drawTextureViewOverlay) {
                drawTextureViewOverlay(b);
            }

            return b;
        } else {
            Bitmap b = Bitmap.createBitmap(width, height, config);
            Canvas c = new Canvas(b);
            c.scale(width / (float) getWidth(), height / (float) getHeight());
            draw(c);
            return b;
        }
    }

    /**
     * Generates a bitmap of the content that is performance optimized based on capture time.
     *
     * <p>
     * To have a consistent capture time across devices, we will scale down the captured bitmap
     * where necessary to reduce the time to generate the bitmap.
     *
     * @param width The width of the content to be captured.
     * @param height The height of the content to be captured.
     * @param drawTextureViewOverlay Whether to draw overlaid Android views on top of the generated
     *                               bitmap.
     * @return A pair of the generated bitmap, and the scale that needs to be applied to return the
     *         bitmap to it's original size (i.e. if the bitmap is scaled down 50%, this
     *         will be 2).
     */
    public Pair<Bitmap, Float> getScaledPerformanceOptimizedBitmap(
            int width, int height, boolean drawTextureViewOverlay) {
        float scale = 1f;
        // On tablets, always scale down to MDPI for performance reasons.
        // TODO(tedchoc): Share this with Chrome on Android's check.
        if (getContext().getResources().getConfiguration().smallestScreenWidthDp >= 600) {
            scale = getContext().getResources().getDisplayMetrics().density;
        }
        return Pair.create(
                getBitmap((int) (width / scale), (int) (height / scale), drawTextureViewOverlay),
                scale);
    }

    /**
     * @return Whether the ChomeView is covered by an overlay that is more than half
     *         of it's surface. This is used to determine if we need to do a slow bitmap capture or
     *         to show the ChromeView without them.
     * @hide
     */
    public boolean hasLargeOverlay() {
        if (getWidth() > 0 && getHeight() > 0 && getChildAt(0) == mTextureView) {
            int threshold = getHeight() * getWidth() / 2;
            // Starts at 1 to skip the textureView.
            for (int i = 1; i < getChildCount(); ++i) {
                View child = getChildAt(i);
                if (child.isShown()) {
                    Rect rect = getVisibilityRect(child);
                    if (rect != null && rect.width() * rect.height() >= threshold) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * Returns the supposed visible rectangle of a view. This implementation consider ViewGroup
     * only as parents and not as visible entity. This implementation also ignore ViewStub views.
     *
     * @param view The View to get the rectangle from.
     * @return The rectangle of the visible area.
     */
    private static Rect getVisibilityRect(View view) {
        if (!view.isShown()) return null;

        Rect rect = null;
        if (view instanceof android.view.ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); ++i) {
                View child = group.getChildAt(i);
                if (child.isShown()) {
                    Rect r = getVisibilityRect(child);
                    if (r != null) {
                        if (rect == null) {
                            rect = r;
                        } else {
                            rect.left = Math.min(rect.left, r.left);
                            rect.top = Math.min(rect.top, r.top);
                            rect.right = Math.max(rect.right, r.right);
                            rect.bottom = Math.max(rect.bottom, r.bottom);
                        }
                    }
                }
            }
        } else if(!(view instanceof ViewStub) && !(view instanceof PlaceholderView)) {
            rect = new Rect();
            if (!view.getGlobalVisibleRect(rect, null)) {
                rect = null;
            }
        }
        return rect;
    }

    private void drawTextureViewOverlay(Bitmap b) {
        Canvas c = new Canvas(b);
        for (int i = 1; i < getChildCount(); ++i) {
            if (getChildAt(i).isShown()) {
                drawChild(c, getChildAt(i), getDrawingTime());
            }
        }
    }

    public boolean canGoBack() {
        return mNativeChromeView != 0 && nativeCanGoBack(mNativeChromeView);
    }

    public boolean canGoForward() {
        if (mNativeChromeView != 0) {
            return nativeCanGoForward(mNativeChromeView);
        }
        return false;
    }

    public void goBack() {
        if (mNativeChromeView != 0)
            nativeGoBack(mNativeChromeView);
    }

    public void goForward() {
        if (mNativeChromeView != 0) {
            nativeGoForward(mNativeChromeView);
        }
    }

    /**
     * Reload the current page.
     */
    public void reload() {
        mAccessibilityInjector.addOrRemoveAccessibilityApisIfNecessary();
        if (mNativeChromeView != 0) {
            nativeReload(mNativeChromeView);
        }
    }

    String getSelectedText() {
        return mHasSelection ? mLastSelectedText : "";
    }

    /**
     * Start profiling the update speed. You must call {@link #stopFpsProfiling}
     * to stop profiling.
     */
    void startFpsProfiling() {
        assert !mProfileFPS;
        mProfileFPS = true;
        mProfileFPSCount = 0;
        mProfileFPSLastTime = SystemClock.uptimeMillis();
    }

    /**
     * Stop profiling the update speed.
     */
    float stopFpsProfiling() {
        assert mProfileFPS;
        mProfileFPS = false;
        return ((float) mProfileFPSCount) /
                (SystemClock.uptimeMillis() - mProfileFPSLastTime) * 1000;
    }

    /**
     * Fling the ChromeView from the current position.
     * @param x Fling touch starting position
     * @param y Fling touch starting position
     * @param velocityX Initial velocity of the fling (X) measured in pixels per second.
     * @param velocityY Initial velocity of the fling (Y) measured in pixels per second.
     */
    void fling(int x, int y, int velocityX, int velocityY) {
        if (mNativeChromeView == 0) {
            return;
        }

        endFling();

        velocityX = clampFlingVelocityX(velocityX);
        velocityY = clampFlingVelocityY(velocityY);
        nativeFlingStart(mNativeChromeView, x, y, velocityX, velocityY);
    }

    void endFling() {
        if (mNativeChromeView == 0) {
            return;
        }

        nativeFlingCancel(mNativeChromeView);
        tellNativeScrollingHasEnded();
    }

    /**
     * Start pinch zoom. You must call {@link #pinchEnd} to stop.
     */
    void pinchBegin() {
        if (mNativeChromeView != 0) {
            nativePinchBegin(mNativeChromeView);
        }
    }

    /**
     * Stop pinch zoom.
     */
    void pinchEnd() {
        if (mNativeChromeView != 0) {
            nativePinchEnd(mNativeChromeView);
        }
    }

    /**
     * Modify the ChromeView magnification level. The effect of calling this
     * method is exactly as after "pinch zoom".
     *
     * @param delta The ratio of the new magnification level over the current
     *            magnification level.
     * @param anchorX The magnification anchor (X) in the current view
     *            coordinate.
     * @param anchorY The magnification anchor (Y) in the current view
     *            coordinate.
     */
    void pinchBy(float delta, int anchorX, int anchorY) {
        if (mNativeChromeView != 0) {
            nativePinchBy(mNativeChromeView, delta, anchorX, anchorY);
        }
    }

    /**
     * Starts the find operation by calling StartFinding on the Tab. This
     * function does not block while a search is in progress. Set a listener
     * using setFindResultReceivedListener to receive the results.
     */
    public void startFinding(String searchString,
                             boolean forwardDirection,
                             boolean caseSensitive) {
        if (mSelectionHandleController != null) {
            mSelectionHandleController.hideAndDisallowAutomaticShowing();
        }
        if (mNativeChromeView != 0) {
            nativeStartFinding(mNativeChromeView, searchString, forwardDirection, caseSensitive);
        }
    }

    /**
     * Selects and zooms to the nearest find result to the point (x,y),
     * where x and y are fractions of the content document's width and height.
     */
    public void activateNearestFindResult(float x, float y) {
        if (mNativeChromeView != 0) {
            nativeActivateNearestFindResult(mNativeChromeView, x, y);
        }
    }

    /** Enum listing the possible actions to take when ending a search. */
    public enum FindSelectionAction {
        /** Translate the find selection into a normal selection. */
        KEEP_SELECTION,
        /** Clear the find selection. */
        CLEAR_SELECTION,
        /** Focus and click the selected node (for links). */
        ACTIVATE_SELECTION
    }

    /** Stops the current find operation. */
    public void stopFinding(FindSelectionAction selectionAction) {
        if (mNativeChromeView != 0) {
            nativeStopFinding(mNativeChromeView, selectionAction.ordinal());
        }
    }

    /** Returns the most recent find text before the current one. */
    public String getPreviousFindText() {
        if (mNativeChromeView != 0) {
            return nativeGetPreviousFindText(mNativeChromeView);
        }
        return null;
    }

    /** Asks the renderer to send the bounding boxes of current find matches. */
    public void requestFindMatchRects(int haveVersion) {
        if (mNativeChromeView != 0) {
            nativeRequestFindMatchRects(mNativeChromeView, haveVersion);
        }
    }

    // TODO(johnme): zoomToRendererRect should use browser coords.
    /** Zoom/pan so the rect (in renderer coords) is onscreen and readable. */
    public void zoomToRendererRect(Rect rect) {
        if (mNativeChromeView != 0) {
            nativeZoomToRendererRect(mNativeChromeView, rect.left, rect.top,
                    rect.width(), rect.height());
        }
    }

    public interface FindResultReceivedListener {

        /**
         * Java equivalent to the C++ FindNotificationDetails class
         * defined in chrome/browser/find_notification_details.h
         */
        public static class FindNotificationDetails {
            /** How many matches were found. */
            public final int numberOfMatches;
            // TODO(johnme): rendererSelectionRect should use browser coords.
            /** Where selection occurred (in renderer coordinates). */
            public final Rect rendererSelectionRect;

            /**
             * The ordinal of the currently selected match.
             *
             * In rare edge cases (e.g. b/4147049 where the m_activeMatch found
             * by WebFrameImpl::find has been removed from the DOM by the time
             * WebFrameImpl::scopeStringMatches tries to find the ordinal of the
             * active match) the activeMatchOrdinal may be -1 (even if there are
             * a positive number of matches) indicating that we failed to
             * locate/highlight the active match.
             * */
            public final int activeMatchOrdinal;
            /** Whether this is the last Find Result update. */
            public final boolean finalUpdate;

            public FindNotificationDetails(int numberOfMatches,
                                           Rect rendererSelectionRect,
                                           int activeMatchOrdinal,
                                           boolean finalUpdate) {
                this.numberOfMatches = numberOfMatches;
                this.rendererSelectionRect = rendererSelectionRect;
                this.activeMatchOrdinal = activeMatchOrdinal;
                this.finalUpdate = finalUpdate;
            }
        }

        public void onFindResultReceived(FindNotificationDetails details);
    }

    /** Register to receive the results of startFinding calls. */
    public void setFindResultReceivedListener(
            FindResultReceivedListener listener) {
        mFindResultReceivedListener = listener;
    }

    /**
     * Injects the passed JavaScript code in the current page and evaluates it.
     * Once evaluated, an asynchronous call to
     * ChromeViewClient.onJavaScriptEvaluationResult is made. Used in automation
     * tests.
     *
     * @return an id that is passed along in the asynchronous onJavaScriptEvaluationResult callback
     * @throws IllegalStateException If the ChromeView has been destroyed.
     * @hide
     */
    public int evaluateJavaScript(String script) throws IllegalStateException {
        checkIsAlive();
        return nativeEvaluateJavaScript(script);
    }

    /**
     * This method should be called when the containing activity is paused
     *
     * @hide
     **/
    public void onActivityPause() {
        TraceEvent.begin();
        hidePopupDialog();
        nativeOnHide(mNativeChromeView, true);
        setAccessibilityState(false);
        TraceEvent.end();
        stopVSync();
    }

    /**
     * This method should be called when the containing activity is resumed
     *
     * @hide
     **/
    public void onActivityResume() {
        nativeOnShow(mNativeChromeView, mTextureViewStatus != TextureViewStatus.INITIALIZING);
        setAccessibilityState(true);
    }

    /**
     * Called when the WebView is shown.
     *
     * @hide
     **/
    public void onShow() {
        nativeOnShow(mNativeChromeView, false);
        setAccessibilityState(true);
    }

    /**
     * Called when the WebView is hidden.
     *
     * @hide
     **/
    public void onHide() {
        hidePopupDialog();
        nativeOnHide(mNativeChromeView, false);
        setAccessibilityState(false);
        stopVSync();
    }

    /**
     * Return the WebSettings object used to control the settings for this
     * WebView.
     * @return A WebSettings object that can be used to control this WebView's
     *         settings.
     */
    public WebSettings getSettings() {
        if (mWebSettings == null) {
            mWebSettings = new WebSettings(this, mNativeChromeView);
        }
        return mWebSettings;
    }

    private void hidePopupDialog() {
        SelectPopupDialog.hide(this);
        hideHandles();
        hideSelectActionBar();
    }

    /**
     * @hide Used by framework only.
     */
    void hideSelectActionBar() {
        if (mActionMode != null) {
            mActionMode.finish();
        }
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }

    /**
     * Final stage of software draw path.  Called from C++ to paint the
     * tiles onto the canvas.
     */
    @CalledByNative
    private void bitmapDrawImpl(Canvas canvas, Matrix matrix, Bitmap[] tiles,
            Point[] tile_positions) {
        canvas.save(Canvas.MATRIX_SAVE_FLAG);
        canvas.concat(matrix);

        if (tiles.length != tile_positions.length) {
            Log.e(TAG, "Invalid tile list");
            canvas.restore();
            return;
        }
        TraceEvent.begin();

        // Initialize instance variables (used only here, but we want to
        // avoid making allocations on every draw to reduce GC cycles).
        if (mContentPaint == null) {
            mContentPaint = new Paint();
            mContentPaint.setFilterBitmap(true);
            mBgColorPaint = new Paint();
            mBgColorRegion = new Region();
            mBgColorPath = new Path();
            mTileRect = new Rect();
        }

        Rect clip_bounds = canvas.getClipBounds();
        mBgColorRegion.set(clip_bounds);

        // Draw content bitmaps.
        for (int i = 0; i < tiles.length; i++) {
            canvas.drawBitmap(tiles[i], tile_positions[i].x, tile_positions[i].y, mContentPaint);

            // Exclude the content area from the bg region.
            mTileRect.set(tile_positions[i].x, tile_positions[i].y,
                    tile_positions[i].x + tiles[i].getWidth(),
                    tile_positions[i].y + tiles[i].getHeight());
            mBgColorRegion.op(mTileRect, Region.Op.DIFFERENCE);
        }

        // Draw background color (if any).
        if (!mBgColorRegion.quickReject(clip_bounds)) {
            mBgColorPaint.setColor(getBackgroundColor());
            mBgColorRegion.getBoundaryPath(mBgColorPath);
            // TODO(aelias): I've heard that drawing paths is slow when in
            // hardware-accelerated mode.  Make sure it's not called (it usually
            // shouldn't, thanks to default tiles and the quickReject).
            canvas.drawPath(mBgColorPath, mBgColorPaint);

            // workaround Skia bug http://code.google.com/p/skia/issues/detail?id=356
            // otherwise the bg color tiles accumulate across each draw
            mBgColorPath.reset();
        }

        canvas.restore();
    }

    /**
     * Obtains the favicon for the current page.
     *
     * @return Favicon bitmap
     */
    public Bitmap getFavicon() {
      return nativeGetFaviconBitmap(mNativeChromeView);
    }

    /**
     * Returns whether the favicon for the current tab is valid.
     *
     * @return False if the favicon was not yet loaded for the page and the
     *     default was rendered instead.
     *
     * @hide For phone tab stack.
     */
    public boolean faviconIsValid() {
      return nativeFaviconIsValid(mNativeChromeView);
    }

    // FrameLayout overrides.

    @Override
    protected void onDraw(Canvas canvas) {
        try {
            TraceEvent.begin();

            if (mNativeChromeView == 0) return;

            boolean crashed = nativeCrashed(mNativeChromeView);
            if (!crashed) {
                if (mTextureView != null) {
                    if (!canvas.isHardwareAccelerated()) {
                        // Screenshots are drawn into a software canvas. The TextureView hardware
                        // layer will show up as a black rectangle, so draw it using a bitmap
                        // snapshot instead. We also need to hide the TextureView to avoid obscuring
                        // the snapshot. It will be made visible again once we regain the hardware
                        // canvas.
                        mTextureView.setVisibility(View.INVISIBLE);
                        Bitmap bitmap = getBitmap();
                        if (bitmap != null) {
                            canvas.drawBitmap(bitmap, 0.f, 0.f, null);
                        } else {
                            canvas.drawColor(getBackgroundColor());
                        }
                    } else if (!mTextureView.isShown()) {
                        mTextureView.setVisibility(View.VISIBLE);
                    }
                } else if (mTextureView == null) {
                    // Software path.
                    int failColor = nativeDraw(mNativeChromeView, canvas);

                    // Clear the canvas with a debug color if the draw failed.
                    if (failColor != 0) {
                        canvas.drawColor(failColor);
                    }
                    if (mLogFPS || mProfileFPS) logFps();
                }
            } else if (!mReloadOnCrash) {
                mPopupZoomer.hide(false);
                canvas.drawARGB(0xFF, 0x2E, 0x3F, 0x51);
                Bitmap sadTabImg = getSadTabBitmap();

                // Note: the sad tab image is currently set by the user.
                if (sadTabImg == null) return;

                int sadTabPaddingLeft = (canvas.getWidth() - sadTabImg.getWidth()) / 2;
                int sadTabPaddingTop = (canvas.getHeight() - 2 * sadTabImg.getHeight()) / 2;
                int sadTabPaddingRight = sadTabPaddingLeft + sadTabImg.getWidth();
                int sadTabPaddingBottom = sadTabPaddingTop + sadTabImg.getHeight();
                Rect dest =
                    new Rect(sadTabPaddingLeft,
                             sadTabPaddingTop,
                             sadTabPaddingRight,
                             sadTabPaddingBottom);
                canvas.drawBitmap(sadTabImg, null, dest, new Paint());

                String sadTabTitle =
                        nativeGetLocalizedString(mNativeChromeView, IDS_SAD_TAB_TITLE);
                String sadTabMessage =
                        nativeGetLocalizedString(mNativeChromeView, IDS_SAD_TAB_MESSAGE);
                String sadTabText = sadTabTitle + "\n\n" + sadTabMessage;
                TextPaint textPaint =
                        new TextPaint(Paint.FAKE_BOLD_TEXT_FLAG | Paint.ANTI_ALIAS_FLAG);
                textPaint.setTextSize(18);
                textPaint.setARGB(0xFF, 0xFF, 0xFF, 0xFF);
                StaticLayout textLayout = new StaticLayout(sadTabText,
                                                           textPaint,
                                                           (int)(canvas.getWidth() * 0.8f),
                                                           Alignment.ALIGN_CENTER,
                                                           1.0f,
                                                           0.0f,
                                                           false);
                Rect textBounds = new Rect();
                textPaint.getTextBounds(sadTabTitle, 0, sadTabTitle.length(), textBounds);
                int titleHeight = textBounds.height();
                canvas.save();
                canvas.translate((int)(canvas.getWidth() * 0.1f),
                        dest.top + sadTabImg.getHeight() + textBounds.height() * 2);
                textLayout.draw(canvas);
                canvas.restore();
                sadTabImg.recycle();
            }
        } finally {
            TraceEvent.end();
        }
    }

    @Override
    protected void onAttachedToWindow() {
        mAttachedToWindow = true;
        super.onAttachedToWindow();
        if (mNativeChromeView != 0) {
            int pid = nativeGetCurrentRenderProcess(mNativeChromeView);
            if (pid > 0) {
                SandboxedProcessLauncher.bindAsHighPriority(pid);
            }
        }
        setAccessibilityState(true);
    }

    @Override
    protected void onDetachedFromWindow() {
        mAttachedToWindow = false;
        super.onDetachedFromWindow();
        mGestureDetectorProxy.cancelLongPress();
        if (mNativeChromeView != 0) {
            int pid = nativeGetCurrentRenderProcess(mNativeChromeView);
            if (pid > 0) {
                SandboxedProcessLauncher.unbindAsHighPriority(pid);
            }
        }

        setAccessibilityState(false);

        // We expect onDetachedFromWindow() to traverse children first, in which case
        // the TextureView (if any) should already have given up its SurfaceTexture.
        if (mWaitForNativeSurfaceDestroy || mSurfaceTextureWiredToProducer != null) {
            // If mWaitForNativeSurfaceDestroy is true, we will temporarily remove the TextureView
            // to avoid creating a new SurfaceTexture, while we have not freed the the last one.
            // (It gets readded once the GL thread has detached the surface.)
            // But if the SurfaceTexture is not owned by the TextureView, make sure things
            // get cleaned up regardless.
            removeTextureViewAndDetachProducer();
        }
    }

    private void prepareTextureViewResize(int oldWidth, int oldHeight) {
        assert(mTextureView != null);
        assert(mTextureViewStatus == TextureViewStatus.READY);
        if (mPlaceholderView.isActive()) return;
        TraceEvent.begin();
        // Grab a placeholder bitmap to display while the renderer is being resized.
        Bitmap bitmap = null;
        if (oldWidth > 0 && oldHeight > 0) {
            try {
                // Don't draw the overlay views (false parameter below). Infobars would be shown
                // twice (the real one and the one on the bitmap).
                bitmap = getScaledPerformanceOptimizedBitmap(oldWidth, oldHeight, false).first;
            } catch (OutOfMemoryError ex) {
                Log.w(TAG, "OutOfMemoryError thrown when trying to grab bitmap for resize.");
                // Fail gracefully if we're out of memory.  We don't actually need this bitmap
                // to resize.
                bitmap = null;
            }
        }
        // FIXME: During an orientation change getBitmap() fails silently and gives back an
        // empty bitmap. Detect this by checking for a transparent black pixel in the lower left
        // corner (OpenGL origin). Valid web content should never have a transparent root layer.
        if (bitmap != null && bitmap.getPixel(0, bitmap.getHeight() - 1) != 0) {
            mPlaceholderView.setBitmap(bitmap, oldWidth, oldHeight);
        } else {
            mPlaceholderView.resetBitmap();
        }
        mPlaceholderView.show();
        TraceEvent.end();
    }

    private void beginTextureViewResize(int width, int height) {
        assert(mTextureView != null);
        TraceEvent.begin("TextureViewResize");
        // Start waiting for a resize notification from the renderer.
        if (mTextureViewStatus == TextureViewStatus.READY ||
                mTextureViewStatus == TextureViewStatus.ATTACHEDINNATIVE) {
            // If the TextureView changes size independently of the ChromeView, we do not get an
            // advance warning and must show the placeholder view without a screenshot.
            if (!mPlaceholderView.isActive()) {
                mPlaceholderView.show();
            }
            mTextureViewStatus = TextureViewStatus.RESIZING;
        }
        // Send the size change notification to the renderer. We will get a callback in
        // onCompositorResized() once it has been processed.
        if (mNativeChromeView != 0) {
            mCompositorResizeTimestamp = TIMESTAMP_PENDING;
            nativeSetSize(mNativeChromeView, width, height);
            updateAfterSizeChanged();
        }
    }

    @CalledByNative
    private void onNativeWindowAttached() {
        if (mTextureView == null) return;
        assert(mTextureViewStatus == TextureViewStatus.INITIALIZING);
        mTextureViewStatus = TextureViewStatus.ATTACHEDINNATIVE;
    }

    @CalledByNative
    private void onNativeWindowDetached() {
        if (mTextureView == null) return;
        assert(mTextureViewStatus == TextureViewStatus.INITIALIZING);
    }

    @CalledByNative
    private void onCompositorResized(long timestamp, int width, int height) {
        TraceEvent.instant("CompositorResized");
        mCompositorWidth = width;
        mCompositorHeight = height;
        if (mTextureView == null || mTextureViewStatus == TextureViewStatus.READY) {
            return;
        }
        // This function may be called during initialization and we do actually want to call
        // updateTextureViewStatus only after a proper resize.
        if (mTextureViewStatus == TextureViewStatus.RESIZING) {
            mCompositorResizeTimestamp = timestamp;
            updateTextureViewStatus();
        }
    }

    /**
     * Update the TextureView state following a new SurfaceTexture frame or a resize acknowledgement
     * from the renderer. If the TextureView contents are determined to be valid (i.e., first frame
     * received in INITIALIZING state or a resize operation is complete), we hide the
     * PlaceholderView to allow the new contents to be shown.
     */
    private void updateTextureViewStatus() {
        assert(mTextureView != null);
        switch (mTextureViewStatus) {
            case INITIALIZING:
                break;
            case READY:
                assert(!mPlaceholderView.isActive());
                break;
            case RESIZING:
                // Check whether the TextureView contents are valid after a resize request.
                //
                // We know the resize has completed when
                //   1) onCompositorResized() has been called with timestamp t1; and
                //   2) onSurfaceTextureUpdated() has been called with timestamp t2; and
                //   3) t2 >= t1 (i.e., new frame was in response to the resize request).
                //
                // Since the renderer message loop and the SurfaceTexture use independent IPC
                // channels, events 1 and 2 might happen in either order. For this reason
                // updateTextureViewStatus() is called after both events.
                assert(mPlaceholderView.isActive());
                if (mCompositorWidth == getWidth() && mCompositorHeight == getHeight() &&
                        mCompositorFrameTimestamp >= mCompositorResizeTimestamp) {
                    endTextureViewResize();
                }
                break;
            case ATTACHEDINNATIVE:
                if (mPlaceholderView.isActive()) {
                    mPlaceholderView.hideLater();
                }
                mTextureViewStatus = TextureViewStatus.READY;
                if (mExternalSurfaceTextureOwner != null) {
                    mExternalSurfaceTextureOwner.onSurfaceTextureIsReady();
                }
                break;
            default:
                Log.e(TAG, "TextureViewStatus " + mTextureViewStatus + " not handled");
                assert(false);
                break;
        }
    }

    private void endTextureViewResize() {
        assert(mTextureView != null);
        assert(mTextureViewStatus == TextureViewStatus.RESIZING);
        TraceEvent.end("TextureViewResize");
        if (mPlaceholderView.isActive()) {
            // If we have a snapshot that covers the visible view, there is no need to fade it out
            // since the entire screen is already valid.
            if (mPlaceholderView.contentCoversView()) {
                mPlaceholderView.hideImmediately();
            } else {
                mPlaceholderView.hideWithAnimation();
            }
        }
        mTextureViewStatus = TextureViewStatus.READY;
    }

    @Override
    protected void onSizeChanged(int w, int h, int ow, int oh) {
        TraceEvent.begin();
        super.onSizeChanged(w, h, ow, oh);
        mPopupZoomer.hide(false);
        // If the TextureView is in use, the renderer size change will be triggered in
        // onSurfaceTextureSizeChanged instead. This is to ensure the SurfaceTexture is resized
        // before the renderer is notified.

        boolean externalSurfaceTextureOwner = mExternalSurfaceTextureOwner != null &&
                mSurfaceTextureWiredToProducer != null;

        if (!externalSurfaceTextureOwner && mTextureView != null &&
                mTextureViewStatus == TextureViewStatus.READY &&
                (mTextureView.getMeasuredWidth() != mTextureView.getWidth() ||
                mTextureView.getMeasuredHeight() != mTextureView.getHeight())) {
            prepareTextureViewResize(ow, oh);
        } else if (mNativeChromeView != 0) {
            if (externalSurfaceTextureOwner) {
                SurfaceTextureSwapConsumer.setDefaultBufferSize(
                        mSurfaceTextureWiredToProducer, w, h);
            }
            nativeSetSize(mNativeChromeView, w, h);
            updateAfterSizeChanged();
        }
        // Update the content size to make sure it is at least the View size
        if (mContentWidth < w) mContentWidth = w;
        if (mContentHeight < h) mContentHeight = h;
        TraceEvent.end();
    }

    private void updateAfterSizeChanged() {
        // Execute a delayed form focus operation because the OSK was brought
        // up earlier.
        if (mFocusOnNextSizeChanged) {
            scrollFocusedEditableNodeIntoView();
            mFocusOnNextSizeChanged = false;
        } else if (mUnfocusOnNextSizeChanged) {
            undoScrollFocusedEditableNodeIntoViewIfNeeded(true);
            mUnfocusOnNextSizeChanged = false;
        }

        if (mNeedUpdateOrientationChanged) {
            sendOrientationChange();
            mNeedUpdateOrientationChanged = false;
        }
    }

    private void scrollFocusedEditableNodeIntoView() {
        if (mNativeChromeView != 0) {
            Runnable scrollTask = new Runnable() {
                @Override
                public void run() {
                    if (mNativeChromeView != 0) {
                        nativeScrollFocusedEditableNodeIntoView(mNativeChromeView);
                    }
                }
            };
            if (mPlaceholderView != null && mPlaceholderView.isActive()) {
                mPlaceholderView.setPostHideRunnable(scrollTask);
            } else {
                scrollTask.run();
            }

            // The native side keeps track of whether the zoom and scroll actually occurred. It is
            // more efficient to do it this way and sometimes fire an unnecessary message rather
            // than synchronize with the renderer and always have an additional message.
            mScrolledAndZoomedFocusedEditableNode = true;
        }
    }

    private void undoScrollFocusedEditableNodeIntoViewIfNeeded(boolean backButtonPressed) {
        // The only call to this function that matters is the first call after the
        // scrollFocusedEditableNodeIntoView function call.
        // If the first call to this function is a result of a back button press we want to undo the
        // preceding scroll. If the call is a result of some other action we don't want to perform
        // an undo.
        // All subsequent calls are ignored since only the scroll function sets
        // mScrolledAndZoomedFocusedEditableNode to true.
        if (mScrolledAndZoomedFocusedEditableNode && backButtonPressed && mNativeChromeView != 0) {
            Runnable scrollTask = new Runnable() {
                @Override
                public void run() {
                    if (mNativeChromeView != 0) {
                        nativeUndoScrollFocusedEditableNodeIntoView(mNativeChromeView);
                    }
                }
            };
            if (mPlaceholderView != null && mPlaceholderView.isActive()) {
                mPlaceholderView.setPostHideRunnable(scrollTask);
            } else {
                scrollTask.run();
            }
        }
        mScrolledAndZoomedFocusedEditableNode = false;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (!mImeAdapter.hasTextInputType()) {
            // Although onCheckIsTextEditor will return false in this case, the EditorInfo
            // is still used by the InputMethodService. Need to make sure the IME doesn't
            // enter fullscreen mode.
            outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN;
        }
        return ImeAdapter.AdapterInputConnection.getInstance(this, mImeAdapter,
                outAttrs);
    }

    @Override
    public boolean onCheckIsTextEditor() {
        return mImeAdapter.hasTextInputType();
    }

    @Override
    protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        TraceEvent.begin();
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        if (mNativeChromeView != 0) {
            nativeSetFocus(mNativeChromeView, gainFocus);
        }
        TraceEvent.end();
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mPopupZoomer.isShowing() && keyCode == KeyEvent.KEYCODE_BACK) {
            mPopupZoomer.hide(true);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean dispatchKeyEventPreIme(KeyEvent event) {
        try {
            TraceEvent.begin();
            if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && mImeAdapter.isActive()) {
                mUnfocusOnNextSizeChanged = true;
            } else {
                undoScrollFocusedEditableNodeIntoViewIfNeeded(false);
            }
            mImeAdapter.dispatchKeyEventPreIme(event);
            return super.dispatchKeyEventPreIme(event);
        } finally {
            TraceEvent.end();
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (mImeAdapter != null &&
                !mImeAdapter.isNativeImeAdapterAttached() && mNativeChromeView != 0) {
            mImeAdapter.attach(nativeGetNativeImeAdapter(mNativeChromeView));
        }
        // The key handling logic is kind of confusing here.
        // The purpose of shouldOverrideKeyEvent() is to filter out some keys that is critical
        // to browser function but useless in renderer process (for example, the back button),
        // so the browser can still respond to these keys in a timely manner when the renderer
        // process is busy/blocked/busted. mImeAdapter.dispatchKeyEvent() forwards the key event
        // to the renderer process. If mImeAdapter is bypassed or is not interested to the event,
        // fall back to the default dispatcher to propagate the event to sub-views.
        return (!getChromeViewClient().shouldOverrideKeyEvent(event)
                && mImeAdapter .dispatchKeyEvent(event))
                || super.dispatchKeyEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        TraceEvent.begin("onTouchEvent");
        mGestureDetectorProxy.cancelLongPressIfNeeded(event);

        // Notify native that scrolling has stopped whenever a down action is processed prior to
        // passing the event to native as it will drop them as an optimization if scrolling is
        // enabled.  Ending the fling ensures scrolling has stopped as well as terminating the
        // current fling if applicable.
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            endFling();
        }

        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            // A new gesture has started, clear all the flags for draining the queue or
            // disabling the timeout. If the pending queue has entries, we cannot clear
            // mDisableWebKitTimeout as the ACTION_UP event could be in the queue.
            // ConsumeTouchEvents() will help us clear mDisableWebKitTimeout when it encounters
            // that ACTION_UP.
            if (mPendingMotionEvents.isEmpty() && mDisableWebKitTimeout)
                mDisableWebKitTimeout = false;
            if (mSkipSendToNative)
                mSkipSendToNative = false;
        }

        undoScrollFocusedEditableNodeIntoViewIfNeeded(false);

        if (offerTouchEventToNative(event)) {
            // offerTouchEventToNative returns true to indicate the event was sent
            // to the render process. If it is not subsequently handled, it will
            // be returned via confirmTouchEvent(false) and eventually passed to
            // processTouchEvent asynchronously.
            TraceEvent.end("onTouchEvent");
            return true;
        }
        return processTouchEvent(event);
    }

    // Mouse move events are sent on hover enter, hover move and hover exit.
    // They are sent on hover exit because sometimes it acts as both a hover
    // move and hover exit.
    @Override
    public boolean onHoverEvent(MotionEvent event) {
        // Ignore hover events caused by fling.  This happens on some device
        // like Nakasi (http://b/6508589, http://b/6522527)
        // AccessibilityEvents also trigger hover events, however, and are necessary to
        // make "touch exploration" work (http://b/6593335)
        boolean accessibilityOn =
                mAccessibilityManager != null && mAccessibilityManager.isEnabled();
        if (!accessibilityOn && event.getDownTime() == mLastDownTime) {
            Log.d(TAG, "Ignoring hover event caused by fling.");
            return false;
        }

        TraceEvent.begin("onHoverEvent");
        removeCallbacks(mFakeMouseMoveRunnable);
        if (mNativeChromeView != 0) {
            nativeMouseMoveEvent(mNativeChromeView, event.getEventTime(),
                    (int) event.getX(), (int) event.getY());
        }
        TraceEvent.end("onHoverEvent");
        return true;
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_CLASS_POINTER) != 0) {
            switch (event.getAction()) {
                case MotionEvent.ACTION_SCROLL:
                    if (event.getAxisValue(MotionEvent.AXIS_VSCROLL) > 0 ) {
                        nativeSendMouseWheelEvent(mNativeChromeView, (int) event.getX(),
                               (int) event.getY(), event.getEventTime(), SCROLL_DIRECTION_UP);
                    } else {
                        nativeSendMouseWheelEvent(mNativeChromeView, (int) event.getX(),
                                (int) event.getY(), event.getEventTime(), SCROLL_DIRECTION_DOWN);
                    }

                    removeCallbacks(mFakeMouseMoveRunnable);
                    // Send a delayed onMouseMove event so that we end
                    // up hovering over the right position after the scroll.
                    final MotionEvent eventFakeMouseMove = MotionEvent.obtain(event);
                    mFakeMouseMoveRunnable = new Runnable() {
                          @Override
                          public void run() {
                              onHoverEvent(eventFakeMouseMove);
                          } };
                    postDelayed(mFakeMouseMoveRunnable, 250);
                    return true;
            }
        }
        return super.onGenericMotionEvent(event);
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        TraceEvent.begin();

        mKeyboardConnected = newConfig.keyboard != Configuration.KEYBOARD_NOKEYS;

        if (mKeyboardConnected) {
            mImeAdapter.attach(nativeGetNativeImeAdapter(mNativeChromeView),
                    ImeAdapter.sTextInputTypeNone);
            InputMethodManager manager = (InputMethodManager)
                    getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            manager.restartInput(this);
        }
        super.onConfigurationChanged(newConfig);
        closeAutofillPopup();
        mNeedUpdateOrientationChanged = true;
        TraceEvent.end();
    }

    @Override
    protected ContextMenu.ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    // Note: Currently the ChromeView scrolling happens in the native side. In
    // the Java view system, it is always pinned at (0, 0). Override scrollBy()
    // and scrollTo(), so that View's mScrollX and mScrollY will be unchanged at
    // (0, 0). This is critical for drawing ChromeView correctly.
    @Override
    public void scrollBy(int x, int y) {
        if (mNativeChromeView != 0) {
            nativeScrollBy(mNativeChromeView, x, y);
        }
    }

    @Override
    public void scrollTo(int x, int y) {
        if (mNativeChromeView != 0) {
            int dx = x - mNativeScrollX, dy = y - mNativeScrollY;
            if (dx != 0 || dy != 0) {
                nativeScrollBy(mNativeChromeView, dx, dy);
            }
        }
    }

    // TODO (dtrainor): Need to expose scroll events properly to public.  Either make getScrollX/Y
    // work or expose computeHorizontalScrollOffset()/computeVerticalScrollOffset as public.

    @Override
    protected int computeHorizontalScrollExtent() {
        return getWidth();
    }

    @Override
    protected int computeHorizontalScrollOffset() {
        return mNativeScrollX;
    }

    @Override
    protected int computeHorizontalScrollRange() {
        return mContentWidth;
    }

    @Override
    protected int computeVerticalScrollExtent() {
        return getHeight();
    }

    @Override
    protected int computeVerticalScrollOffset() {
        return mNativeScrollY;
    }

    @Override
    protected int computeVerticalScrollRange() {
        return mContentHeight;
    }

    // End FrameLayout overrides.

    @Override
    protected boolean awakenScrollBars(int startDelay, boolean invalidate) {
        // For the default implementation of ChromeView which draws the scrollBars on the native
        // side, calling this function may get us into a bad state where we keep drawing the
        // scrollBars, so disable it by always returning false.
        if (getScrollBarStyle() == View.SCROLLBARS_INSIDE_OVERLAY) {
            return false;
        } else {
            return super.awakenScrollBars(startDelay, invalidate);
        }
    }

    public static class ChromeViewContextMenuInfo implements ContextMenu.ContextMenuInfo {

        /**
         * The x coordinate in the browser/window space of the event that generated the context menu.
         */
        public final int x;

        /**
         * The y coordinate in the browser/window space of the event that generated the context menu.
         */
        public final int y;

        /**
         * The coordinates in the document space of the start and end of the selection,
         * if there is one.
         */
        public final int selectionStartX;
        public final int selectionStartY;
        public final int selectionEndX;
        public final int selectionEndY;

        public final String linkUrl;
        public final String linkText;
        public final String unfilteredLinkUrl;
        public final String srcUrl;
        public final String selectionText;
        public final boolean isEditable;

        //TODO(olilan): Add identification of phone/email/geo types
        public final boolean isAnchor;
        public final boolean isImage;
        public final boolean isSelectedText;
        public final boolean isTextLink;
        public final boolean isVideo;
        public final boolean isAudio;
        public final boolean isPlugin;

        // These must correspond to the MediaType enum in
        // WebKit/chromium/public/WebContextMenuData.h
        private final int mediaType;
        private static final int MEDIA_TYPE_NONE = 0;
        private static final int MEDIA_TYPE_IMAGE = 1;
        private static final int MEDIA_TYPE_VIDEO = 2;
        private static final int MEDIA_TYPE_AUDIO = 3;
        private static final int MEDIA_TYPE_PLUGIN = 4;

        public static class CustomMenuItem {
            public final String mLabel;
            public final int mAction;

            public CustomMenuItem(String label, int action) {
                mLabel = label;
                mAction = action;
            }
        }

        private ArrayList<CustomMenuItem> mCustomItems;

        private ChromeViewContextMenuInfo(int x, int y, int mediaType,
                int selectionStartX, int selectionStartY, int selectionEndX, int selectionEndY,
                String linkUrl, String linkText, String unfilteredLinkUrl, String srcUrl,
                String selectionText, boolean isEditable) {
            this.x = x;
            this.y = y;
            this.mediaType = mediaType;
            this.selectionStartX = selectionStartX;
            this.selectionStartY = selectionStartY;
            this.selectionEndX = selectionEndX;
            this.selectionEndY = selectionEndY;
            this.linkUrl = linkUrl;
            this.linkText = linkText;
            this.unfilteredLinkUrl = unfilteredLinkUrl;
            this.srcUrl = srcUrl;
            this.selectionText = selectionText;
            this.isEditable = isEditable;

            isAnchor = linkUrl != null && !linkUrl.isEmpty();
            isSelectedText = selectionText != null && !selectionText.isEmpty();
            isImage = mediaType == MEDIA_TYPE_IMAGE;
            isTextLink = isAnchor && mediaType == MEDIA_TYPE_NONE;
            isVideo = mediaType == MEDIA_TYPE_VIDEO;
            isAudio = mediaType == MEDIA_TYPE_AUDIO;
            isPlugin = mediaType == MEDIA_TYPE_PLUGIN;
        }

        private ChromeViewContextMenuInfo(int x, int y) {
            this(x, y, MEDIA_TYPE_NONE, 0, 0, 0, 0, null, null, null, null, null, false);
        }

        private void addCustomItem(String label, int action) {
            if (mCustomItems == null) mCustomItems = new ArrayList<CustomMenuItem>();
            mCustomItems.add(new CustomMenuItem(label, action));
        }

        public boolean isCustomMenu() {
            return mCustomItems != null;
        }

        public int getCustomItemSize() {
            assert mCustomItems != null;
            return mCustomItems.size();
        }

        public CustomMenuItem getCustomItemAt(int i) {
            assert mCustomItems != null;
            return mCustomItems.get(i);
        }
    }

    private class WebKitTimeoutRunnable implements Runnable {
        @Override
        public void run() {
            // WebKit is busy handling sth and the touch event we sent don't result in any acks.
            // drain all the touch events for the current gesture, and increase the number of
            // acks to ignore.
            consumePendingTouchEvents(false, true);
            mPendingTouchAcksToIgnore++;
        }
    }

    private boolean offerTouchEventToNative(MotionEvent event) {
        mGestureDetectorProxy.onOfferTouchEventToNative(event);

        if (!mNeedTouchEvents || mSkipSendToNative) {
            return false;
        }

        if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
            // Only send move events if the move has exceeded the slop threshold.
            if (!mGestureDetectorProxy.confirmOfferMoveEventToNative(event)) {
                return true;
            }
            // Avoid flooding the renderer process with move events: if the previous pending
            // command is also a move (common case), skip sending this event to the webkit
            // side and collapse it into the pending event.
            PendingMotionEvent previousEvent = mPendingMotionEvents.peekLast();
            if (previousEvent != null && previousEvent.offeredToNative() == EVENT_FORWARDED_TO_NATIVE
                    && previousEvent.event().getActionMasked() == MotionEvent.ACTION_MOVE
                    && previousEvent.event().getPointerCount() == event.getPointerCount()) {
                MotionEvent.PointerCoords[] coords =
                    new MotionEvent.PointerCoords[event.getPointerCount()];
                for (int i = 0; i < coords.length; ++i) {
                    coords[i] = new MotionEvent.PointerCoords();
                    event.getPointerCoords(i, coords[i]);
                }
                previousEvent.event().addBatch(event.getEventTime(), coords, event.getMetaState());
                return true;
            }
        }

        TouchPoint[] pts = new TouchPoint[event.getPointerCount()];
        int type = TouchPoint.createTouchPoints(event, pts);

        if (type != TouchPoint.CONVERSION_ERROR &&
            mNativeChromeView != 0) {
            int forwarded = nativeTouchEvent(mNativeChromeView, type, event.getEventTime(), pts);
            if (forwarded != EVENT_NOT_FORWARDED_TO_NATIVE || !mPendingMotionEvents.isEmpty()) {
                // If the event is converted to a touch cancel, make it timeout immediately so that
                // we will handle it as soon as we encounter it in the pending queue.
                long timeoutTime = SystemClock.uptimeMillis();
                if (forwarded == EVENT_FORWARDED_TO_NATIVE)
                    timeoutTime = timeoutTime + WEBKIT_TIMEOUT_MILLIS;
                // Copy the event, as the original may get mutated after this method returns.
                PendingMotionEvent pendingEvent = new PendingMotionEvent(MotionEvent.obtain(event),
                        timeoutTime, forwarded);
                // If mDisableWebKitTimeout is true, the webkit is interested in processing the
                // current gesture. As a result, don't fire the timer. The timer will be fired after
                // we encounter an ACTION_UP.
                if (mPendingMotionEvents.isEmpty() && !mDisableWebKitTimeout) {
                    mWebKitTimeoutTime = pendingEvent.timeoutTime();
                    mHandler.postAtTime(mWebKitTimeoutRunnable, mWebKitTimeoutTime);
                }
                mPendingMotionEvents.add(pendingEvent);
                return true;
            }
        }
        return false;
    }


    private void consumePendingTouchEvents(boolean handled, boolean timeout) {
        if (mPendingMotionEvents.isEmpty()) {
            Log.w(TAG, "consumePendingTouchEvent with Empty pending list!");
            return;
        }
        TraceEvent.begin();
        PendingMotionEvent event = mPendingMotionEvents.removeFirst();
        if (!handled || event.offeredToNative() == EVENT_CONVERTED_TO_CANCEL) {
            if (!processTouchEvent(event.event())) {
                // TODO(joth): If the Java side gesture handler also fails to consume
                // this deferred event, should it be bubbled up to the parent view?
                Log.w(TAG, "Unhandled deferred touch event");
            }
        } else {
            mDisableWebKitTimeout = true;
            // Need to pass the touch event to ScaleGestureDetector so that its internal
            // state won't go wrong. But instruct the listener to do nothing as the
            // renderer already handles the touch event.
            mIgnoreScaleGestureDetectorEvents = true;
            try {
                mScaleGestureDetector.onTouchEvent(event.event());
            } catch (Exception e) {
                Log.e(TAG, "ScaleGestureDetector got into a bad state!");
                assert(false);
            }
        }

        // Clean up the timer.
        if (mWebKitTimeoutTime == event.timeoutTime())
            mHandler.removeCallbacks(mWebKitTimeoutRunnable);

        boolean withinSameTouchSequence = (event.event().getAction() != MotionEvent.ACTION_UP);

        // If timeout is true, we need to drain the pending queue until we reach an
        // ACTION_UP event.
        boolean continueToDrain = timeout && withinSameTouchSequence;

        // Record the current mDisableWebKitTimeout value to check whether WebKit is handling the current
        // touch sequence.
        boolean touchHandledByWebKit = mDisableWebKitTimeout;

        // We need to disable the timeout until we reach an ACTION_UP event.
        mDisableWebKitTimeout = mDisableWebKitTimeout && withinSameTouchSequence;

        // We may have pending events that could cancel the timers:
        // For instance, if we received an UP before the DOWN completed
        // its roundtrip (so it didn't cancel the timer during onTouchEvent()).
        mGestureDetectorProxy.cancelLongPressIfNeeded(mPendingMotionEvents.iterator());

        // Now process all events that are either:
        // 1. in the queue but not sent to the native.
        // 2. or we are in a draining mode and we still haven't encountered the ACTION_UP event.
        PendingMotionEvent nextEvent = mPendingMotionEvents.peekFirst();
        while (nextEvent != null && (nextEvent.offeredToNative() == EVENT_NOT_FORWARDED_TO_NATIVE
                || continueToDrain)) {
            withinSameTouchSequence = (nextEvent.event().getAction() != MotionEvent.ACTION_UP);
            continueToDrain = continueToDrain && withinSameTouchSequence;
            mDisableWebKitTimeout = mDisableWebKitTimeout && withinSameTouchSequence;
            processTouchEvent(nextEvent.event());
            mPendingMotionEvents.removeFirst();
            nextEvent.event().recycle();
            // In draining mode, if a event is sent to the native, we need to ignore its ack.
            if (nextEvent.offeredToNative() != EVENT_NOT_FORWARDED_TO_NATIVE)
                mPendingTouchAcksToIgnore++;
            nextEvent = mPendingMotionEvents.peekFirst();
        }

        if (nextEvent != null && !mDisableWebKitTimeout) {
            mWebKitTimeoutTime = nextEvent.timeoutTime();
            // Fire another timer for the next task.
            if (touchHandledByWebKit && nextEvent.event().getAction() == MotionEvent.ACTION_DOWN
                    && nextEvent.offeredToNative() == EVENT_FORWARDED_TO_NATIVE) {
                // A new touch sequence could be queued up while WebKit is processing the
                // previous one. In this case, the new sequence could have already missed
                // its timeout. To handle this, we give the ACTION_DOWN another 200 ms to
                // wait for the ack. If WebKit handles the touch down, we will proceed to
                // handle the whole sequence. Otherwise, the rest of the sequence will
                // timeout as usual.
                mHandler.postAtTime(mWebKitTimeoutRunnable,
                        SystemClock.uptimeMillis() + WEBKIT_TIMEOUT_MILLIS);
            } else {
                mHandler.postAtTime(mWebKitTimeoutRunnable, mWebKitTimeoutTime);
            }
        }

        if (nextEvent == null && continueToDrain) {
            // We could reach here while we are still in the middle of a gesture event.
            // Stop sending all the touch events to native until next ACTION_UP.
            // And send a touch cancel to the WebKit to cancel the current touch event.
            assert(!mSkipSendToNative);
            mSkipSendToNative = true;
            if (mNativeChromeView != 0) {
                MotionEvent ev = event.event();
                ev.setAction(MotionEvent.ACTION_CANCEL);
                TouchPoint[] pts = new TouchPoint[ev.getPointerCount()];
                int type = TouchPoint.createTouchPoints(ev, pts);
                int forwarded = nativeTouchEvent(mNativeChromeView, type, ev.getEventTime(), pts);
                // If the event is forwarded to WebKit, just ignore the ack.
                if (forwarded != EVENT_NOT_FORWARDED_TO_NATIVE)
                    mPendingTouchAcksToIgnore++;
            }
        }
        event.event().recycle();
        TraceEvent.end();
    }

    private boolean processTouchEvent(MotionEvent event) {
        boolean handled = false;
        // The last "finger up" is an end to scrolling but may not be
        // an end to movement (e.g. fling scroll).  We do not tell
        // native code to end scrolling until we are sure we did not
        // fling.
        boolean possiblyEndMovement = false;

        // "Last finger raised" could be an end to movement.  However,
        // give the mSimpleTouchDetector a chance to continue
        // scrolling with a fling.
        if ((event.getAction() == MotionEvent.ACTION_UP)) {
            if (mNativeScrolling) {
                possiblyEndMovement = true;
            }
            mHideHandleTemporarilyForScroll = false;
            fadeinHandleIfNeeded();
        }

        // Use the framework's GestureDetector to detect pans and zooms not already
        // handled by the WebKit touch events gesture manager.
        if (mGestureDetectorProxy.canHandle(event)) {
            handled |= mGestureDetectorProxy.onTouchEvent(event);
        }

        mIgnoreScaleGestureDetectorEvents = false;
        try {
            handled |= mScaleGestureDetector.onTouchEvent(event);
        } catch (Exception e) {
            Log.e(TAG, "ScaleGestureDetector got into a bad state!");
            assert(false);
        }

        if (possiblyEndMovement && !handled) {
            tellNativeScrollingHasEnded();
        }

        TraceEvent.end("onTouchEvent");
        return handled;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onTabCrash(int pid) {
        getChromeViewClient().onTabCrash(pid);
        // Remove the attached TextureView as the tab is crashed.
        if (!mReloadOnCrash && mTextureView != null) {
            removeTextureViewAndDetachProducer();
            mTextureView = null;
        }
        if (!mReloadOnCrash && mPlaceholderView != null) {
            removeView(mPlaceholderView);
            mPlaceholderView = null;
        }
        // Update the view to the "Ah, snap!" screen.
        invalidate();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onPopupBlockStateChanged() {
        getChromeViewClient().onPopupBlockStateChanged();
    }

    /**
     * Returns true if the given Activity has hardware acceleration enabled
     * in its manifest, or in its foreground window.
     *
     * @hide Used by platform browser and internally only.
     *
     * TODO(husky): Remove when initialize() is refactored (see TODO there)
     * TODO(dtrainor) This is still used by other classes.  Make sure to pull some version of this
     * out before removing it.
     */
    public static boolean hasHardwareAcceleration(Activity activity) {
        // Has HW acceleration been enabled manually in the current window?
        Window window = activity.getWindow();
        if (window != null) {
            if ((window.getAttributes().flags
                    & WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED) != 0) {
                return true;
            }
        }

        // Has HW acceleration been enabled in the manifest?
        try {
            ActivityInfo info = activity.getPackageManager().getActivityInfo(
                    activity.getComponentName(), 0);
            if ((info.flags & ActivityInfo.FLAG_HARDWARE_ACCELERATED) != 0) {
                return true;
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.e("Chrome", "getActivityInfo(self) should not fail");
        }

        return false;
    }

    /**
     * Returns true if the given Context is a HW-accelerated Activity.
     *
     * TODO(husky): Remove when initialize() is refactored (see TODO there)
     */
    private static boolean hasHardwareAcceleration(Context context) {
        if (context instanceof Activity) {
            return hasHardwareAcceleration((Activity) context);
        }
        return false;
    }

    private void initialize(Context context, boolean incognito, int nativeWebContents,
            Personality personality) {
        TraceEvent.begin();
        HeapStatsLogger.init(context.getApplicationContext());
        // Check whether to use hardware acceleration. This is a bit hacky, and
        // only works if the Context is actually an Activity (as it is in the
        // Chrome application).
        //
        // What we're doing here is checking whether the app has *requested*
        // hardware acceleration by setting the appropriate flags. This does not
        // necessarily mean we're going to *get* hardware acceleration -- that's
        // up to the Android framework.
        //
        // TODO(husky): Once the native code has been updated so that the
        // HW acceleration flag can be set dynamically (Grace is doing this),
        // move this check into onAttachedToWindow(), where we can test for
        // HW support directly.
        boolean hardwareAccelerated = hasHardwareAcceleration(context);

        mNativeChromeView = nativeInit(context.getApplicationContext(), incognito,
                hardwareAccelerated, nativeWebContents);

        mIncognito = incognito;
        setWillNotDraw(false);
        setFocusable(true);
        setFocusableInTouchMode(true);
        if (getScrollBarStyle() == View.SCROLLBARS_INSIDE_OVERLAY) {
            setHorizontalScrollBarEnabled(false);
            setVerticalScrollBarEnabled(false);
        }
        setClickable(true);
        initGestureDetectors(context);
        mNativeScrollX = mNativeScrollY = 0;
        mNativePageScaleFactor = 1.0f;
        initPopupZoomer(context);
        DownloadController.getInstance().setContext(context);
        switch (personality) {
            case WEB_VIEW:
                setAllowContentAccess(true);
                setAllowFileAccess(true);
                break;
            case BROWSER:
                setAllowContentAccess(false);
                // Note that the WebKit security framework disallows access to local files from
                // remote or unique security origins regardless of this setting.
                setAllowFileAccess(true);
                break;
        }
        mWebViewLegacy = new WebViewLegacy(this);
        // As it checks the CommandLine, it needs to be called after nativeInit()
        mLogFPS = CommandLine.getInstance().hasSwitch(CommandLine.LOG_FPS);

        // We should set this constant based on the GPU performance. As it doesn't exist in the
        // framework yet, we use the memory class as an indicator. Here are some good values that
        // we determined via manual experimentation:
        //
        // Device            Screen size        Memory class   Tiles per 100ms
        // ================= ================== ============== =====================
        // Nexus S            480 x  800         128            9 (3 rows portrait)
        // Galaxy Nexus       720 x 1280         256           12 (3 rows portrait)
        // Nexus 7           1280 x  800         384           18 (3 rows landscape)
        // M tablet          2560 x 1600         512           44 (4 rows landscape)
        //
        // Here is a spreadsheet with the data, plus a curve fit: http://goto/max_num_upload_tiles
        // That gives us tiles-per-100ms of 8, 13, 22, 37 for the devices listed above.
        // Not too bad, and it should behave reasonably sensibly for unknown devices.
        // If you want to tweak these constants, please update the spreadsheet appropriately.
        //
        // The curve is y = b * m^x, with coefficients as follows:
        double b = 4.70009671080384;
        double m = 1.00404437546897;
        int memoryClass = ((ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE))
                .getLargeMemoryClass();
        MAX_NUM_UPLOAD_TILES = (int) Math.round(b * Math.pow(m, memoryClass));

        mKeyboardConnected = getResources().getConfiguration().keyboard
                != Configuration.KEYBOARD_NOKEYS;
        initVSync(context);
        TraceEvent.end();
    }

    private void initPopupZoomer(Context context){
        mPopupZoomer = new PopupZoomer(context);
        addView(mPopupZoomer);
        PopupZoomer.OnTapListener listener = new PopupZoomer.OnTapListener() {
            @Override
            public boolean onSingleTap(View v, MotionEvent e) {
                ChromeView.this.requestFocus();
                if (mNativeChromeView != 0) {
                    nativeSingleTap(mNativeChromeView, (int)e.getX(), (int)e.getY(), false);
                }
                return true;
            }

            @Override
            public boolean onLongPress(View v, MotionEvent e) {
                if (mNativeChromeView != 0) {
                    nativeLongPress(mNativeChromeView, (int)e.getX(), (int)e.getY(), false);
                }
                return true;
            }
        };
        mPopupZoomer.setOnTapListener(listener);
    }

    private void vsyncCallback(long vsyncTimeUSec, long deltaTimeUSec) {
        TraceEvent.instant("VSync");
        if (mNativeChromeView != 0) {
            nativeSendVSyncEvent(mNativeChromeView, vsyncTimeUSec, deltaTimeUSec);
        }
    }

    // FIXME: Currently m18 is based on ICS, so we have to use a reflection hack to implement
    // Choreographer.FrameCallback. It is ugly. We should change to use the class directly when
    // moving to JB.
    private class ProxyListener implements java.lang.reflect.InvocationHandler {
        public Object invoke(Object proxy, Method method, Object[] args)
                throws Throwable {
            long frameTimeNanos = (Long)args[0];
            postVSyncCallback();
            // Compensate for input event lag.
            frameTimeNanos += INPUT_EVENT_LAG_FROM_VSYNC_NSEC;
            vsyncCallback(frameTimeNanos / NANOSECONDS_PER_MICROSECOND, mRefreshRateInUSec);
            return null;
        }
    }

    // VSyncTimer is a simple timer based class simulating vsync when Choreographer doesn't work.
    // It only works well when it is started close to the beginning of the draw cycle.
    private class VSyncTimer implements Runnable {
        public void run() {
            long frameTimeUSec = mNextEstimatedVSyncUSec / MICROSECONDS_PER_MILLISECOND;
            if (mVSyncTimer == this) {
                postVSyncCallback();
            }
            vsyncCallback(frameTimeUSec, mRefreshRateInUSec);
        }
    }

    // VSyncMonitor keeps vsync running for a period of VSYNC_RUN_PERIOD. If there is no update
    // in the last quarter of the time, it will stop vsync.
    private class VSyncMonitor implements Runnable {
        public void run() {
            if (System.nanoTime() > mCompositorFrameTimestamp + VSYNC_RUN_PERIOD * 250000) {
                stopVSync();
            } else if (mVSyncMonitor == this) {
                postDelayed(this, VSYNC_RUN_PERIOD);
            }
        }
    }

    private void initVSync(Context context) {
        float refreshRate = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE))
                .getDefaultDisplay().getRefreshRate();
        if (refreshRate <= 0) refreshRate = 60;
        mRefreshRateInUSec = (long) (MICROSECONDS_PER_SECOND / refreshRate);

        // Use Choreographer on >= JB to get notified of vsync.
        if (Build.VERSION.SDK_INT >= 16) {
            try {
                Class<?> choreographerClass = Class.forName("android.view.Choreographer");
                Method getInstance = choreographerClass.getMethod("getInstance");
                Object choreographer = getInstance.invoke(null);
                Class<?> listener = Class.forName("android.view.Choreographer$FrameCallback");
                mFrameCallback = Proxy.newProxyInstance(
                        listener.getClassLoader(), new Class[]{listener}, new ProxyListener());
                mChoreographerPostFrameCallback = choreographerClass.getMethod("postFrameCallback",
                                                                               listener);
                mChoreographer = choreographer;
            } catch (ClassNotFoundException e) {
                Log.e(TAG, "Can't find class: " + e);
            } catch (IllegalAccessException e) {
                Log.e(TAG, "Can't create Choreographer or can't invoke method: " + e);
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "Can't invoke method: " + e);
            } catch (InvocationTargetException e) {
                Log.e(TAG, "Can't invoke method: " + e);
            } catch (NoSuchMethodException e) {
                Log.e(TAG, "Can't find method: " + e);
            }
        }
    }

    // updateVSync will start vsync if it hasn't started yet. To save the battery life, we will
    // stop the vsync VSYNC_RUN_PERIOD if it is inactive.
    private void updateVSync() {
        if (!mInVSync) {
            mInVSync = true;
            mVSyncMonitor = new VSyncMonitor();
            postDelayed(mVSyncMonitor, VSYNC_RUN_PERIOD);
            postVSyncCallback();
        }
    }

    private void postVSyncCallback() {
        if (!mInVSync) {
            return;
        }

        if (mChoreographer != null) {
            try {
                mChoreographerPostFrameCallback.invoke(mChoreographer, mFrameCallback);
                return;
            } catch (IllegalAccessException e) {
                Log.e(TAG, "Can't post frame callback: " + e);
            } catch (InvocationTargetException e) {
                Log.e(TAG, "Can't post frame callback: " + e);
            }
        }

        // Fall back to timer-based vsync.
        if (mVSyncTimer == null) {
            mVSyncTimer = new VSyncTimer();
        }

        // Timebase adjustment adapted from CCDelayBasedTimeSource.
        long now = System.nanoTime() / NANOSECONDS_PER_MICROSECOND;
        if (mNextEstimatedVSyncUSec == 0) {
            mNextEstimatedVSyncUSec = now;
        }
        long numIntervalsElapsed = (now - mNextEstimatedVSyncUSec) / mRefreshRateInUSec;
        long lastEffectiveTick =
                mNextEstimatedVSyncUSec + numIntervalsElapsed * mRefreshRateInUSec;
        mNextEstimatedVSyncUSec = lastEffectiveTick + mRefreshRateInUSec;
        long delay = (mNextEstimatedVSyncUSec - now) / MICROSECONDS_PER_MILLISECOND;
        assert(delay >= 0);
        postDelayed(mVSyncTimer, delay);
    }

    private void stopVSync() {
        mInVSync = false;
        mVSyncMonitor = null;
        mVSyncTimer = null;
        mNextEstimatedVSyncUSec = 0;
    }

    // If native thinks scrolling (or fling-scrolling) is going on, tell native
    // it has ended.
    private void tellNativeScrollingHasEnded() {
        if (mNativeScrolling) {
            mNativeScrolling = false;
            if (mNativeChromeView != 0) {
                nativeScrollEnd(mNativeChromeView);
            }
        }
    }

    public boolean isIncognito() {
        return mIncognito;
    }

    public int getSingleTapX()  {
        return mSingleTapX;
    }

    public int getSingleTapY()  {
        return mSingleTapY;
    }

    private void setClickXAndY(int x, int y) {
        mSingleTapX = x;
        mSingleTapY = y;
    }

    private void handleTapOrPress(float x, float y, boolean isLongPress) {
        if (!isFocused()) requestFocus();

        if (!mPopupZoomer.isShowing()) mPopupZoomer.setLastTouch(x, y);

        if (isLongPress) {
            getInsertionHandleController().allowAutomaticShowing();
            getSelectionHandleController().allowAutomaticShowing();
            if (mNativeChromeView != 0) {
                nativeLongPress(mNativeChromeView, (int) x, (int) y, true);
            }
        } else {
            if (!mShowPressIsCalled && mNativeChromeView != 0) {
                nativeShowPressState(mNativeChromeView, (int) x, (int) y);
            }
            if (mSelectionEditable) getInsertionHandleController().allowAutomaticShowing();
            if (mNativeChromeView != 0) {
                nativeSingleTap(mNativeChromeView, (int) x, (int) y, true);
            }
            setClickXAndY((int) x, (int) y);
        }
    }

    private void startNativeScrolling(int x, int y) {
        if (!mNativeScrolling) {
            mNativeScrolling = true;
            closeAutofillPopup();
            if (mNativeChromeView != 0) {
                nativeScrollBegin(mNativeChromeView, x, y);
            }
        }
    }

    private void initGestureDetectors(final Context context) {
        int scaledTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
        mScaledTouchSlopSquare = scaledTouchSlop * scaledTouchSlop;
        try {
            TraceEvent.begin();
            GestureDetector.SimpleOnGestureListener listener =
                new GestureDetector.SimpleOnGestureListener() {
                    @Override
                    public boolean onDown(MotionEvent e) {
                        mShowPressIsCalled = false;
                        mIgnoreSingleTap = false;
                        mSeenFirstScroll = false;
                        mSnapScrollMode = SNAP_NONE;
                        mLastRawX = e.getRawX();
                        mLastRawY = e.getRawY();
                        mAccumulatedScrollErrorX = 0;
                        mAccumulatedScrollErrorY = 0;
                        // Return true to indicate that we want to handle touch
                        mLastDownTime = e.getDownTime();
                        return true;
                    }

                    @Override
                    public boolean onScroll(MotionEvent e1, MotionEvent e2,
                            float distanceX, float distanceY) {
                        // Scroll snapping
                        if (!mSeenFirstScroll) {
                            mAverageAngle = calculateDragAngle(distanceX, distanceY);
                            // Initial scroll event
                            if (mScaleGestureDetector == null
                                    || !mScaleGestureDetector.isInProgress()) {
                                // if it starts nearly horizontal or vertical, enforce it
                                if (mAverageAngle < HSLOPE_TO_START_SNAP) {
                                    mSnapScrollMode = SNAP_HORIZ;
                                    mAverageAngle = ANGLE_HORIZ;
                                } else if (mAverageAngle > VSLOPE_TO_START_SNAP) {
                                    mSnapScrollMode = SNAP_VERT;
                                    mAverageAngle = ANGLE_VERT;
                                }
                            }
                            mSeenFirstScroll = true;
                            // Ignore the first scroll delta to avoid a visible jump.
                            return true;
                        } else {
                            mAverageAngle +=
                                (calculateDragAngle(distanceX, distanceY) - mAverageAngle)
                                / MMA_WEIGHT_N;
                            if (mSnapScrollMode != SNAP_NONE) {
                                if ((mSnapScrollMode == SNAP_VERT
                                        && mAverageAngle < VSLOPE_TO_BREAK_SNAP)
                                        || (mSnapScrollMode == SNAP_HORIZ
                                                && mAverageAngle > HSLOPE_TO_BREAK_SNAP)) {
                                    // radical change means getting out of snap mode
                                    mSnapScrollMode = SNAP_NONE;
                                }
                            } else {
                                if (mScaleGestureDetector == null
                                        || !mScaleGestureDetector.isInProgress()) {
                                    if (mAverageAngle < HSLOPE_TO_START_SNAP) {
                                        mSnapScrollMode = SNAP_HORIZ;
                                        mAverageAngle = (mAverageAngle + ANGLE_HORIZ) / 2;
                                    } else if (mAverageAngle > VSLOPE_TO_START_SNAP) {
                                        mSnapScrollMode = SNAP_VERT;
                                        mAverageAngle = (mAverageAngle + ANGLE_VERT) / 2;
                                    }
                                }
                            }
                        }

                        if (mSnapScrollMode != SNAP_NONE) {
                            if (mSnapScrollMode == SNAP_HORIZ) {
                                distanceY = 0;
                            } else {
                                distanceX = 0;
                            }
                        }

                        boolean didUIStealScroll = getChromeViewClient().shouldOverrideScroll(
                                e2.getRawX() - mLastRawX, e2.getRawY() - mLastRawY,
                                computeHorizontalScrollOffset(), computeVerticalScrollOffset());

                        mLastRawX = e2.getRawX();
                        mLastRawY = e2.getRawY();
                        if (didUIStealScroll) return true;
                        startNativeScrolling((int) e1.getX(), (int) e1.getY());

                        // distanceX and distanceY is the scrolling offset since last onScroll.
                        // Because we are passing integers to webkit, this could introduce
                        // rounding errors. The rounding errors will accumulate overtime.
                        // To solve this, we should adding back the rounding errors each time
                        // when we calculate the new offset.
                        int dx = (int) (distanceX + mAccumulatedScrollErrorX);
                        int dy = (int) (distanceY + mAccumulatedScrollErrorY);
                        mAccumulatedScrollErrorX = distanceX + mAccumulatedScrollErrorX - dx;
                        mAccumulatedScrollErrorY = distanceY + mAccumulatedScrollErrorY - dy;
                        if ((dx | dy) != 0) {
                            mHideHandleTemporarilyForScroll = true;
                            hideHandleTemporarily();
                            ChromeView.this.scrollBy(dx, dy);
                        }
                        return true;
                    }

                    @Override
                    public boolean onFling(MotionEvent e1, MotionEvent e2,
                            float velocityX, float velocityY) {
                        if (mSnapScrollMode == SNAP_NONE) {
                            float flingAngle = calculateDragAngle(velocityX, velocityY);
                            if (flingAngle < HSLOPE_TO_START_SNAP) {
                                mSnapScrollMode = SNAP_HORIZ;
                                mAverageAngle = ANGLE_HORIZ;
                            } else if (flingAngle > VSLOPE_TO_START_SNAP) {
                                mSnapScrollMode = SNAP_VERT;
                                mAverageAngle = ANGLE_VERT;
                            }
                        }

                        if (mSnapScrollMode != SNAP_NONE) {
                            if (mSnapScrollMode == SNAP_HORIZ) {
                                velocityY = 0;
                            } else {
                                velocityX = 0;
                            }
                        }

                        fling((int) e1.getX(0), (int) e1.getY(0), (int) velocityX, (int) velocityY);
                        return true;
                    }

                    @Override
                    public void onShowPress(MotionEvent e) {
                        mShowPressIsCalled = true;
                        if (mNativeChromeView != 0) {
                            nativeShowPressState(mNativeChromeView, (int) e.getX(), (int) e.getY());
                        }
                    }

                    @Override
                    public boolean onSingleTapUp(MotionEvent e) {
                        if (isDistanceBetweenDownAndUpTooLong(e.getRawX(), e.getRawY())) {
                            mIgnoreSingleTap = true;
                            return true;
                        }
                        // This is a hack to address the issue where user hovers
                        // over a link for longer than DOUBLE_TAP_TIMEOUT, then
                        // onSingleTapConfirmed() is not triggered. But we still
                        // want to trigger the tap event at UP. So we override
                        // onSingleTapUp() in this case. This assumes singleTapUp
                        // gets always called before singleTapConfirmed.
                        if (!mIgnoreSingleTap && !mGestureDetectorProxy.isInLongPress()) {
                            if (e.getEventTime() - e.getDownTime() > DOUBLE_TAP_TIMEOUT) {
                                float x = e.getX();
                                float y = e.getY();
                                if (mNativeChromeView != 0) {
                                    nativeSingleTap(mNativeChromeView, (int) x, (int) y, true);
                                    mIgnoreSingleTap = true;
                                }
                                setClickXAndY((int) x, (int) y);
                                return true;
                            } else if (mNativeMinimumScale == mNativeMaximumScale) {
                                // If page is not user scalable, we don't need to wait
                                // for double tap timeout.
                                float x = e.getX();
                                float y = e.getY();
                                handleTapOrPress(x, y, false);
                                mIgnoreSingleTap = true;
                            }
                        }
                        return false;
                    }

                    @Override
                    public boolean onSingleTapConfirmed(MotionEvent e) {
                        // Long taps in the edges of the screen have their events delayed by
                        // ChromeViewHolder for tab swipe operations. As a consequence of the delay
                        // this method might be called after receiving the up event.
                        // These corner cases should be ignored.
                        if (mGestureDetectorProxy.isInLongPress() || mIgnoreSingleTap) return true;

                        float x = e.getX();
                        float y = e.getY();
                        handleTapOrPress(x, y, false);
                        return true;
                    }

                    @Override
                    public boolean onDoubleTap(MotionEvent e) {
                        if (mNativeChromeView != 0) {
                            nativeDoubleTap(mNativeChromeView, (int) e.getX(), (int) e.getY());
                        }
                        return true;
                    }

                    @Override
                    public void onLongPress(MotionEvent e) {
                        if (mScaleGestureDetector == null || !mScaleGestureDetector.isInProgress()) {
                            float x = e.getX();
                            float y = e.getY();
                            handleTapOrPress(x, y, true);
                        }
                    }

                    /**
                     * This method inspects the distance between where the user started touching
                     * the surface, and where she released. If the points are too far apart, we
                     * should assume that the web page has consumed the scroll-events in-between,
                     * and as such, this should not be considered a single-tap.
                     *
                     * We use the Android frameworks notion of how far a touch can wander before
                     * we think the user is scrolling.
                     *
                     * @param x the new x coordinate
                     * @param y the new y coordinate
                     * @return true if the distance is too long to be considered a single tap
                     */
                    private boolean isDistanceBetweenDownAndUpTooLong(float x, float y) {
                        double deltaX = mLastRawX - x;
                        double deltaY = mLastRawY - y;
                        return deltaX * deltaX + deltaY * deltaY > mScaledTouchSlopSquare;
                    }
                };
            mGestureDetectorProxy = new GestureDetectorProxy(
                context, new GestureDetector(context, listener), listener);

            mScaleGestureDetector = new ScaleGestureDetector(
                context, new ScaleGestureDetector.OnScaleGestureListener() {

                    @Override
                    public void onScaleEnd(ScaleGestureDetector detector) {
                        // Send pinchEnd if we sent any pinch events before even if javascript
                        // handled finger release.
                        if (mPinchEventSent) {
                            pinchEnd();
                            mPinchEventSent = false;
                        }
                        mHideHandleTemporarilyForPinch = false;
                        fadeinHandleIfNeeded();
                    }

                    @Override
                    public boolean onScaleBegin(ScaleGestureDetector detector) {
                        if (mIgnoreScaleGestureDetectorEvents)
                            return false;
                        closeAutofillPopup();
                        mPinchEventSent = false;
                        mIgnoreSingleTap = true;
                        return true;
                    }

                    @Override
                    public boolean onScale(ScaleGestureDetector detector) {
                        if (mIgnoreScaleGestureDetectorEvents)
                            return false;
                        // It is possible that pinchBegin() was never called when we reach here.
                        // This happens when webkit handles the 2nd touch down event. That causes
                        // chromeview to ignore the onScaleBegin() call. And if webkit does not
                        // handle the touch move events afterwards, we will face a situation
                        // that pinchBy() is called without any pinchBegin().
                        // To solve this problem, we call pinchBegin() here if it is never called.
                        if (!mPinchEventSent) {
                            startNativeScrolling((int) detector.getFocusX(),
                                    (int) detector.getFocusY());
                            pinchBegin();
                            mPinchEventSent = true;
                        }
                        float scaleFactor = detector.getScaleFactor();
                        pinchBy(scaleFactor, (int) detector.getFocusX(), (int) detector.getFocusY());
                        mHideHandleTemporarilyForPinch = true;
                        hideHandleTemporarily();
                        return true;
                    }
                });
        } finally {
            TraceEvent.end();
        }
    }

    private float calculateDragAngle(float dx, float dy) {
        dx = Math.abs(dx);
        dy = Math.abs(dy);
        return (float) Math.atan2(dy, dx);
    }

    /*
     * To avoid checkerboard, we clamp the fling velocity based on the maximum number of tiles
     * allowed to be uploaded per 100ms. Calculation is limited to one direction. We assume the
     * tile size is 256x256. The precised distance / velocity should be calculated based on the
     * logic in Scroller.java. As it is almost linear for the first 100ms, we use a simple math.
     */
    private int clampFlingVelocityX(int velocity) {
        int cols = MAX_NUM_UPLOAD_TILES / (int) (Math.ceil((float) getHeight() / 256) + 1);
        int maxVelocity = cols > 0 ? cols * 2560 : 1000;
        if (Math.abs(velocity) > maxVelocity) {
            return velocity > 0 ? maxVelocity : -maxVelocity;
        } else {
            return velocity;
        }
    }

    private int clampFlingVelocityY(int velocity) {
        int rows = MAX_NUM_UPLOAD_TILES / (int) (Math.ceil((float) getWidth() / 256) + 1);
        int maxVelocity = rows > 0 ? rows * 2560 : 1000;
        if (Math.abs(velocity) > maxVelocity) {
            return velocity > 0 ? maxVelocity : -maxVelocity;
        } else {
            return velocity;
        }
    }

    void selectPopupMenuItems(int[] indices) {
        if (mNativeChromeView != 0) {
            nativeSelectPopupMenuItems(mNativeChromeView, indices);
        }
    }

    // Set the autofill window's position in the proper space (window space).
    // Window can be open or closed.
    private void setAutofillWindowPosition() {
        if (mAutofillWindow != null && mLastPressRect != null) {
            // Convert to window space
            Rect r = new Rect((int) (mLastPressRect.left * mNativePageScaleFactor - mNativeScrollX),
                              (int) (mLastPressRect.top * mNativePageScaleFactor - mNativeScrollY),
                              (int) (mLastPressRect.right * mNativePageScaleFactor - mNativeScrollX),
                              (int) (mLastPressRect.bottom * mNativePageScaleFactor - mNativeScrollY));
            mAutofillWindow.setPositionAround(r);
        }
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void openAutofillPopup(AutofillData[] data) {
        Log.v(TAG, "openAutofillPopup");
        if (mAutofillWindow == null) {
            mAutofillWindow = new AutofillWindow(
                    this,
                    mAutofillResourceId,
                    mAutofillNameTextViewResourceId,
                    mAutofillLabelTextViewResourceId,
                    data);
        } else {
            mAutofillWindow.setAutofillData(data);
        }
        if (mLastPressRect != null) {
            setAutofillWindowPosition();
            mAutofillWindow.show();
        }
    }

    void autofillPopupSelected(int queryId, int listIndex, int uniqueId) {
        if (mNativeChromeView != 0) {
            nativeAutofillPopupSelected(mNativeChromeView, queryId, listIndex, uniqueId);

            // As text in input field has changed, need to restartInput so IME is in
            // correct state.
            InputMethodManager manager = (InputMethodManager)
                    getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            manager.restartInput(this);
        }
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void closeAutofillPopup() {
        if (mAutofillWindow == null) {
            return;
        }
        Log.v(TAG, "closeAutofillPopup");
        mAutofillWindow.dismiss();
        mAutofillWindow = null;
        if (mNativeChromeView != 0) {
            nativeAutofillPopupClosed(mNativeChromeView);
        }
    }

    AutofillWindow getAutofillWindow() {
        return mAutofillWindow;
    }

    /**
     * Notifies the renderer that a custom context menu was selected.
     * @param action The action of the custom context menu that was selected.
     * @hide
     */
    public void onCustomMenuAction(int action) {
        nativeOnCustomMenuAction(mNativeChromeView, action);
    }

    /**
     * Get the screen orientation from the OS and push it to WebKit.
     *
     * TODO(husky): Add a hook for mock orientations.
     *
     * TODO(husky): Currently each new tab starts with an orientation of 0 until
     * you actually rotate the device. This is wrong if you actually started in
     * landscape mode. To fix this, we need to push the correct orientation, but
     * only after WebKit's Frame object has been fully initialized. Need to find
     * a good time to do that. onPageFinished() would probably work but it isn't
     * implemented yet.
     */
    private void sendOrientationChange() {
        if (mNativeChromeView == 0) {
            return;
        }
        WindowManager windowManager = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        switch (windowManager.getDefaultDisplay().getRotation()) {
            case Surface.ROTATION_90:
                nativeOrientationChange(mNativeChromeView, 90);
                break;
            case Surface.ROTATION_180:
                nativeOrientationChange(mNativeChromeView, 180);
                break;
            case Surface.ROTATION_270:
                nativeOrientationChange(mNativeChromeView, -90);
                break;
            case Surface.ROTATION_0:
                nativeOrientationChange(mNativeChromeView, 0);
                break;
            default:
                Log.w(TAG, "Unknown rotation!");
                break;
        }
    }

    /**
     * The host activity should call this during its onPause() handler to ensure
     * all state is saved when the app is suspended. This method is synchronous.
     */
    public static void flushPersistentData() {
        TraceEvent.begin();
        nativeFlushPersistentData();
        TraceEvent.end();
    }

    /**
     * Register the interface to be used when content can not be handled by the
     * rendering engine, and should be downloaded instead. This will replace the
     * current handler.
     *
     * @param listener An implementation of DownloadListener.
     */
    // TODO: decide if setDownloadListener2 will be public API. If so, this
    // method should be deprecated and the javadoc should make reference to the
    // fact that a DownloadListener2 will be used in preference to a
    // DownloadListener.
    public void setDownloadListener(DownloadListener listener) {
        mDownloadListener = listener;
    }

    // Called by DownloadController.
    DownloadListener downloadListener() {
        return mDownloadListener;
    }

    /**
     * Register the interface to be used when content can not be handled by
     * the rendering engine, and should be downloaded instead. This will replace
     * the current handler or existing DownloadListner.
     * @param listener An implementation of DownloadListener2.
     */
    public void setDownloadListener2(DownloadListener2 listener) {
        mDownloadListener2 = listener;
    }

    // Called by DownloadController.
    DownloadListener2 downloadListener2() {
        return mDownloadListener2;
    }

    private SelectionHandleController getSelectionHandleController() {
        if (mSelectionHandleController == null) {
            mSelectionHandleController = new SelectionHandleController(this) {
                @Override
                public void selectBetweenCoordinates(int x1, int y1, int x2, int y2) {
                    if (mNativeChromeView != 0 && !(x1 == x2 && y1 == y2)) {
                        nativeSelectBetweenCoordinates(mNativeChromeView, x1, y1, x2, y2);
                    }
                }

                @Override
                public void showHandlesAt(int x1, int y1, int dir1, int x2, int y2, int dir2) {
                    super.showHandlesAt(x1, y1, dir1, x2, y2, dir2);
                    mStartHandleNormalizedPoint.set(
                        (x1 + mNativeScrollX) / mNativePageScaleFactor,
                        (y1 + mNativeScrollY) / mNativePageScaleFactor);
                    mEndHandleNormalizedPoint.set(
                        (x2 + mNativeScrollX) / mNativePageScaleFactor,
                        (y2 + mNativeScrollY) / mNativePageScaleFactor);

                    showSelectActionBar();
                }

            };

            mSelectionHandleController.hideAndDisallowAutomaticShowing();
        }

        return mSelectionHandleController;
    }

    private InsertionHandleController getInsertionHandleController() {
        if (mInsertionHandleController == null) {
            mInsertionHandleController = new InsertionHandleController(this) {
                private static final int AVERAGE_LINE_HEIGHT = 14;

                @Override
                public void setCursorPosition(int x, int y) {
                    if (mNativeChromeView != 0) {
                        nativeSelectBetweenCoordinates(mNativeChromeView, x, y, x, y);
                    }
                }

                @Override
                public void paste() {
                    mImeAdapter.paste();
                    hideHandles();
                }

                @Override
                public int getLineHeight() {
                    return (int) (mNativePageScaleFactor * AVERAGE_LINE_HEIGHT);
                }

                @Override
                public void showHandleAt(int x, int y) {
                    super.showHandleAt(x, y);
                    mInsertionHandleNormalizedPoint.set(
                        (x + mNativeScrollX) / mNativePageScaleFactor,
                        (y + mNativeScrollY) / mNativePageScaleFactor);
                }
            };

            mInsertionHandleController.hideAndDisallowAutomaticShowing();
        }

        return mInsertionHandleController;
    }

    private void hideHandleTemporarily() {
        if (mSelectionHandleController != null && mSelectionHandleController.isShowing()) mSelectionHandleController.hideTemporarily();
        if (mInsertionHandleController != null && mInsertionHandleController.isShowing()) mInsertionHandleController.hideTemporarily();
    }

    private void fadeinHandleIfNeeded() {
        if (mHideHandleTemporarilyForScroll || mHideHandleTemporarilyForPinch) return;

        if (!(mSelectionHandleController != null && mSelectionHandleController.isHiddenTemporarily())
          && !(mInsertionHandleController != null && mInsertionHandleController.isHiddenTemporarily())) return;

        if (mDeferredUndoHideHandleRunnable == null) {
            mDeferredUndoHideHandleRunnable = new Runnable() {
                public void run() {
                    mDeferredUndoHideHandleRunnableScheduled = false;
                    if (mHideHandleTemporarilyForScroll || mHideHandleTemporarilyForPinch) return;

                    if (mSelectionHandleController != null) mSelectionHandleController.undoHideTemporarily();
                    if (mInsertionHandleController != null) mInsertionHandleController.undoHideTemporarily();
                }
            };
        }

        Handler handler = getHandler();
        if (handler == null) {
            mDeferredUndoHideHandleRunnableScheduled = false;
            return;
        }

        if (mDeferredUndoHideHandleRunnableScheduled) {
            mDeferredUndoHideHandleRunnableScheduled = false;
            handler.removeCallbacks(mDeferredUndoHideHandleRunnable);
        }

        mDeferredUndoHideHandleRunnableScheduled = true;
        handler.postDelayed(mDeferredUndoHideHandleRunnable, 300);
    }

    private void updateHandleScreenPositions() {
        if (mSelectionHandleController != null && mSelectionHandleController.isShowing()) {
            mSelectionHandleController.setStartHandlePosition(
                    (int)(mStartHandleNormalizedPoint.x * mNativePageScaleFactor - mNativeScrollX),
                    (int)(mStartHandleNormalizedPoint.y * mNativePageScaleFactor - mNativeScrollY));
            mSelectionHandleController.setEndHandlePosition(
                    (int)(mEndHandleNormalizedPoint.x * mNativePageScaleFactor - mNativeScrollX),
                    (int)(mEndHandleNormalizedPoint.y * mNativePageScaleFactor - mNativeScrollY));
        }

        if (mInsertionHandleController != null && mInsertionHandleController.isShowing()) {
            mInsertionHandleController.setHandlePosition(
                    (int)(mInsertionHandleNormalizedPoint.x * mNativePageScaleFactor - mNativeScrollX),
                    (int)(mInsertionHandleNormalizedPoint.y * mNativePageScaleFactor - mNativeScrollY));
        }
    }

    private void hideHandles() {
        if (mSelectionHandleController != null) {
            mSelectionHandleController.hideAndDisallowAutomaticShowing();
        }
        if (mInsertionHandleController != null) {
            mInsertionHandleController.hideAndDisallowAutomaticShowing();
        }
    }

    private void showSelectActionBar() {
        if (mActionMode != null) {
            mActionMode.invalidate();
            return;
        }

        // Start a new action mode with a SelectActionModeCallback.
        SelectActionModeCallback.ActionHandler actionHandler =
                new SelectActionModeCallback.ActionHandler() {
            @Override
            public boolean selectAll() {
                return mImeAdapter.selectAll();
            }

            @Override
            public boolean cut() {
                return mImeAdapter.cut();
            }

            @Override
            public boolean copy() {
                return mImeAdapter.copy();
            }

            @Override
            public boolean paste() {
                return mImeAdapter.paste();
            }

            @Override
            public boolean isSelectionEditable() {
                return mSelectionEditable;
            }

            @Override
            public String getSelectedText() {
                return ChromeView.this.getSelectedText();
            }

            @Override
            public void onDestroyActionMode() {
                mActionMode = null;
                mImeAdapter.unselect();
                getChromeViewClient().onContextualActionBarHidden();
            }
        };
        mActionMode = startActionMode(
                new SelectActionModeCallback(getContext(), actionHandler, isIncognito()));

        // TODO(jcivelli): http://b/6308469 temporary disable animation as they can cause a crasher.
        disableActionBarAnimation();

        if (mActionMode == null) {
            // There is no ActionMode, so remove the selection.
            mImeAdapter.unselect();
        } else {
            getChromeViewClient().onContextualActionBarShown();
        }
    }

    private void disableActionBarAnimation() {
        ActionBar actionBar = ((Activity) getContext()).getActionBar();
        if (actionBar == null) {
            Log.w(TAG, "Failed to disable action bar animations, no action bar.");
            return;
        }
        try {
            Class clazz = Class.forName("com.android.internal.app.ActionBarImpl");
            Method setShowHideAnimationEnabledMethod =
                    clazz.getDeclaredMethod("setShowHideAnimationEnabled", boolean.class);
            setShowHideAnimationEnabledMethod.invoke(actionBar, false);
        } catch (ClassNotFoundException cnfe) {
            Log.e(TAG, "Failed to find ActionBarImpl class to disable animation.");
        } catch (NoSuchMethodException nsme) {
            Log.e(TAG, "Failed to find ActionBarImpl disabling animation method.");
        } catch (InvocationTargetException ite) {
            Log.e(TAG, "Error disabling animation from ActionBar.", ite);
        } catch (IllegalAccessException iae) {
            Log.e(TAG, "Error disabling animation from ActionBar.", iae);
        }
    }

    public boolean getUseDesktopUserAgent() {
        if (mNativeChromeView != 0) {
            return nativeGetUseDesktopUserAgent(mNativeChromeView);
        }
        return false;
    }

    /**
     * Set whether or not we're using a desktop user agent for the currently loaded page.
     * @param override If true, use a desktop user agent.  Use a mobile one otherwise.
     * @param reloadOnChange Reload the page if the UA has changed.
     */
    public void setUseDesktopUserAgent(boolean override, boolean reloadOnChange) {
        if (mNativeChromeView != 0) {
            nativeSetUseDesktopUserAgent(mNativeChromeView, override, reloadOnChange);
        }
    }

    boolean getJavaScriptEnabled() {
        if (mNativeChromeView != 0) {
            return nativeGetJavaScriptEnabled(mNativeChromeView);
        }
        return false;
    }

    /**
     * @return Whether the native ChromeView has crashed.
     */
    public boolean isCrashed() {
        if (mNativeChromeView == 0) return false;
        return nativeCrashed(mNativeChromeView);
    }

    // The following methods are called by native through jni

    @SuppressWarnings("unused")
    @CalledByNative
    private void updateContentSize(int width, int height) {
        if (mContentWidth != width || mContentHeight != height) {
            mPopupZoomer.hide(true);
        }
        // Make sure the content size is at least the View size
        mContentWidth = Math.max(width, getWidth());
        mContentHeight = Math.max(height, getHeight());
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void updateScrollOffsetAndPageScaleFactor(int x, int y, float scale) {
        if (mNativeScrollX == x && mNativeScrollY == y && mNativePageScaleFactor == scale)
            return;

        onScrollChanged(x, y, mNativeScrollX, mNativeScrollY);

        // This function should be called back from native as soon
        // as the scroll is applied to the backbuffer.  We should only
        // update mNativeScrollX/Y here for consistency.
        mNativeScrollX = x;
        mNativeScrollY = y;
        mNativePageScaleFactor = scale;

        mPopupZoomer.hide(true);
        setAutofillWindowPosition();
        hideHandleTemporarily();
        fadeinHandleIfNeeded(); // this scheduled a deferred unhide if appropriate
        updateHandleScreenPositions();
        if (mTextureView == null)
            awakenScrollBars();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void updatePageScaleLimits(float minimumScale, float maximumScale) {
        mNativeMinimumScale = minimumScale;
        mNativeMaximumScale = maximumScale;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void imeUpdateAdapter(int nativeImeAdapterAndroid, int textInputType,
            int cursorX, int cursorY, int cursorBottom, int cursorRight,
            String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd, boolean showImeIfNeeded,
            long requestTime) {
        TraceEvent.begin();

        // Non-breaking spaces can cause the IME to get confused. Replace with normal spaces.
        text = text.replace('\u00A0', ' ');

        mSelectionEditable = (textInputType != ImeAdapter.sTextInputTypeNone);
        if (mActionMode != null) mActionMode.invalidate();

        mImeAdapter.attachAndShowIfNeeded(nativeImeAdapterAndroid, textInputType,
                text, showImeIfNeeded);

        // In WebKit if there's a composition then the selection may be the
        // same as the composition, whereas Android IMEs expect the selection to be
        // just a caret at the end of the composition.
        if (selectionStart == compositionStart && selectionEnd == compositionEnd) {
            selectionStart = selectionEnd;
        }
        mImeAdapter.setEditableText(text, selectionStart, selectionEnd,
                compositionStart, compositionEnd, requestTime);

        InputMethodManager manager = (InputMethodManager)
                getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        if (manager.isWatchingCursor(this)) {
            // TODO(bulach): we need to translate from the widget coordinate to
            // this view.
            manager.updateCursor(this, cursorX, cursorY, cursorRight, cursorBottom);
        }
        TraceEvent.end();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void setLastPressAck(float x, float y, float width, float height) {
        // Normalize to 1.0-scale coordinates.
        mLastPressRect = new RectF(x / mNativePageScaleFactor,
                                   y / mNativePageScaleFactor,
                                   (x + width) / mNativePageScaleFactor,
                                   (y + height) / mNativePageScaleFactor);
        setAutofillWindowPosition();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void resetLastPressAck() {
        mLastPressRect = null;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void setTitle(String title) {
        getChromeViewClient().onUpdateTitle(title);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void showSelectPopup(String[] items, int[] enabled, boolean multiple,
            int[] selectedIndices) {
        SelectPopupDialog.show(this, items, enabled, multiple, selectedIndices);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void confirmTouchEvent(boolean handled) {
        if (mPendingTouchAcksToIgnore > 0) {
            // Webkit finished processing the touch event, but the touch event has
            // already be consumed by gesture detector.
            mPendingTouchAcksToIgnore--;
            return;
        }

        consumePendingTouchEvents(handled, false);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void didSetNeedTouchEvents(boolean needTouchEvents) {
        mNeedTouchEvents = needTouchEvents;
        // When mainframe is loading, FrameLoader::transitionToCommitted will
        // call this method to set mNeedTouchEvents to false. We use this as
        // an indicator to clear the pending motion events so that events from
        // the previous page will not be carried over to the new page.
        if (!mNeedTouchEvents) {
            mPendingMotionEvents.clear();
            mHandler.removeCallbacks(mWebKitTimeoutRunnable);
            mPendingTouchAcksToIgnore = 0;
            mDisableWebKitTimeout = false;
            mSkipSendToNative = false;
        }
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onFindResultAvailable(FindResultReceivedListener.FindNotificationDetails details) {
        if (mFindResultReceivedListener != null) {
            mFindResultReceivedListener.onFindResultReceived(details);
        }
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onMultipleTargetsTouched(Rect rect) {
        mPopupZoomer.show(rect);
    }

    @SuppressWarnings("unused")
    // TODO(bulach): this should have either a throw clause, or
    // handle the exception in the java side rather than the native side.
    @CalledByNativeUnchecked
    private Bitmap createNewOnDemandBitmap(int w, int h, ByteBuffer memory) {
        Bitmap zoomedBitmap =
            Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        zoomedBitmap.copyPixelsFromBuffer(memory);
        mPopupZoomer.setBitmap(zoomedBitmap);
        return zoomedBitmap;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onSelectionChanged(String text) {
        mLastSelectedText = text;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onSelectionBoundsChanged(int x1, int y1, int dir1, int x2, int y2, int dir2) {
        if (x1 != x2 || y1 != y2) {
            if (mInsertionHandleController != null) {
                mInsertionHandleController.hide();
            }
            getSelectionHandleController().onSelectionChanged(x1, y1, dir1, x2, y2, dir2);
            mHasSelection = true;
        } else {
            hideSelectActionBar();
            if (x1 != 0 && y1 != 0
                    && (mSelectionHandleController == null
                            || !mSelectionHandleController.isDragging())
                    && mSelectionEditable) {
                // Selection is a caret, and a text field is focused.
                if (mSelectionHandleController != null) {
                    mSelectionHandleController.hide();
                }
                getInsertionHandleController().onCursorPositionChanged(x1, y1);
            } else {
                // Deselection
                if (mSelectionHandleController != null && !mSelectionHandleController.isDragging()) {
                    mSelectionHandleController.hideAndDisallowAutomaticShowing();
                }
                if (mInsertionHandleController != null) {
                    mInsertionHandleController.hideAndDisallowAutomaticShowing();
                }
            }
            mHasSelection = false;
        }
    }

    @SuppressWarnings("unused")
    @CalledByNative
    void onEvaluateJavaScriptResult(int id, String jsonResult) {
        getChromeViewClient().onEvaluateJavaScriptResult(id, jsonResult);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    void onNewTabPageReady() {
        getChromeViewClient().onNewTabPageReady();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void showContextMenu(ChromeViewContextMenuInfo contextMenuInfo) {
        mContextMenuInfo = contextMenuInfo;

        // If the view has been detached from the window, this is probably a stale event and
        // should be ignored.
        if (!mAttachedToWindow) {
          return;
        }

        // TODO(olilan): Add implementation of getHitTestResult for WebView compatibility

        if (contextMenuInfo.isEditable && !contextMenuInfo.isSelectedText) {
            getInsertionHandleController().showHandleWithPastePopupAt(
                    contextMenuInfo.selectionStartX - mNativeScrollX,
                    contextMenuInfo.selectionStartY - mNativeScrollY);
        } else if (contextMenuInfo.isAnchor || contextMenuInfo.isEditable
                || contextMenuInfo.isImage || contextMenuInfo.isSelectedText) {
            performLongClick();
        } else if (contextMenuInfo.isCustomMenu()) {
            showCustomContextMenu(contextMenuInfo);
        }
    }

    private void showCustomContextMenu(final ChromeViewContextMenuInfo contextMenuInfo) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        String[] items = new String[contextMenuInfo.getCustomItemSize()];
        for (int i = 0; i < items.length; ++i) {
            items[i] = contextMenuInfo.getCustomItemAt(i).mLabel;
        }
        builder.setItems(items, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int position) {
                int action = contextMenuInfo.getCustomItemAt(position).mAction;
                onCustomMenuAction(action);
            }
        });
        AlertDialog dialog = builder.create();
        WindowManager.LayoutParams params = dialog.getWindow().getAttributes();
        params.gravity = Gravity.TOP | Gravity.LEFT;
        params.x = (int) mLastRawX;
        params.y = (int) mLastRawY;
        dialog.getWindow().setAttributes(params);
        dialog.show();
    }


    @SuppressWarnings("unused")
    private void showSelectFileDialog(SelectFileDialog dialog) {
        getChromeViewClient().showSelectFileDialog(dialog);
    }


    private void addTextureView() {
        if (mTextureView != null && indexOfChild(mTextureView) == -1) {
            addView(mTextureView, 0, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
        }
    }

    private void removeTextureView() {
        if (mTextureView != null) {
            removeView(mTextureView);
        }
    }

    private void removeTextureViewAndDetachProducer() {
        ExternalSurfaceTextureOwner externalOwner = mExternalSurfaceTextureOwner;
        if (externalOwner != null) {
            externalOwner.onNewSurfaceTexture(null, false);
            // Setting the External to null to force the release of the SurfaceTexture.
            mExternalSurfaceTextureOwner = null;
        }
        // This avoids creating a new SurfaceTexture, while we have not freed the
        // the last one. Once the GL thread has detached the surface and we released it,
        // we will re-add the TextureView.
        if (mTextureView != null && indexOfChild(mTextureView) != -1) {
            removeView(mTextureView);
        } else if (mTextureView != null && mSurfaceTextureWiredToProducer != null) {
            // In case the ChromeView is detached from the Window we still need to release the
            // producer. As the TextureView does not own the SurfaceTexture the producer need to
            // be detached manually.
            TextureView.SurfaceTextureListener listener = mTextureView.getSurfaceTextureListener();
            listener.onSurfaceTextureDestroyed(mSurfaceTextureWiredToProducer);
        }
        mExternalSurfaceTextureOwner = externalOwner;
    }

    /**
    * Set an {@link ExternalSurfaceTextureOwner} that will take ownership of the TextureView's
    * {@link SurfaceTexture}.
    * Be sure to call {@link #onExternalSurfaceTextureUpdated()} when the {@link SurfaceTexture}
    * get updated with {@link SurfaceTexture#updateTexImage()}.
    * @return Whether the {@link ExternalSurfaceTextureOwner} has been updated as intended.
    */
    public boolean setExternalSurfaceTextureOwner(ExternalSurfaceTextureOwner owner) {
        if (!SurfaceTextureSwapConsumer.isSupported()) return false;
        if (mExternalSurfaceTextureOwner == owner) return true;
        if (mExternalSurfaceTextureOwner != null) {
            mExternalSurfaceTextureOwner.onNewSurfaceTexture(null, false);
        }
        mExternalSurfaceTextureOwner = owner;

        // If there is no TextureView yet, then the owner may be notified later when the
        // SurfaceTexture get created.
        if (mTextureView == null) return true;

        // Taking the SurfaceTexture from the TextureView
        if (owner != null) {
            if (indexOfChild(mTextureView) == -1) {
                owner.onNewSurfaceTexture(mSurfaceTextureWiredToProducer, isReady());
            } else {
                // removeView will eventually call owner.onNewSurfaceTexture
                removeTextureView();
            }
        // Giving back the SurfaceTexture to the TextureView
        } else {
            if (mSurfaceTextureWiredToProducer != null) {
                mSurfaceTextureWiredToProducer.setOnFrameAvailableListener(null);
                SurfaceTextureSwapConsumer.setSurfaceTexture(
                        mTextureView, mSurfaceTextureWiredToProducer);
            }
            addTextureView();
        }
        return true;
    }

    /**
     * To be called by the {@link ExternalSurfaceTextureOwner} after the {@link SurfaceTexture}
     * get updated with {@link SurfaceTexture#updateTexImage()}.
     */
    public void onExternalSurfaceTextureUpdated() {
        assert ThreadUtils.runningOnUiThread() : "This must run on the UI thread";
        if (mExternalSurfaceTextureOwner != null && mSurfaceTextureWiredToProducer != null) {
            onProducerFrameUpdated(mSurfaceTextureWiredToProducer);
        }
    }

    private final Runnable mProducerFrameAvailableRunnable = new Runnable() {
        @Override
        public void run() {
            assert ThreadUtils.runningOnUiThread() : "This must run on the UI thread";
            if (mSurfaceTextureUpdatedListeners != null) {
                final int listenerSize = mSurfaceTextureUpdatedListeners.size();
                for (int i = 0; i < listenerSize; ++i) {
                    mSurfaceTextureUpdatedListeners.get(i).onFrameAvailable(ChromeView.this);
                }
            }
        }
    };

    private void onProducerFrameUpdated(SurfaceTexture surfaceTexture) {
        TraceEvent.instant("onSurfaceTextureUpdated");

        // TODO(dtrainor, jrg): We may have unmatched end calls; fix to match?
        // perf(dtrainor): Please don't rename or remove.
        PerfTraceEvent.end("TabStrip:TabSelected");

        // perf(dtrainor): Please don't rename or remove.
        PerfTraceEvent.end("TabStrip:TabClosed");
        mCompositorFrameTimestamp = surfaceTexture.getTimestamp();

        boolean previousReadyStatus = isReady();

        updateTextureViewStatus();
        updateVSync();

        if (mSurfaceTextureUpdatedListeners != null) {
            final int listenerSize = mSurfaceTextureUpdatedListeners.size();
            for (int i = 0; i < listenerSize; ++i) {
                mSurfaceTextureUpdatedListeners.get(i).onSurfaceTextureUpdated(ChromeView.this);
            }
        }

        if (mLogFPS || mProfileFPS) logFps();

        if (TraceEvent.enabled()) {
            getHandler().postAtFrontOfQueue(new Runnable() {
                @Override
                public void run() {
                    bufferSwapped();
                } });
        }

        // Since we are triple-buffered our SwapBuffers ACK is sent now after we
        // dequeued the new frame, since that essentially means a new buffer is
        // available to the producer to render into.
        if (mNativeChromeView != 0)
            nativeAcknowledgeSwapBuffers(mNativeChromeView);
    }

    @CalledByNative
    private void activateHardwareAcceleration(boolean activated, final int pid, final int type,
            final int primaryID, final int secondaryID) {
        if (!activated) {
            if (mTextureView != null) {
                removeTextureViewAndDetachProducer();
                removeView(mPlaceholderView);
                mTextureView = null;
                mPlaceholderView = null;
                mTextureViewStatus = TextureViewStatus.INITIALIZING;
            }
            return;
        }
        TraceEvent.begin();
        if (mTextureView != null) {
            Log.w("ChromeView", "We should have only one attached TextureView. Hmm...");
            removeTextureViewAndDetachProducer();
            mTextureViewStatus = TextureViewStatus.INITIALIZING;
        }
        if (mPlaceholderView != null) {
            Log.w("ChromeView", "We should have only one attached PlaceholderView. Hmm...");
            removeView(mPlaceholderView);
        }

        mTextureView = new TextureView(getContext());

        TextureView.SurfaceTextureListener listener = new TextureView.SurfaceTextureListener() {
            private int mSurfaceID = 0;

            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture,
                    int width, int height) {
                // TODO(jscholler): remove this ugly test once the onSurfaceTextureAvailable()
                // callback is not called after SetSurfaceTexture
                if (mSurfaceTextureWiredToProducer != null) {
                    assert mSurfaceID != 0;
                    return;
                }
                mSurfaceTextureWiredToProducer = surfaceTexture;
                Surface surface = new Surface(surfaceTexture);
                // pid == 0 means the current process is the target process.
                if (pid == 0) {
                    // Calling nativeSetSurface will release the surface.
                    nativeSetSurface(surface, primaryID, secondaryID, 0);
                } else {
                    SandboxedProcessLauncher.establishSurfacePeer(pid, type, surface, primaryID,
                            secondaryID);
                    // The SandboxProcessService now holds a reference to the Surface's resources,
                    // so we release our reference to it now to avoid waiting for the finalizer
                    // to get around to it.
                    surface.release();
                }
                mSurfaceID = nativeGetSurfaceID(secondaryID, primaryID);

                // In case the externalSurfaceTexture owner is set it steals the SurfaceTexture.
                if (mExternalSurfaceTextureOwner != null) {
                    mHandler.postAtFrontOfQueue(new Runnable() {
                        @Override
                        public void run() {
                            removeTextureView();
                        }
                    });
                }
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width,
                    int height) {
                if (width != mCompositorWidth || height != mCompositorHeight) {
                    beginTextureViewResize(getWidth(), getHeight());
                }
            }

            @Override
            public boolean onSurfaceTextureDestroyed(final SurfaceTexture surfaceTexture) {
                if (mExternalSurfaceTextureOwner != null) {
                    surfaceTexture.setOnFrameAvailableListener(
                            new SurfaceTexture.OnFrameAvailableListener() {
                                @Override
                                public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                                    // Make sure the event is propagated from the UI thread.
                                    if (ThreadUtils.runningOnUiThread()) {
                                        mProducerFrameAvailableRunnable.run();
                                    } else {
                                        mHandler.postAtFrontOfQueue(mProducerFrameAvailableRunnable);
                                    }
                                }
                            });
                    mExternalSurfaceTextureOwner.onNewSurfaceTexture(surfaceTexture, isReady());
                    return false;
                }

                // This will kill the current rendering GL surface.  So we should clear the cached
                // compositor width and height.
                mCompositorWidth = 0;
                mCompositorHeight = 0;

                assert surfaceTexture == mSurfaceTextureWiredToProducer :
                    "unexpected SurfaceTexture destroyed";
                mSurfaceTextureWiredToProducer = null;

                if (mSurfaceID == 0 || surfaceTexture == null) return false;
                // TODO(sievers): Do this more properly after crbug.com/119006.
                // Right now the problem is that routing back a reply through IPC is flaky for the
                // purpose of releasing the surface, because the view can go away at any time.
                if (pid == 0) {
                    final WaitableNativeEvent completion = new WaitableNativeEvent();
                    nativeSetSurface(null, mSurfaceID, secondaryID, completion.mNativeInstance);
                    mWaitForNativeSurfaceDestroy = true;

                    // We have to wait for the producer to give up the surface before we can
                    // call release() or will will trigger errors in SwapBuffers() and MakeCurrent().
                    new AsyncTask<Void, Void, Void>() {
                        protected Void doInBackground(Void... v) {
                            boolean didSignal = completion.WaitOnce(2);
                            surfaceTexture.release();
                            if (!didSignal) Log.w(TAG, "Timed out waiting for surface to detach.");
                            return null;
                        }

                        protected void onPostExecute(Void v) {
                            mWaitForNativeSurfaceDestroy = false;
                            // We removed the TextureView to avoid creating another SurfaceTexture
                            // while were detaching the previous one.
                            addTextureView();
                        }
                    }.execute();
                } else {
                    SandboxedProcessLauncher.establishSurfacePeer(pid, type,
                            (Surface) null, mSurfaceID, secondaryID);
                    final SurfaceTexture finalSurfaceTexture = surfaceTexture;
                    postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            finalSurfaceTexture.release();
                        }
                    }, 2000);
                    addTextureView();
                }
                mTextureViewStatus = TextureViewStatus.INITIALIZING;
                if (mPlaceholderView != null) {
                    mPlaceholderView.show();
                }

                mSurfaceID = 0;
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
                onProducerFrameUpdated(surfaceTexture);
            }
        };

        mTextureView.setSurfaceTextureListener(listener);
        // Make TextureView not focusable so that all the events goes to ChromeView.
        mTextureView.setFocusable(false);
        mTextureView.setFocusableInTouchMode(false);
        addTextureView();

        // Add the PlaceHolderView on top so it can obscure possibly invalid content in the
        // TextureView. The view is shown until the first frame from the renderer after a resize.
        // As framework doesn't support requestLayout (which is called by addView) inside layout,
        // we need to add it first and adjust visibility later.
        mPlaceholderView = new PlaceholderView(getContext(), this);
        addView(mPlaceholderView);

        TraceEvent.end();
    }

    // This is called right after UI view calls eglSwapBuffers. Currently it is only used for
    // trace event.
    private void bufferSwapped() {
        TraceEvent.instant("bufferSwapped");
    }

    /**
     * Log FPS, producing a log message roughly every second.
     */
    private void logFps() {
        mFPSCount++;
        mProfileFPSCount++;
        long delta = SystemClock.uptimeMillis() - mFPSLastTime;
        if (delta > 1000) {
            Log.w("ChromeView", "ChromeView update fps = " + ((float) mFPSCount) / delta * 1000);
            mFPSCount = 0;
            mFPSLastTime = SystemClock.uptimeMillis();
        }
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onRenderProcessSwap(int oldPid, int newPid) {
        if (mAttachedToWindow && oldPid != newPid) {
            if (oldPid > 0) {
                SandboxedProcessLauncher.unbindAsHighPriority(oldPid);
            }
            if (newPid > 0) {
                SandboxedProcessLauncher.bindAsHighPriority(newPid);
            }
        }
    }

    @CalledByNative
    private void startContentIntent(String contentUrl) {
        getChromeViewClient().onStartContentIntent(this, contentUrl);
    }

    /**
     * Called (from native) when page loading begins.
     */
    @CalledByNative
    private void onPageStarted() {
        hidePopupDialog();
        mAccessibilityInjector.onPageLoadStarted();
    }

    @CalledByNative
    private void onPageFinished(String url) {
        mAccessibilityInjector.injectAccessibilityScriptIntoPage();
    }

    public boolean needsReload() {
        if (mNativeChromeView != 0) {
            return nativeNeedsReload(mNativeChromeView);
        }
        return false;
    }

    @Override
    @CalledByNative
    public void invalidate() {
        super.invalidate();
    }

    @Override
    @CalledByNative
    public void postInvalidate() {
        super.postInvalidate();
    }

    @Override
    @CalledByNative
    public boolean hasFocus() {
        return super.hasFocus();
    }

    /**
     * Enable or disable content:// URL support. For security reasons the browser will disable
     * content URL support automatically, but for a WebView content URLs are enabled by default.
     *
     * TODO(skyostil): This should be configured via WebSettings once that is supported.
     */
    public void setAllowContentAccess(boolean allow) {
        if (mNativeChromeView != 0) {
            nativeSetAllowContentAccess(mNativeChromeView, allow);
        }
    }

    /**
     * Enable or disable file:// URL support. Note that URLs starting with file://android_asset/
     * and file://android_res/ can always be accessed regardless of this setting.
     *
     * TODO(skyostil): This should be configured via WebSettings once that is supported.
     */
    public void setAllowFileAccess(boolean allow) {
        if (mNativeChromeView != 0) {
            nativeSetAllowFileAccess(mNativeChromeView, allow);
        }
    }

    /**
     * Checks whether the WebView can be zoomed in.
     *
     * @return True if the WebView can be zoomed in.
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public boolean canZoomIn() {
        return mNativePageScaleFactor < mNativeMaximumScale;
    }

    /**
     * Checks whether the WebView can be zoomed out.
     *
     * @return True if the WebView can be zoomed out.
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public boolean canZoomOut() {
        return mNativePageScaleFactor > mNativeMinimumScale;
    }

    /**
     * Zooms in the WebView by 25% (or less if that would result in zooming in
     * more than possible).
     *
     * @return True if there was a zoom change, false otherwise.
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public boolean zoomIn() {
        if (!canZoomIn()) {
            return false;
        }

        if (mNativeChromeView == 0) {
            return false;
        }

        pinchBegin();
        pinchBy(1.25f, getWidth() / 2, getHeight() / 2);
        pinchEnd();
        return true;
    }

    /**
     * Zooms out the WebView by 20% (or less if that would result in zooming out
     * more than possible).
     *
     * @return True if there was a zoom change, false otherwise.
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public boolean zoomOut() {
        if (!canZoomOut()) {
            return false;
        }

        if (mNativeChromeView == 0) {
            return false;
        }

        pinchBegin();
        pinchBy(0.8f, getWidth() / 2, getHeight() / 2);
        pinchEnd();
        return true;
    }

    /**
     * This method injects the supplied Java object into the WebView. The
     * object is injected into the JavaScript context of the main frame, using
     * the supplied name. This allows the Java object to be accessed from
     * JavaScript. Note that that injected objects will not appear in
     * JavaScript until the page is next (re)loaded. For example:
     * <pre> webView.addJavascriptInterface(new Object(), "injectedObject");
     * webView.loadData("<!DOCTYPE html><title></title>", "text/html", null);
     * webView.loadUrl("javascript:alert(injectedObject.toString())");</pre>
     * <p><strong>IMPORTANT:</strong>
     * <ul>
     * <li> addJavascriptInterface() can be used to allow JavaScript to control
     * the host application. This is a powerful feature, but also presents a
     * security risk. Use of this method in a WebView containing untrusted
     * content could allow an attacker to manipulate the host application in
     * unintended ways, executing Java code with the permissions of the host
     * application. Use extreme care when using this method in a WebView which
     * could contain untrusted content. To combat this, try setting
     * {@code allowInheritedMethods} to {@code false} so that no super methods
     * will be allowed to be called.  This protects JavaScript from potentially
     * accessing dangerous methods like {@link Object#getClass()} or
     * {@link Class#getClassLoader()}.  As an additional security measure, try to
     * make sure none of your allowed methods allow JavaScript to get access to a
     * raw {@link Object} class.
     * <li> JavaScript interacts with Java object on a private, background
     * thread of the WebView. Care is therefore required to maintain thread
     * safety.</li>
     * </ul></p>
     * @param object The Java object to inject into the WebView's JavaScript
     *               context. Null values are ignored.
     * @param name The name used to expose the instance in JavaScript.
     * @param allowInheritedMethods Whether or not to allow inherited methods to be called in
     *                              JavaScript.
     */
    public void addJavascriptInterface(Object object, String name, boolean allowInheritedMethods) {
        if (mNativeChromeView != 0 && object != null) {
            nativeAddJavascriptInterface(mNativeChromeView, object, name, allowInheritedMethods);
        }
    }

    /**
     * Removes a previously added JavaScript interface with the given name.
     * @param name The name of the interface to remove.
     */
    public void removeJavascriptInterface(String name) {
        if (mNativeChromeView != 0) {
            nativeRemoveJavascriptInterface(mNativeChromeView, name);
        }
    }

    /**
     * Called when context menu option to create the bookmark shortcut on homescreen is called.
     */
    @CalledByNative
    private void addShortcutToBookmark(String url, String title, Bitmap favicon, int rValue,
            int gValue, int bValue) {
        getChromeViewClient().addShortcutToBookmark(url, title, favicon, rValue, gValue, bValue);

    }

    /**
     * Called when the mobile promo action asks to send an email.
     */
    @CalledByNative
    private void promoSendEmail(String email, String subj, String body, String inv) {
        getChromeViewClient().promoSendEmail(email, subj, body, inv);
    }


    // The following methods are implemented at native side.

    // Initialize the ChromeView native side.
    // If nativeInitProcess is never called, the first time this method is
    // called, nativeInitProcess will be called implicitly with the default
    // settings.
    // |context| is the application context.
    // |incognito| if true, no state (cache, cookies...) are persisted when the
    //             view is closed.
    // |hardwareAccelerated| if true, the View uses hardware accelerated rendering.
    // |nativeWebContents| if not null, points to the native WebContents
    // associated with the ChromeView.
    private native int nativeInit(Context context, boolean incognito,
            boolean hardwareAccelerated, int nativeWebContents);

    private native String nativeGetLocalizedString(int nativeChromeView, int id);

    private native void nativeDestroy(int nativeChromeView);

    private native void nativeSetNetworkAvailable(int nativeChromeView, boolean networkUp);

    private native void nativeLoadUrlWithoutUrlSanitization(int nativeChromeView,
            String url, int pageTransition);

    private native String nativeGetURL(int nativeChromeView);

    private native String nativeGetTitle(int nativeChromeView);

    private native double nativeGetLoadProgress(int nativeChromeView);

    private native int nativeDraw(int nativeChromeView, Canvas canvas);

    private native void nativeSetSurface(Surface surface, int primaryID, int secondaryID, int nativeWaitableNativeEvent);

    private native int nativeGetSurfaceID(int rendererID, int viewID);

    private native Bitmap nativeGetFaviconBitmap(int nativeChromeView);
    private native boolean nativeFaviconIsValid(int nativeChromeView);

    // Returns true if the native side crashed so that java side can draw a sad tab.
    private native boolean nativeCrashed(int nativeChromeView);

    private native void nativeSetSize(int nativeChromeView, int w, int h);

    private native void nativeSetFocus(int nativeChromeView, boolean focused);

    // TODO(clank) Currently the native only uses int instead of float for screen coordinates,
    // so there is no point to pass the float to the native. Need to reconsider it later.

    private native int nativeTouchEvent(int nativeChromeView,
                                        int action, long timeStamp,
                                        TouchPoint[] pts);

    private native int nativeMouseMoveEvent(int nativeChromeView, long time, int x, int y);

    private native void nativeOrientationChange(int nativeChromeView, int orientation);

    private native void nativeShowPressState(int nativeChromeView, int x, int y);

    private native void nativeSingleTap(int nativeChromeView, int x, int y,
            boolean checkMultipleTargets);

    private native void nativeDoubleTap(int nativeChromeView, int x, int y);

    private native void nativeLongPress(int nativeChromeView, int x, int y,
            boolean checkMultipleTargets);

    private native void nativeSelectBetweenCoordinates(int nativeChromeView,
            int x1, int y1, int x2, int y2);

    private native void nativeScrollBy(int nativeChromeView, int deltaX, int deltaY);

    private native int nativeSendMouseWheelEvent(int nativeChromeView, int x, int y,
            long eventTime, int direction);

    private native void nativeFlingStart(int nativeChromeView, int x, int y, int vx, int vy);

    private native void nativeFlingCancel(int nativeChromeView);

    private native void nativePinchBy(int nativeChromeView,
            float deltaScale, int anchorX, int anchorY);

    private native boolean nativeCanGoBack(int nativeChromeView);

    private native boolean nativeCanGoForward(int nativeChromeView);

    private native boolean nativeCanGoBackOrForward(int nativeChromeView, int steps);

    private native void nativeGoBackOrForward(int nativeChromeView, int steps);

    private native void nativeGoBack(int nativeChromeView);

    private native void nativeGoForward(int nativeChromeView);

    private native void nativeStopLoading(int nativeChromeView);

    private native void nativeReload(int nativeChromeView);

    private native int nativeFindAll(int nativeChromeView, String find);

    private native void nativeFindNext(int nativeChromeView, boolean forward);

    private native void nativeStartFinding(int nativeChromeView,
                                           String searchString,
                                           boolean forwardDirection,
                                           boolean caseSensitive);

    private native void nativeActivateNearestFindResult(int nativeChromeView, float x, float y);

    private native void nativeStopFinding(int nativeChromeView, int selectionAction);

    private native String nativeGetPreviousFindText(int nativeChromeView);

    private native void nativeRequestFindMatchRects(int nativeChromeView, int haveVersion);

    private native void nativeZoomToRendererRect(int nativeChromeView,
            int x, int y, int width, int height);

    private native void nativeSetClient(int nativeChromeView, ChromeViewClient client);

    // Called on first scroll.
    private native void nativeScrollBegin(int nativeChromeView, int x, int y);
    // Called when the last scroll has been sent, from either a scroll
    // event or a fling.
    private native void nativeScrollEnd(int nativeChromeView);

    private native void nativeScrollFocusedEditableNodeIntoView(int nativeChromeView);
    private native void nativeUndoScrollFocusedEditableNodeIntoView(int nativeChromeView);

    private native void nativePinchBegin(int nativeChromeView);
    private native void nativePinchEnd(int nativeChromeView);

    private native void nativeSelectPopupMenuItems(int nativeChromeView, int[] indices);

    private native void nativeAutofillPopupSelected(int nativeChromeView, int queryId,
            int listIndex, int uniqueId);
    private native void nativeAutofillPopupClosed(int nativeChromeView);

    private native boolean nativeNeedsReload(int nativeChromeView);

    private native void nativeClearHistory(int nativeChromeView);

    private static native void nativeRestoreSessionCookies();
    private static native void nativeFlushPersistentData();
    private static native void nativeDestroyIncognitoProfile();

    private native void nativeSetAllowContentAccess(int nativeChromeView, boolean allow);
    private native void nativeSetAllowFileAccess(int nativeChromeView, boolean allow);

    private native int nativeEvaluateJavaScript(String script);

    // return the pid of the current renderer process.
    private native int nativeGetCurrentRenderProcess(int nativeChromeView);

    private native int nativeGetBackgroundColor(int nativeChromeView);

    private native void nativeOnCustomMenuAction(int nativeChromeView, int action);

    private native void nativeOnShow(int nativeChromeView, boolean requestByActivity);
    private native void nativeOnHide(int nativeChromeView, boolean requestByActivity);

    private native void nativeAddJavascriptInterface(int nativeChromeView, Object object,
                                                     String name, boolean allowInheritedMethods);
    private native void nativeRemoveJavascriptInterface(int nativeChromeView, String name);

    private native int nativeGetNativeImeAdapter(int nativeChromeView);

    private native void nativeSetUseDesktopUserAgent(int nativeChromeView,
                                                     boolean enabled,
                                                     boolean reloadOnChange);
    private native boolean nativeGetUseDesktopUserAgent(int nativeChromeView);

    // TODO(dtrainor): Remove this once b/6355537 is resolved?
    private native boolean nativeGetJavaScriptEnabled(int nativeChromeView);

    private native void nativeAcknowledgeSwapBuffers(int nativeChromeView);

    private native void nativeSendVSyncEvent(int nativeChromeView,
                                             long frameBeginTimeUSec,
                                             long currentFrameIntervalUSec);

    // The following constants, classes and methods exist to support the API of
    // the legacy WebView. These will likely be deprecated in future releases.

    /**
     * The scheme for a telephone number URI.
     */
    public static final String SCHEME_TEL = "tel:";
    /**
     * The scheme for an email address URI.
     */
    public static final String SCHEME_MAILTO = "mailto:";
    /**
     * The scheme and prefix for a Google Maps URI.
     */
    public static final String SCHEME_GEO = "geo:0,0?q=";

    /**
     * Represents the result of searching the currently loaded document for a
     * clickable element, at the location of the current cursor node. An
     * instance of this class is returned by
     * {@link #getHitTestResult() getHitTestResult()}.
     */
    public static class HitTestResult {
        /**
         * The type of the clickable element is unknown.
         */
        public static final int UNKNOWN_TYPE = 0;
        /**
         * @deprecated This type is no longer used.
         */
        @Deprecated
        public static final int ANCHOR_TYPE = 1;
        /**
         * The clickable element is an HTML anchor element specifiying a
         * telephone number URI.
         */
        public static final int PHONE_TYPE = 2;
        /**
         * The clickable element is an HTML anchor element specifiying a Google
         * Maps URI.
         */
        public static final int GEO_TYPE = 3;
        /**
         * The clickable element is an HTML anchor element specifiying an email
         * address URI.
         */
        public static final int EMAIL_TYPE = 4;
        /**
         * The clickable element is an HTML image element.
         */
        public static final int IMAGE_TYPE = 5;
        /**
         * @deprecated This type is no longer used.
         */
        @Deprecated
        public static final int IMAGE_ANCHOR_TYPE = 6;
        /**
         * The clickable element is an HTML anchor element specifying a
         * non-JavaScript URI.
         */
        public static final int SRC_ANCHOR_TYPE = 7;
        /**
         * The clickable element is an HTML anchor element specifying a
         * non-JavaScript URI, with an image element as a child element.
         */
        public static final int SRC_IMAGE_ANCHOR_TYPE = 8;
        /**
         * The clickable element is an HTML text input element.
         */
        public static final int EDIT_TEXT_TYPE = 9;

        private int mType;
        private String mExtra;

        HitTestResult() {
            mType = UNKNOWN_TYPE;
        }

        private void setType(int type) {
            mType = type;
        }

        private void setExtra(String extra) {
            mExtra = extra;
        }

        /**
         * Gets the type of this HitTestResult, specified as one of the constant
         * members of this class, such as {@link #UNKNOWN_TYPE UNKNOWN_TYPE}.
         *  See {@link #getHitTestResult() getHitTestResult()} for details.
         * @return The type of this HitTestResult.
         */
        public int getType() {
            return mType;
        }

        /**
         * Gets additional information about this HitTestResult. This is
         * typically the URI associated with the element. See
         * {@link #getHitTestResult() getHitTestResult()} for details.
         * @return Additional information about this HitTestResult.
         */
        public String getExtra() {
            return mExtra;
        }
    }

    /**
     * Return the list of currently loaded plugins.
     * @return The list of currently loaded plugins.
     *
     * @deprecated This was used for Gears, which has been deprecated.
     * @hide
     */
    @Deprecated
    public static PluginList getPluginList() {
        return WebViewLegacy.getPluginList();
    }

    /**
     * @deprecated This was used for Gears, which has been deprecated.
     * @hide
     */
    @Deprecated
    public void refreshPlugins(boolean reloadOpenPages) {
        mWebViewLegacy.refreshPlugins(reloadOpenPages);
    }

    /**
     * Pause all layout, parsing, and JavaScript timers for all WebViews. This
     * applies to all WebViews in this process, not just this WebView. This can
     * be useful if the application has been paused.
     */
    public void pauseTimers() {
        mWebViewLegacy.pauseTimers();
    }

    /**
     * Resume all layout, parsing, and JavaScript timers for all WebViews. This
     * will resume dispatching all timers.
     */
    public void resumeTimers() {
        mWebViewLegacy.resumeTimers();
    }

    /**
     * Call this to pause any extra processing associated with this WebView and
     * its associated DOM, plugins, JavaScript etc. For example, if the WebView
     * is taken offscreen, this could be called to reduce unnecessary CPU or
     * network traffic. When the WebView is again "active", call onResume().
     *
     * Note that this differs from pauseTimers(), which affects all WebViews.
     */
    public void onPause() {
        mWebViewLegacy.onPause();
    }

    /**
     * Call this to resume a WebView after a previous call to onPause().
     */
    public void onResume() {
        mWebViewLegacy.onResume();
    }

    /**
     * Clears the WebView's page history in both the backwards and forwards
     * directions.
     */
    public void clearHistory() {
        if (mNativeChromeView != 0) {
            nativeClearHistory(mNativeChromeView);
        }
    }

    /**
     * Find all instances of find on the page and highlight them.
     *
     * @param searchString String to find.
     * @return The number of occurrences of the searchString that were found.
     */
    public int findAll(String searchString) {
        return mWebViewLegacy.findAll(searchString);
    }

    /**
     * Highlight and scroll to the next occurrence of the string last passed to
     * {@link ChromeView#findAll(String) findAll()}. The search wraps around the
     * page infinitely. Must be preceded by a call to
     * {@link ChromeView#findAll(String) findAll()} that has not been cancelled
     * by a subsequent call to {@link ChromeView#clearMatches() clearMatches()}.
     *
     * @see ChromeView#findAll(String)
     * @see ChromeView#clearMatches()
     * @param forward Whether to search forwards (true) or backwards (false)
     */
    public void findNext(boolean forward) {
        mWebViewLegacy.findNext(forward);
    }

    /**
     * Clear the highlighting surrounding text matches created by
     * {@link ChromeView#findAll(String) findAll()}.
     *
     * @see ChromeView#findAll(String)
     * @see ChromeView#findNext(boolean)
     */
    public void clearMatches() {
        mWebViewLegacy.clearMatches();
    }

    /**
     * Return true if the page can navigate back or forward in the page history
     * the given number of steps. Negative steps means going backwards, positive
     * steps means going forwards.
     *
     * @param steps The number of steps to check against in the page history.
     */
    public boolean canGoBackOrForward(int steps) {
        return mWebViewLegacy.canGoBackOrForward(steps);
    }

    /**
     * Go to the history item that is the number of steps away from the current
     * item. Negative steps means going backwards, positive steps means going
     * forwards. If steps is beyond available history, nothing happens.
     *
     * @param steps The number of steps to take back or forward in the history.
     */
    public void goBackOrForward(int steps) {
        mWebViewLegacy.goBackOrForward(steps);
    }

    /**
     * Return the current scale of the WebView
     * @return The current scale.
     */
    public float getScale() {
        return mNativePageScaleFactor;
    }

    /**
     * Called to inform the WebView that a new child is added to a parent view.
     *
     * @deprecated This method no longer needs to be called on the WebView and
     *             is a no-op.
     */
    @Override
    @Deprecated
    public void onChildViewAdded(View parent, View child) {
        mWebViewLegacy.onChildViewAdded(parent, child);
    }

    /**
     * Called to inform the WebView that a child is removed from a parent view.
     *
     * @deprecated This method no longer needs to be called on the WebView and
     *             is a no-op.
     */
    @Override
    @Deprecated
    public void onChildViewRemoved(View p, View child) {
        mWebViewLegacy.onChildViewRemoved(p, child);
    }

    /**
     * Called to inform the WebView about the focus change in the view tree.
     *
     * @deprecated This method no longer needs to be called on the WebView and
     *             is a no-op.
     */
    @Override
    @Deprecated
    public void onGlobalFocusChanged(View oldFocus, View newFocus) {
        mWebViewLegacy.onGlobalFocusChanged(oldFocus, newFocus);
    }

    /**
     * Load the given data into the WebView using a 'data' scheme URL. Content
     * loaded in this way does not have the ability to load content from the
     * network.
     * <p>
     * If the value of the encoding parameter is 'base64', then the data must be
     * encoded as base64. Otherwise, the data must use ASCII encoding for octets
     * inside the range of safe URL characters and use the standard %xx hex
     * encoding of URLs for octets outside that range.
     *
     * @param data A String of data in the given encoding.
     * @param mimeType The MIMEType of the data, e.g. 'text/html'.
     * @param encoding The encoding of the data.
     */
    public void loadData(String data, String mimeType, String encoding) {
        mWebViewLegacy.loadData(data, mimeType, encoding);
    }

    /**
     * Get the height in pixels of the visible portion of the title bar.
     *
     * @return The height in pixels of the visible portion of the title bar.
     * @deprecated This method is now obsolete.
     */
    @Deprecated
    public int getVisibleTitleHeight() {
        return mWebViewLegacy.getVisibleTitleHeight();
    }

    // Legacy Client APIs.

    /**
     * Gets the WebViewClient
     * @return the current WebViewClient instance.
     */
    public WebViewClient getWebViewClient() {
        // TODO(mkosiba): move this to ChromeWebViewImpl.
        throw new UnsupportedOperationException("getWebViewClient is not implemented yet!");
    }

    /**
     * Set the WebViewClient that will receive various notifications and
     * requests. This will replace the current handler.
     * @param client An implementation of WebViewClient.
     */
    public void setWebViewClient(WebViewClient client) {
        // TODO(mkosiba): move this to ChromeWebViewImpl.
        throw new UnsupportedOperationException("setWebViewClient is not implemented yet!");
    }

    /**
     * Gets the chrome handler.
     * @return the current WebChromeClient instance.
     */
    public WebChromeClient getWebChromeClient() {
        // TODO(mkosiba): move this to ChromeWebViewImpl.
        throw new UnsupportedOperationException("getWebChromeClient is not implemented yet!");
    }

    /**
     * Set the chrome handler. This is an implementation of WebChromeClient for
     * use in handling JavaScript dialogs, favicons, titles, and the progress.
     * This will replace the current handler.
     * @param client An implementation of WebChromeClient.
     */
    public void setWebChromeClient(WebChromeClient client) {
        // TODO(mkosiba): move this to ChromeWebViewImpl.
        throw new UnsupportedOperationException("setWebChromeClient is not implemented yet!");
    }

    // The following methods are needed by WebViewLegacy to access native side

    boolean canGoBackOrForwardLegacy(int steps) {
        if (mNativeChromeView != 0) {
            return nativeCanGoBackOrForward(mNativeChromeView, steps);
        }
        return false;
    }

    void goBackOrForwardLegacy(int steps) {
        if (mNativeChromeView != 0) {
            nativeGoBackOrForward(mNativeChromeView, steps);
        }
    }

    int findAllLegacy(String find) {
        if (mNativeChromeView != 0) {
            return nativeFindAll(mNativeChromeView, find);
        }
        return 0;
    }

    void findNextLegacy(boolean forwards) {
        if (mNativeChromeView != 0) {
            nativeFindNext(mNativeChromeView, forwards);
        }
    }

    /**
     * If the view is ready to draw contents to the screen. In hardware mode,
     * the initialization of the surface texture may not occur until after the
     * view has been added to the layout. This method will return {@code true}
     * once the texture is actually ready.
     *
     * @hide
     */
    public boolean isReady() {
        return mTextureViewStatus == TextureViewStatus.READY;
    }

    /**
     * @return Whether or not the texture view is available or not.
     *
     * @hide
     */
    public boolean isAvailable() {
        return mTextureView != null &&
                mExternalSurfaceTextureOwner == null &&
                mTextureView.isAvailable();
    }

    /**
     * @return Whether or not the loading and rendering of the page is done.  This is equivalent to
     *         checking for isReady() and getProgress() == 100.
     */
    public boolean isLoadingAndRenderingDone() {
        return isReady() && getProgress() >= CONSIDERED_READY_LOAD_PERCENTAGE;
    }

    /**
     * @Override This is an added View JB API.
     */
    public boolean performAccessibilityAction(int action, Bundle arguments) {
        if (mAccessibilityInjector.supportsAccessibilityAction(action)) {
            return mAccessibilityInjector.performAccessibilityAction(action, arguments);
        }

        return AccessibilityInjectorAPIAdapter.View_performAccessibilityAction(
                this, action, arguments);
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        mAccessibilityInjector.onInitializeAccessibilityNodeInfo(info);
    }

    /**
     * Fills in scrolling values for AccessibilityEvents.
     * @param event Event being fired.
     */
    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(this.getClass().getName());

        // Identify where the top-left of the screen currently points to.
        event.setScrollX(mNativeScrollX);
        event.setScrollY(mNativeScrollY);

        // The maximum scroll values are determined by taking the content dimensions and
        // subtracting off the actual dimensions of the ChromeView.
        int maxScrollX = Math.max(0, mContentWidth - getWidth());
        int maxScrollY = Math.max(0, mContentHeight - getHeight());
        event.setScrollable(maxScrollX > 0 || maxScrollY > 0);

        // Setting the maximum scroll values requires API level 15 or higher.
        final int SDK_VERSION_REQUIRED_TO_SET_SCROLL = 15;
        if (Build.VERSION.SDK_INT >= SDK_VERSION_REQUIRED_TO_SET_SCROLL) {
            event.setMaxScrollX(maxScrollX);
            event.setMaxScrollY(maxScrollY);
        }
    }

    /**
     * Returns whether or not accessibility injection is being used.
     */
    public boolean isInjectingAccessibilityScript() {
        return mAccessibilityInjector.accessibilityIsAvailable();
    }

    /**
     * Enable or disable accessibility features.
     */
    public void setAccessibilityState(boolean state) {
        mAccessibilityInjector.setScriptEnabled(state);
    }

    /**
     * Stop any TTS notifications that are currently going on.
     */
    public void stopCurrentAccessibilityNotifications() {
        mAccessibilityInjector.stopCurrentNotifications();
    }
}
