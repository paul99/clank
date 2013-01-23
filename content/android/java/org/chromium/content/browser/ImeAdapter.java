// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.AlertDialog;
import android.app.DatePickerDialog;
import android.app.TimePickerDialog;
import android.app.DatePickerDialog.OnDateSetListener;
import android.app.TimePickerDialog.OnTimeSetListener;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.os.Handler;
import android.os.ResultReceiver;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.text.format.Time;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputMethodManager;
import android.widget.DatePicker;
import android.widget.TimePicker;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.DateTimePickerDialog.OnDateTimeSetListener;
import org.chromium.content.browser.MonthPickerDialog.OnMonthSetListener;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

// See more information about Focus / IME / Native Input Text at
// http://b/issue?id=3248163
// We have to adapt and plumb android IME service and chrome text input API.
// ImeAdapter provides an interface in both ways native <-> java:
// 1. InputConnectionAdapter notifies native code of text composition state and
//    dispatch key events from java -> WebKit.
// 2. Native ImeAdapter notifies java side to clear composition text.
//
// The basic flow is:
// 1. Intercept dispatchKeyEventPreIme() to record the current key event, but do
//    nothing else.
// 2. When InputConnectionAdapter gets called with composition or result text:
//    a) If a key event has been recorded in dispatchKeyEventPreIme() and we
//       receive a result text with single character, then we probably need to
//       send the result text as a Char event rather than a ConfirmComposition
//       event. So we need to dispatch the recorded key event followed by a
//       synthetic Char event.
//    b) If we receive a composition text or a result text with more than one
//       characters, then no matter if we recorded a key event or not in
//       dispatchKeyEventPreIme(), we just need to dispatch a synthetic key
//       event with special keycode 229, and then dispatch the composition or
//       result text.
// 3. Intercept dispatchKeyEvent() method for key events not handled by IME, we
//    need to dispatch them to webkit and check webkit's reply. Then inject a
//    new key event for further processing if webkit didn't handle it.
class ImeAdapter {
    interface ViewEmbedder {
        /**
         * @param isFinish whether the event is occuring because input is finished.
         */
        public void onImeEvent(boolean isFinish);
        public void onSetFieldValue();
        public void onDismissInput();
        public View getAttachedView();
        public ResultReceiver getNewShowKeyboardReceiver();
    }

    static final int COMPOSITION_KEY_CODE = 229;

    // Default values used in Time representations of selected date/time before formatting.
    // They are never displayed to the user.
    static final int YEAR_DEFAULT = 1970;
    static final int MONTH_DEFAULT = 0;
    static final int MONTHDAY_DEFAULT = 1;
    static final int HOUR_DEFAULT = 0;
    static final int MINUTE_DEFAULT = 0;

    static int sEventTypeRawKeyDown;
    static int sEventTypeKeyUp;
    static int sEventTypeChar;
    static int sTextInputTypeNone;
    static int sTextInputTypeText;
    static int sTextInputTypeTextArea;
    static int sTextInputTypePassword;
    static int sTextInputTypeSearch;
    static int sTextInputTypeUrl;
    static int sTextInputTypeEmail;
    static int sTextInputTypeTel;
    static int sTextInputTypeNumber;
    static int sTextInputTypeDate;
    static int sTextInputTypeDateTime;
    static int sTextInputTypeDateTimeLocal;
    static int sTextInputTypeMonth;
    static int sTextInputTypeTime;
    static int sTextInputTypeWeek;
    static int sTextInputTypeContentEditable;
    static int sModifierShift;
    static int sModifierAlt;
    static int sModifierCtrl;
    static int sModifierCapsLockOn;
    static int sModifierNumLockOn;

    @CalledByNative
    static void initializeWebInputEvents(int eventTypeRawKeyDown, int eventTypeKeyUp,
            int eventTypeChar, int modifierShift, int modifierAlt, int modifierCtrl,
            int modifierCapsLockOn, int modifierNumLockOn) {
        sEventTypeRawKeyDown = eventTypeRawKeyDown;
        sEventTypeKeyUp = eventTypeKeyUp;
        sEventTypeChar = eventTypeChar;
        sModifierShift = modifierShift;
        sModifierAlt = modifierAlt;
        sModifierCtrl = modifierCtrl;
        sModifierCapsLockOn = modifierCapsLockOn;
        sModifierNumLockOn = modifierNumLockOn;
    }

