// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;

import org.chromium.base.CalledByNative;

// TODO(benm): This implements the default JS modal dialog behavior
// (i.e popping up a dialog). This should be in the framework, not the app
// but as it makes use of resources to layout the dialog, it's hard to put
// it there until we are integrated more fully with the Android framework.

public class JSModalDialog {
    private String mTitle;
    private String mMessage;
    private boolean mShouldShowSuppressCheckBox;
    private int mNativeDialogPointer;
    private AlertDialog mDialog;

    private JSModalDialog(String title, String message, boolean shouldShowSuppressCheckBox) {
        mTitle = title;
        mMessage = message;
        mShouldShowSuppressCheckBox = shouldShowSuppressCheckBox;
    }

    @CalledByNative
    public static JSModalDialog createAlertDialog(String title, String message,
            boolean shouldShowSuppressCheckBox) {
        return new JSAlertDialog(title, message, shouldShowSuppressCheckBox);
    }

    @CalledByNative
    public static JSModalDialog createConfirmDialog(String title, String message,
            boolean shouldShowSuppressCheckBox) {
        return new JSConfirmDialog(title, message, shouldShowSuppressCheckBox);
    }

    @CalledByNative
    public static JSModalDialog createPromptDialog(String title, String message,
            boolean shouldShowSuppressCheckBox, String defaultPromptText) {
        return new JSPromptDialog(title, message, shouldShowSuppressCheckBox, defaultPromptText);
    }

    @CalledByNative
    void showJSModalDialog(View view, int nativeDialogPointer) {
        assert view != null;

        Context context = view.getContext();

        // Cache the native dialog pointer so that we can use it to return the response.
        mNativeDialogPointer = nativeDialogPointer;

        // TODO(benm): Need to give the embedding app a chance to handle the dialog once this
        // code is moved into the framework.

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        ViewGroup dialogLayout = (ViewGroup) inflater.inflate(R.layout.js_modal_dialog, null);

        prepare(dialogLayout);

        mDialog = new AlertDialog.Builder(context)
            .setView(dialogLayout)
            .setOnCancelListener(new DialogInterface.OnCancelListener() {
                @Override
                public void onCancel(DialogInterface dialog) {
                    cancel(false);
                }
            })
            .create();
        mDialog.setCanceledOnTouchOutside(false);
        mDialog.show();
    }

    @CalledByNative
    void dismiss() {
        mDialog.dismiss();
    }

    void prepare(final ViewGroup layout) {
        // Set the title and message.
        TextView titleView = (TextView) layout.findViewById(R.id.js_modal_dialog_title);
        TextView messageView = (TextView) layout.findViewById(R.id.js_modal_dialog_message);
        titleView.setText(mTitle);
        messageView.setText(mMessage);

        // Setup the OK button.
        Button okButton = (Button) layout.findViewById(R.id.js_modal_dialog_button_confirm);
        okButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                boolean suppress =
                        ((CheckBox)layout.findViewById(R.id.suppress_js_modal_dialogs)).isChecked();
                String prompt = ((TextView)layout.findViewById(
                        R.id.js_modal_dialog_prompt)).getText().toString();

                confirm(prompt, suppress);
                mDialog.dismiss();
            }
        });

        // Setup the Cancel button.
        Button cancelButton = (Button) layout.findViewById(R.id.js_modal_dialog_button_cancel);
        cancelButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                boolean suppress =
                        ((CheckBox)layout.findViewById(R.id.suppress_js_modal_dialogs)).isChecked();

                cancel(suppress);
                mDialog.dismiss();
            }
        });

        // Display the checkbox for supressing dialogs if necessary.
        layout.findViewById(R.id.suppress_js_modal_dialogs).setVisibility(
                mShouldShowSuppressCheckBox ? View.VISIBLE : View.GONE);
    }

    public void confirm(String promptResult, boolean suppressDialogs) {
        nativeDidAcceptAppModalDialog(mNativeDialogPointer, promptResult, suppressDialogs);
    }

    public void cancel(boolean suppressDialogs) {
        nativeDidCancelAppModalDialog(mNativeDialogPointer, suppressDialogs);
    }

    private static class JSAlertDialog extends JSModalDialog {
        public JSAlertDialog(String title, String message, boolean shouldShowSuppressCheckBox) {
            super(title, message, shouldShowSuppressCheckBox);
        }

        public void prepare(ViewGroup layout) {
            super.prepare(layout);
            layout.findViewById(R.id.js_modal_dialog_button_cancel).setVisibility(View.GONE);
        }
    }

    private static class JSConfirmDialog extends JSModalDialog {
        public JSConfirmDialog(String title, String message, boolean shouldShowSuppressCheckBox) {
            super(title, message, shouldShowSuppressCheckBox);
        }

        public void prepare(ViewGroup layout) {
            super.prepare(layout);
        }
    }

    private static class JSPromptDialog extends JSModalDialog {
        private String mDefaultPromptText;

        public JSPromptDialog(String title, String message,
                boolean shouldShowSuppressCheckBox, String defaultPromptText) {
            super(title, message, shouldShowSuppressCheckBox);
            mDefaultPromptText = defaultPromptText;
        }

        public void prepare(ViewGroup layout) {
            super.prepare(layout);
            EditText prompt = (EditText) layout.findViewById(R.id.js_modal_dialog_prompt);
            prompt.setVisibility(View.VISIBLE);

            if (mDefaultPromptText.length() > 0) {
                prompt.setText(mDefaultPromptText);
                prompt.selectAll();
            }
        }
    }

    private native void nativeDidAcceptAppModalDialog(int nativeJSModalDialogAndroid,
            String prompt, boolean suppress);
    private native void nativeDidCancelAppModalDialog(int nativeJSModalDialogAndroid,
            boolean suppress);
}