    @CalledByNative
    static void initializeTextInputTypes(int textInputTypeNone, int textInputTypeText,
            int textInputTypeTextArea, int textInputTypePassword, int textInputTypeSearch,
            int textInputTypeUrl, int textInputTypeEmail, int textInputTypeTel,
            int textInputTypeNumber, int textInputTypeDate, int textInputTypeDateTime,
            int textInputTypeDateTimeLocal, int textInputTypeMonth, int textInputTypeTime,
            int textInputTypeWeek, int textInputTypeContentEditable) {
        sTextInputTypeNone = textInputTypeNone;
        sTextInputTypeText = textInputTypeText;
        sTextInputTypeTextArea = textInputTypeTextArea;
        sTextInputTypePassword = textInputTypePassword;
        sTextInputTypeSearch = textInputTypeSearch;
        sTextInputTypeUrl = textInputTypeUrl;
        sTextInputTypeEmail = textInputTypeEmail;
        sTextInputTypeTel = textInputTypeTel;
        sTextInputTypeNumber = textInputTypeNumber;
        sTextInputTypeDate = textInputTypeDate;
        sTextInputTypeDateTime = textInputTypeDateTime;
        sTextInputTypeDateTimeLocal = textInputTypeDateTimeLocal;
        sTextInputTypeMonth = textInputTypeMonth;
        sTextInputTypeTime = textInputTypeTime;
        sTextInputTypeWeek = textInputTypeWeek;
        sTextInputTypeContentEditable = textInputTypeContentEditable;
    }

    private int mNativeImeAdapterAndroid;
    private int mTextInputType;
    private int mPreImeEventCount;
    private boolean mDialogCanceled;

    private Context mContext;
    private SelectionHandleController mSelectionHandleController;
    private InsertionHandleController mInsertionHandleController;
    private AdapterInputConnection mInputConnection;
    private ViewEmbedder mViewEmbedder;
    private AlertDialog mDialog;
    private Handler mHandler;
    private final Editable mEditable;
    private long mLastActionTime;

    private class DelayedDismissInput implements Runnable {
        private int mNativeImeAdapter;

        DelayedDismissInput(int nativeImeAdapter) {
            mNativeImeAdapter = nativeImeAdapter;
        }

        @Override
        public void run() {
            attach(mNativeImeAdapter, sTextInputTypeNone);
            dismissInput();
        }
    };

    private DelayedDismissInput mDismissInput = null;

    // Delay introduced to avoid hiding the keyboard if new show requests are received.
    // The time required by the unfocus-focus events triggered by tab has been measured in soju:
    // Mean: 18.633 ms, Standard deviation: 7.9837 ms.
    // The value here should be higher enough to cover these cases, but not too high to avoid
    // letting the user perceiving important delays.
    private static final int INPUT_DISMISS_DELAY = 150;

    // Date formats as accepted by Time.format.
    private static final String HTML_DATE_FORMAT = "%Y-%m-%d";
    private static final String HTML_TIME_FORMAT = "%H:%M";
    // For datetime we always send selected time as UTC, as we have no timezone selector.
    // This is consistent with other browsers.
    private static final String HTML_DATE_TIME_FORMAT = "%Y-%m-%dT%H:%MZ";
    private static final String HTML_DATE_TIME_LOCAL_FORMAT = "%Y-%m-%dT%H:%M";
    private static final String HTML_MONTH_FORMAT = "%Y-%m";

    // Date formats as accepted by SimpleDateFormat.
    private static final String PARSE_DATE_FORMAT = "yyyy-MM-dd";
    private static final String PARSE_TIME_FORMAT = "HH:mm";
    private static final String PARSE_DATE_TIME_FORMAT = "yyyy-MM-dd'T'HH:mm'Z'";
    private static final String PARSE_DATE_TIME_LOCAL_FORMAT = "yyyy-MM-dd'T'HH:mm";
    private static final String PARSE_MONTH_FORMAT = "yyyy-MM";

    ImeAdapter(Context context, SelectionHandleController selectionHandleController,
            InsertionHandleController insertionHandleController, ViewEmbedder embedder) {
        mPreImeEventCount = 0;
        mContext = context;
        mSelectionHandleController = selectionHandleController;
        mInsertionHandleController = insertionHandleController;
        mViewEmbedder = embedder;
        mHandler = new Handler();
        mEditable = Editable.Factory.getInstance().newEditable("");
        Selection.setSelection(mEditable, 0);
    }

    boolean isFor(int nativeImeAdapter, int textInputType) {
        return mNativeImeAdapterAndroid == nativeImeAdapter &&
               mTextInputType == textInputType;
    }

    void attachAndShowIfNeeded(int nativeImeAdapter, int textInputType,
            String text, boolean showIfNeeded) {
        mHandler.removeCallbacks(mDismissInput);

        // If current input type is none and showIfNeeded is false, IME should not be shown
        // and input type should remain as none.
        if (mTextInputType == sTextInputTypeNone && !showIfNeeded) {
            return;
        }

        if (!isFor(nativeImeAdapter, textInputType)) {
            // Set a delayed task to perform unfocus. This avoids hiding the keyboard when tabbing
            // through text inputs or when JS rapidly changes focus to another text element.
            if (textInputType == sTextInputTypeNone) {
                mDismissInput = new DelayedDismissInput(nativeImeAdapter);
                mHandler.postDelayed(mDismissInput, INPUT_DISMISS_DELAY);
                return;
            }

            int previousType = mTextInputType;
            attach(nativeImeAdapter, textInputType);

            InputMethodManager manager = (InputMethodManager)
                    mContext.getSystemService(Context.INPUT_METHOD_SERVICE);

            if (hasTextInputType()) {
                manager.restartInput(mViewEmbedder.getAttachedView());
                // If type has changed from dialog to text, show even if showIfNeeded is not true.
                if (showIfNeeded || isDialogShowing()) {
                    showKeyboard();
                }
            } else if (hasDialogInputType()) {
                // If type has changed from text to dialog, show even if showIfNeeded is not true.
                if (showIfNeeded || isTextInputType(previousType)) {
                    showDialog(text);
                }
            }
        } else if (hasInputType()) {
            if (!isDialogShowing() && showIfNeeded) {
                if (hasDialogInputType()) {
                    showDialog(text);
                } else {
                    showKeyboard();
                }
            }
        }
    }

    void attach(int nativeImeAdapter, int textInputType) {
        mNativeImeAdapterAndroid = nativeImeAdapter;
        mTextInputType = textInputType;
        nativeAttachImeAdapter(mNativeImeAdapterAndroid);
    }

    /**
     * Attaches the imeAdapter to its native counterpart. This is needed to start forwarding
     * keyboard events to WebKit.
     * @param nativeImeAdapter The pointer to the native ImeAdapter object.
     */
    void attach(int nativeImeAdapter) {
        mNativeImeAdapterAndroid = nativeImeAdapter;
        if (nativeImeAdapter != 0) {
            nativeAttachImeAdapter(mNativeImeAdapterAndroid);
        }
    }

    /**
     * Used to check whether the native counterpart of the ImeAdapter has been attached yet.
     * @return Whether native ImeAdapter has been attached and its pointer is currently nonzero.
     */
    boolean isNativeImeAdapterAttached() {
        return mNativeImeAdapterAndroid != 0;
    }

    private void showKeyboard() {
        if (isDialogShowing()) mDialog.dismiss();
        InputMethodManager manager = (InputMethodManager)
                mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        manager.showSoftInput(mViewEmbedder.getAttachedView(), 0,
                mViewEmbedder.getNewShowKeyboardReceiver());
    }

    private void showDialog(String text) {
        mInsertionHandleController.hideAndDisallowAutomaticShowing();
        if (isDialogShowing()) mDialog.dismiss();

        Time time = parse(text);
        if (mTextInputType == sTextInputTypeDate) {
            mDialog = new DatePickerDialog(mContext, new DateListener(),
                    time.year, time.month, time.monthDay);
        } else if (mTextInputType == sTextInputTypeTime) {
            mDialog = new TimePickerDialog(mContext, new TimeListener(),
                    time.hour, time.minute, DateFormat.is24HourFormat(mContext));
        } else if (mTextInputType == sTextInputTypeDateTime ||
                mTextInputType == sTextInputTypeDateTimeLocal) {
            mDialog = new DateTimePickerDialog(mContext,
                    new DateTimeListener(mTextInputType == sTextInputTypeDateTimeLocal),
                    time.year, time.month, time.monthDay,
                    time.hour, time.minute, DateFormat.is24HourFormat(mContext));
        } else if (mTextInputType == sTextInputTypeMonth) {
            mDialog = new MonthPickerDialog(mContext, new MonthListener(),
                    time.year, time.month);
        }

        mDialog.setButton(DialogInterface.BUTTON_POSITIVE,
                mContext.getText(ChromeView.getResourceId("R.string.date_picker_dialog_set")),
                (DialogInterface.OnClickListener) mDialog);

        mDialog.setButton(DialogInterface.BUTTON_NEGATIVE,
                mContext.getText(android.R.string.cancel),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        mDialogCanceled = true;
                    }
                });

        mDialog.setButton(DialogInterface.BUTTON_NEUTRAL,
                mContext.getText(ChromeView.getResourceId("R.string.date_picker_dialog_clear")),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        mDialogCanceled = true;
                        nativeReplaceText(mNativeImeAdapterAndroid, "");
                    }
                });

        mDialog.setOnDismissListener(new OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        nativeClearFocus(mNativeImeAdapterAndroid);
                    }
                });
        mDialogCanceled = false;
        mDialog.show();
    }

    private SimpleDateFormat getParseDateFormat() {
        String formatString = null;
        if (mTextInputType == sTextInputTypeDate) {
            formatString = PARSE_DATE_FORMAT;
        } else if (mTextInputType == sTextInputTypeTime) {
            formatString = PARSE_TIME_FORMAT;
        } else if (mTextInputType == sTextInputTypeDateTime) {
            formatString = PARSE_DATE_TIME_FORMAT;
        } else if (mTextInputType == sTextInputTypeDateTimeLocal) {
            formatString = PARSE_DATE_TIME_LOCAL_FORMAT;
        } else if (mTextInputType == sTextInputTypeMonth) {
            formatString = PARSE_MONTH_FORMAT;
        }

        if (formatString != null) {
            return new SimpleDateFormat(formatString);
        }

        return null;
    }

    /**
     * Parse the text String according to the current text input type.
     * @param text
     */
    private Time parse(String text) {
        Time result = null;
        if (!TextUtils.isEmpty(text)) {
            try {
                Date date = getParseDateFormat().parse(text);
                result = new Time();
                result.set(date.getTime());
            } catch (ParseException e) {
                // Leave time as null.
            }
        }

        if (result == null) {
            result = new Time();
            result.setToNow();
        }

        return result;
    }

    private void dismissInput() {
        hideKeyboard(true);
        mViewEmbedder.onDismissInput();
    }

    private void hideKeyboard(boolean unzoomIfNeeded) {
        InputMethodManager manager = (InputMethodManager)
                mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        View view = mViewEmbedder.getAttachedView();
        if (manager.isActive(view)) {
            manager.hideSoftInputFromWindow(view.getWindowToken(), 0,
                    unzoomIfNeeded ? mViewEmbedder.getNewShowKeyboardReceiver() : null);
        }
    }

    private boolean isDialogShowing() {
        return mDialog != null && mDialog.isShowing();
    }

    private class DateListener implements OnDateSetListener {
        @Override
        public void onDateSet(DatePicker view, int year, int month, int monthDay) {
            if (!mDialogCanceled) {
                setFieldDateTimeValue(year, month, monthDay, HOUR_DEFAULT, MINUTE_DEFAULT,
                        HTML_DATE_FORMAT);
            }
        }
    }

    private class TimeListener implements OnTimeSetListener {
        @Override
        public void onTimeSet(TimePicker view, int hourOfDay, int minute) {
            if (!mDialogCanceled) {
                setFieldDateTimeValue(YEAR_DEFAULT, MONTH_DEFAULT, MONTHDAY_DEFAULT,
                        hourOfDay, minute, HTML_TIME_FORMAT);
            }
        }
    }

    private class DateTimeListener implements OnDateTimeSetListener {
        private boolean mLocal;

        public DateTimeListener(boolean local) {
            mLocal = local;
        }

        @Override
        public void onDateTimeSet(DatePicker dateView, TimePicker timeView,
                int year, int month, int monthDay,
                int hourOfDay, int minute) {
            if (!mDialogCanceled) {
                setFieldDateTimeValue(year, month, monthDay, hourOfDay, minute,
                        mLocal ? HTML_DATE_TIME_LOCAL_FORMAT : HTML_DATE_TIME_FORMAT);
            }
        }
    }

    private class MonthListener implements OnMonthSetListener {
        @Override
        public void onMonthSet(MonthPicker view, int year, int month) {
            if (!mDialogCanceled) {
                setFieldDateTimeValue(year, month, MONTHDAY_DEFAULT,
                        HOUR_DEFAULT, MINUTE_DEFAULT, HTML_MONTH_FORMAT);
            }
        }
    }

    private void setFieldDateTimeValue(int year, int month, int monthDay, int hourOfDay,
            int minute, String dateFormat) {
        Time time = new Time();
        time.year = year;
        time.month = month;
        time.monthDay = monthDay;
        time.hour = hourOfDay;
        time.minute = minute;
        nativeReplaceText(mNativeImeAdapterAndroid, time.format(dateFormat));
        mViewEmbedder.onSetFieldValue();
    }

    @CalledByNative
    void detach() {
        mNativeImeAdapterAndroid = 0;
        mTextInputType = 0;
    }

    boolean hasInputType() {
        return mTextInputType != sTextInputTypeNone;
    }

    static boolean isTextInputType(int type) {
        return type != sTextInputTypeNone && !isDialogInputType(type);
    }

    static boolean isDialogInputType(int type) {
        return type == sTextInputTypeDate || type == sTextInputTypeTime
                || type == sTextInputTypeDateTime || type == sTextInputTypeDateTimeLocal
                || type == sTextInputTypeMonth;
    }

    boolean hasTextInputType() {
        return isTextInputType(mTextInputType);
    }

    boolean hasDialogInputType() {
        return isDialogInputType(mTextInputType);
    }

    void dispatchKeyEventPreIme(KeyEvent event) {
        // We only register that a key was pressed, but we don't actually intercept
        // it.
        ++mPreImeEventCount;
    }

    boolean dispatchKeyEvent(KeyEvent event) {
        mPreImeEventCount = 0;
        return translateAndSendNativeEvents(event);
    }

    void commitText() {
        cancelComposition();
        if (mNativeImeAdapterAndroid != 0) {
            nativeCommitText(mNativeImeAdapterAndroid, "");
        }
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void cancelComposition() {
        getInputMethodManager().restartInput(mViewEmbedder.getAttachedView());
    }

    private boolean checkCompositionQueueAndCallNative(String text, int newCursorPosition,
            boolean isCommit) {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }

        // Committing an empty string finishes the current composition.
        boolean isFinish = text.isEmpty();
        if (!isFinish) {
            mSelectionHandleController.hideAndDisallowAutomaticShowing();
            mInsertionHandleController.hideAndDisallowAutomaticShowing();
        }
        mViewEmbedder.onImeEvent(isFinish);
        boolean hasSingleChar = mPreImeEventCount == 1 && text.length() == 1;
        int keyCode = hasSingleChar ? text.codePointAt(0) : COMPOSITION_KEY_CODE;
        int keyChar = hasSingleChar ? text.codePointAt(0) : 0;
        long timeStampSeconds = System.currentTimeMillis() / 1000;
        nativeSendSyntheticKeyEvent(mNativeImeAdapterAndroid, sEventTypeRawKeyDown,
                timeStampSeconds, keyCode,  keyChar);
        if (hasSingleChar) {
            nativeSendSyntheticKeyEvent(mNativeImeAdapterAndroid, sEventTypeChar,
                    timeStampSeconds, text.codePointAt(0), text.codePointAt(0));
        } else {
            if (isCommit) {
                nativeCommitText(mNativeImeAdapterAndroid, text);
            } else {
                nativeSetComposingText(mNativeImeAdapterAndroid, text, newCursorPosition);
            }
        }
        nativeSendSyntheticKeyEvent(mNativeImeAdapterAndroid, sEventTypeKeyUp,
                timeStampSeconds, keyCode,  keyChar);
        mPreImeEventCount = 0;
        return true;
    }

    private boolean translateAndSendNativeEvents(KeyEvent event) {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        int action = event.getAction();
        if (action != KeyEvent.ACTION_DOWN &&
            action != KeyEvent.ACTION_UP) {
            // action == KeyEvent.ACTION_MULTIPLE
            // TODO(bulach): confirm the actual behavior. Apparently:
            // If event.getKeyCode() == KEYCODE_UNKNOWN, we can send a
            // composition key down (229) followed by a commit text with the
            // string from event.getUnicodeChars().
            // Otherwise, we'd need to send an event with a
            // WebInputEvent::IsAutoRepeat modifier. We also need to verify when
            // we receive ACTION_MULTIPLE: we may receive it after an ACTION_DOWN,
            // and if that's the case, we'll need to review when to send the Char
            // event.
            return false;
        }
        mViewEmbedder.onImeEvent(false);
        return nativeSendKeyEvent(mNativeImeAdapterAndroid, event, event.getAction(),
                getModifiers(event.getMetaState()), event.getEventTime(),
                        KeycodeConversionHelper.convertAndroidKeyCodeToWebKit(event.getKeyCode()),
                                event.isSystem(), event.getUnicodeChar());
    }

    private void setInputConnection(AdapterInputConnection inputConnection) {
        mInputConnection = inputConnection;
    }

    private static int getModifiers(int metaState) {
        int modifiers = 0;
        if ((metaState & KeyEvent.META_SHIFT_ON) != 0) {
          modifiers |= sModifierShift;
        }
        if ((metaState & KeyEvent.META_ALT_ON) != 0) {
          modifiers |= sModifierAlt;
        }
        if ((metaState & KeyEvent.META_CTRL_ON) != 0) {
          modifiers |= sModifierCtrl;
        }
        if ((metaState & KeyEvent.META_CAPS_LOCK_ON) != 0) {
          modifiers |= sModifierCapsLockOn;
        }
        if ((metaState & KeyEvent.META_NUM_LOCK_ON) != 0) {
          modifiers |= sModifierNumLockOn;
        }
        return modifiers;
    }

    boolean isActive() {
        return getInputMethodManager().isActive();
    }

    private boolean sendSyntheticKeyEvent(
            int eventType, long timestampSeconds, int keyCode, int unicodeChar) {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeSendSyntheticKeyEvent(
                mNativeImeAdapterAndroid, eventType, timestampSeconds, keyCode, unicodeChar);
        return true;
    }

    private boolean deleteSurroundingText(int leftLength, int rightLength) {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeDeleteSurroundingText(mNativeImeAdapterAndroid, leftLength, rightLength);
        return true;
    }

    private boolean setEditableSelectionOffsets(int start, int end) {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeSetEditableSelectionOffsets(mNativeImeAdapterAndroid, start, end);
        return true;
    }

    private boolean setComposingRegion(int start, int end) {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeSetComposingRegion(mNativeImeAdapterAndroid, start, end);
        return true;
    }

    boolean unselect() {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeUnselect(mNativeImeAdapterAndroid);
        return true;
    }

    boolean selectAll() {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeSelectAll(mNativeImeAdapterAndroid);
        return true;
    }

    boolean cut() {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeCut(mNativeImeAdapterAndroid);
        return true;
    }

    boolean copy() {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeCopy(mNativeImeAdapterAndroid);
        return true;
    }

    boolean paste() {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativePaste(mNativeImeAdapterAndroid);
        return true;
    }

    boolean requestTextInputStateUpdate() {
        if (mNativeImeAdapterAndroid == 0) {
            return false;
        }
        nativeRequestTextInputStateUpdate(mNativeImeAdapterAndroid,
            System.currentTimeMillis());
        return true;
    }

    /**
     * Updates the ImeAdapter's internal representation of the text
     * being edited and its selection and composition properties. The resulting
     * Editable is accessible through the getEditable() method of AdapterInputConnection.
     * If the text has not changed, this also calls updateSelection on the InputMethodManager.
     * @param text The String contents of the field being edited
     * @param selectionStart The character offset of the selection start, or the caret
     * position if there is no selection
     * @param selectionEnd The character offset of the selection end, or the caret
     * position if there is no selection
     * @param compositionStart The character offset of the composition start, or -1
     * if there is no composition
     * @param compositionEnd The character offset of the composition end, or -1
     * if there is no selection
     */
    void setEditableText(String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd, long requestTime) {

        // If this update was a result of a request (i.e. a call to requestTextInputStateUpdate())
        // that originated before the last action that could have changed the state, ignore
        // this update.
        if (requestTime != 0 && requestTime < mLastActionTime) {
            return;
        }

        int prevSelectionStart = Selection.getSelectionStart(mEditable);
        int prevSelectionEnd = Selection.getSelectionEnd(mEditable);
        int prevEditableLength = mEditable.length();
        int prevCompositionStart = BaseInputConnection.getComposingSpanStart(mEditable);
        int prevCompositionEnd = BaseInputConnection.getComposingSpanEnd(mEditable);
        String prevText = mEditable.toString();

        selectionStart = Math.min(selectionStart, text.length());
        selectionEnd = Math.min(selectionEnd, text.length());
        compositionStart = Math.min(compositionStart, text.length());
        compositionEnd = Math.min(compositionEnd, text.length());

        boolean textUnchanged = prevText.equals(text);

        if (textUnchanged
                && prevSelectionStart == selectionStart && prevSelectionEnd == selectionEnd
                && prevCompositionStart == compositionStart
                && prevCompositionEnd == compositionEnd) {
            // Nothing has changed; don't need to do anything
            return;
        }

        // When a programmatic change has been made to the editable field, both the start
        // and end positions for the composition will equal zero. In this case we cancel the
        // active composition in the editor as this no longer is relevant.
        if (textUnchanged && compositionStart == 0 && compositionEnd == 0) {
            cancelComposition();
        }

        if (!textUnchanged) {
            mEditable.replace(0, mEditable.length(), text);
        }
        Selection.setSelection(mEditable, selectionStart, selectionEnd);

        if (mInputConnection != null) {
            mInputConnection.setInternalComposingRegion(compositionStart, compositionEnd);
        }

        if (textUnchanged || prevText.equals("")) {
            // updateSelection should be called when a manual selection change occurs.
            // Should not be called if text is being entered else issues can occur
            // e.g. backspace to undo autocorrection will not work with the default OSK.
            getInputMethodManager().updateSelection(mViewEmbedder.getAttachedView(),
                    selectionStart, selectionEnd, compositionStart, compositionEnd);
        }
    }

    private Editable getEditable() {
        return mEditable;
    }

    private InputMethodManager getInputMethodManager() {
        return (InputMethodManager) mViewEmbedder.getAttachedView().getContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
    }

    private void updateLastActionTime() {
        mLastActionTime = System.currentTimeMillis();
    }

    // This InputConnection is created by ChromeView.onCreateInputConnection.
    // It then adapts android's IME to chrome's RenderWidgetHostView using the
    // native ImeAdapterAndroid via the outer class ImeAdapter.
    static public class AdapterInputConnection extends BaseInputConnection {
        private View mInternalView;
        private ImeAdapter mImeAdapter;
        private boolean mSingleLine;
        private int mNumBatchEdits;
        private boolean mShouldUpdateImeSelection;

        // Factory function.
        static public AdapterInputConnection getInstance(View view, ImeAdapter imeAdapter,
                EditorInfo outAttrs) {
            return new AdapterInputConnection(view, imeAdapter, outAttrs);
        }

        @Override
        public Editable getEditable() {
            return mImeAdapter.getEditable();
        }

        @Override
        public boolean setComposingText(CharSequence text, int newCursorPosition) {
            mImeAdapter.updateLastActionTime();
            if (!mImeAdapter.checkCompositionQueueAndCallNative(text.toString(),
                    newCursorPosition, false)) {
                return false;
            }
            super.setComposingText(text, newCursorPosition);
            mShouldUpdateImeSelection = true;
            return true;
        }

        @Override
        public boolean commitText(CharSequence text, int newCursorPosition) {
            mImeAdapter.updateLastActionTime();
            if (!mImeAdapter.checkCompositionQueueAndCallNative(text.toString(),
                    newCursorPosition, text.length() > 0)) {
                return false;
            }
            super.commitText(text, newCursorPosition);
            mShouldUpdateImeSelection = true;
            return true;
        }

        @Override
        public boolean performEditorAction(int actionCode) {
            mImeAdapter.updateLastActionTime();
            switch (actionCode) {
                case EditorInfo.IME_ACTION_NEXT:
                    mImeAdapter.cancelComposition();
                    // Send TAB key event
                    long timeStampSeconds = System.currentTimeMillis() / 1000;
                    mImeAdapter.sendSyntheticKeyEvent(
                            sEventTypeRawKeyDown, timeStampSeconds, KeyEvent.KEYCODE_TAB, 0);
                    return true;
                case EditorInfo.IME_ACTION_GO:
                case EditorInfo.IME_ACTION_SEARCH:
                    mImeAdapter.dismissInput();
                    break;
            }

            return super.performEditorAction(actionCode);
        }

        @Override
        public boolean performContextMenuAction(int id) {
            mImeAdapter.updateLastActionTime();
            switch (id) {
                case android.R.id.selectAll:
                    return mImeAdapter.selectAll();
                case android.R.id.cut:
                    return mImeAdapter.cut();
                case android.R.id.copy:
                    return mImeAdapter.copy();
                case android.R.id.paste:
                    return mImeAdapter.paste();
                default:
                    return false;
            }
        }

        @Override
        public ExtractedText getExtractedText(ExtractedTextRequest request, int flags) {
            ExtractedText et = new ExtractedText();
            Editable editable = getEditable();
            if (editable == null) {
                et.text = "";
            } else {
                et.text = editable.toString();
                et.partialEndOffset = editable.length();
                et.selectionStart = Selection.getSelectionStart(editable);
                et.selectionEnd = Selection.getSelectionEnd(editable);
            }
            et.flags = mSingleLine ? ExtractedText.FLAG_SINGLE_LINE : 0;
            return et;
        }

        @Override
        public boolean deleteSurroundingText(int leftLength, int rightLength) {
            if (getEditable() == null) return false;
            mImeAdapter.updateLastActionTime();
            if (!mImeAdapter.deleteSurroundingText(leftLength, rightLength)) return false;
            super.deleteSurroundingText(leftLength, rightLength);
            mShouldUpdateImeSelection = true;
            return true;
        }

        @Override
        public boolean sendKeyEvent(KeyEvent event) {
            mImeAdapter.updateLastActionTime();
            mImeAdapter.mSelectionHandleController.hideAndDisallowAutomaticShowing();
            mImeAdapter.mInsertionHandleController.hideAndDisallowAutomaticShowing();

            // If this is a key-up, and backspace/del or if the key has a character representation,
            // need to update the underlying Editable (i.e. the local representation of the text
            // being edited).
            if (event.getAction() == KeyEvent.ACTION_UP) {
                if (event.getKeyCode() == KeyEvent.KEYCODE_DEL) {
                    super.deleteSurroundingText(1, 0);
                } else if (event.getKeyCode() == KeyEvent.KEYCODE_FORWARD_DEL) {
                    super.deleteSurroundingText(0, 1);
                } else {
                    int unicodeChar = event.getUnicodeChar();
                    if (unicodeChar != 0) {
                        Editable editable = getEditable();
                        int selectionStart = Selection.getSelectionStart(editable);
                        int selectionEnd = Selection.getSelectionEnd(editable);
                        if (selectionStart > selectionEnd) {
                            int temp = selectionStart;
                            selectionStart = selectionEnd;
                            selectionEnd = temp;
                        }
                        editable.replace(selectionStart, selectionEnd,
                                Character.toString((char)unicodeChar));
                    }
                }
            }
            mShouldUpdateImeSelection = true;
            return super.sendKeyEvent(event);
        }

        @Override
        public boolean finishComposingText() {
            mImeAdapter.updateLastActionTime();
            Editable editable = getEditable();
            if (editable == null
                    || (getComposingSpanStart(editable) == getComposingSpanEnd(editable))) {
                return true;
            }
            if (!mImeAdapter.checkCompositionQueueAndCallNative("", 0, true)) return false;
            super.finishComposingText();
            return true;
        }

        @Override
        public boolean setSelection(int start, int end) {
            mImeAdapter.updateLastActionTime();
            if (start < 0 || end < 0) return true;
            if (!mImeAdapter.setEditableSelectionOffsets(start, end)) return false;
            super.setSelection(start, end);
            mShouldUpdateImeSelection = true;
            return true;
        }

        @Override
        public boolean setComposingRegion(int start, int end) {
            mImeAdapter.updateLastActionTime();
            int a = Math.min(start, end);
            int b = Math.max(start, end);
            if (!mImeAdapter.setComposingRegion(a, b)) return false;
            super.setComposingRegion(a, b);
            return true;
        }

        private void updateImeSelection() {
            Editable editable = getEditable();
            if (editable != null) {
                mImeAdapter.getInputMethodManager().updateSelection(mInternalView,
                        Selection.getSelectionStart(editable),
                        Selection.getSelectionEnd(editable),
                        getComposingSpanStart(editable),
                        getComposingSpanEnd(editable));
            }
        }

        @Override
        public boolean beginBatchEdit() {
            mImeAdapter.updateLastActionTime();
            ++mNumBatchEdits;
            return false;
        }

        @Override
        public boolean endBatchEdit() {
            if (--mNumBatchEdits == 0) {
                if (mShouldUpdateImeSelection) {
                    updateImeSelection();
                    mShouldUpdateImeSelection = false;
                }
                mImeAdapter.requestTextInputStateUpdate();
            }
            return false;
        }

        void setInternalComposingRegion(int start, int end) {
            super.setComposingRegion(start, end);
        }

        private AdapterInputConnection(View view, ImeAdapter imeAdapter, EditorInfo outAttrs) {
            super(view, true);
            mInternalView = view;
            mImeAdapter = imeAdapter;
            mImeAdapter.setInputConnection(this);
            mSingleLine = true;
            outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN;
            outAttrs.inputType = EditorInfo.TYPE_CLASS_TEXT
                    | EditorInfo.TYPE_TEXT_VARIATION_WEB_EDIT_TEXT;
            if (imeAdapter.mTextInputType == ImeAdapter.sTextInputTypeText) {
                // Normal text field
                outAttrs.imeOptions |= EditorInfo.IME_ACTION_GO;
            } else if (imeAdapter.mTextInputType == ImeAdapter.sTextInputTypeTextArea ||
                    imeAdapter.mTextInputType == ImeAdapter.sTextInputTypeContentEditable) {
                // TextArea or contenteditable.
                outAttrs.inputType |= EditorInfo.TYPE_TEXT_FLAG_MULTI_LINE
                        | EditorInfo.TYPE_TEXT_FLAG_CAP_SENTENCES
                        | EditorInfo.TYPE_TEXT_FLAG_AUTO_CORRECT;
                outAttrs.imeOptions |= EditorInfo.IME_ACTION_NONE;
                mSingleLine = false;
            } else if (imeAdapter.mTextInputType == ImeAdapter.sTextInputTypePassword) {
                // Password
                outAttrs.inputType = InputType.TYPE_CLASS_TEXT
                        | InputType.TYPE_TEXT_VARIATION_WEB_PASSWORD;
                outAttrs.imeOptions |= EditorInfo.IME_ACTION_GO;
            } else if (imeAdapter.mTextInputType == ImeAdapter.sTextInputTypeSearch) {
                // Search
                outAttrs.imeOptions |= EditorInfo.IME_ACTION_SEARCH;
            } else if (imeAdapter.mTextInputType == ImeAdapter.sTextInputTypeUrl) {
                // Url
                // TYPE_TEXT_VARIATION_URI prevents Tab key from showing, so
                // exclude it for now.
                outAttrs.imeOptions |= EditorInfo.IME_ACTION_GO;
            } else if (imeAdapter.mTextInputType == ImeAdapter.sTextInputTypeEmail) {
                // Email
                outAttrs.inputType = InputType.TYPE_CLASS_TEXT
                        | InputType.TYPE_TEXT_VARIATION_WEB_EMAIL_ADDRESS;
                outAttrs.imeOptions |= EditorInfo.IME_ACTION_GO;
            } else if (imeAdapter.mTextInputType == ImeAdapter.sTextInputTypeTel) {
                // Telephone
                // Number and telephone do not have both a Tab key and an
                // action in default OSK, so set the action to NEXT
                outAttrs.inputType = InputType.TYPE_CLASS_PHONE;
                outAttrs.imeOptions |= EditorInfo.IME_ACTION_NEXT;
            } else if (imeAdapter.mTextInputType == ImeAdapter.sTextInputTypeNumber) {
                // Number
                outAttrs.inputType = InputType.TYPE_CLASS_NUMBER
                        | InputType.TYPE_NUMBER_VARIATION_NORMAL;
                outAttrs.imeOptions |= EditorInfo.IME_ACTION_NEXT;
            }
        }
    }

    private native boolean nativeSendSyntheticKeyEvent(int nativeImeAdapterAndroid,
            int eventType, long timestampSeconds, int keyCode, int unicodeChar);

    private native boolean nativeSendKeyEvent(int nativeImeAdapterAndroid, KeyEvent event,
            int action, int modifiers, long eventTime, int keyCode, boolean isSystemKey,
            int unicodeChar);

    private native void nativeSetComposingText(int nativeImeAdapterAndroid, String text,
        int newCursorPosition);

    private native void nativeCommitText(int nativeImeAdapterAndroid, String text);

    private native void nativeAttachImeAdapter(int nativeImeAdapterAndroid);

    private native void nativeReplaceText(int nativeImeAdapterAndroid, String text);

    private native void nativeClearFocus(int nativeImeAdapterAndroid);

    private native void nativeSetEditableSelectionOffsets(int nativeImeAdapterAndroid,
            int start, int end);

    private native void nativeSetComposingRegion(int nativeImeAdapterAndroid, int start, int end);

    private native void nativeDeleteSurroundingText(int nativeImeAdapterAndroid,
            int before, int after);

    private native void nativeUnselect(int nativeImeAdapterAndroid);
    private native void nativeSelectAll(int nativeImeAdapterAndroid);
    private native void nativeCut(int nativeImeAdapterAndroid);
    private native void nativeCopy(int nativeImeAdapterAndroid);
    private native void nativePaste(int nativeImeAdapterAndroid);

    private native void nativeRequestTextInputStateUpdate(int nativeImeAdapterAndroid,
        long requestTime);
}
