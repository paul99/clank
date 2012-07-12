// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.util.SparseBooleanArray;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.CheckedTextView;
import android.widget.ListView;

class SelectPopupDialog {
    // The currently showing popup dialog, null if none is showing.
    private static SelectPopupDialog sShownDialog;

    // The dialog hosting the popup list view.
    private AlertDialog mListBoxPopup = null;

    private ChromeView mChromeView;

    /**
     * Subclass ArrayAdapter so we can disable OPTION_GROUP items.
     */
    private class SelectPopupArrayAdapter extends ArrayAdapter<String> {
        /**
         * Possible values for mItemEnabled. Keep in sync with the value passed from chrome_view.cc
         */
        final static int POPUP_ITEM_TYPE_GROUP = 0;
        final static int POPUP_ITEM_TYPE_DISABLED = 1;
        final static int POPUP_ITEM_TYPE_ENABLED = 2;

        // Indices the type and enabled state of each item.
        private int[] mItemEnabled;

        public SelectPopupArrayAdapter(String[] labels, int[] enabled, boolean multiple) {
            super(mChromeView.getContext(), multiple ?
                  android.R.layout.select_dialog_multichoice :
                  android.R.layout.select_dialog_singlechoice, labels);
            mItemEnabled = enabled;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (position < 0 || position >= getCount()) {
                return null;
            }

            // Always pass in null so that we will get a new CheckedTextView. Otherwise, an item
            // which was previously used as an <optgroup> element (i.e. has no check), could get
            // used as an <option> element, which needs a checkbox/radio, but it would not have
            // one.
            convertView = super.getView(position, null, parent);
            if (mItemEnabled[position] != POPUP_ITEM_TYPE_ENABLED) {
                if (mItemEnabled[position] == POPUP_ITEM_TYPE_GROUP) {
                    // Currently select_dialog_multichoice & select_dialog_multichoice use
                    // CheckedTextViews. If that changes, the class cast will no longer be valid.
                    ((CheckedTextView) convertView).setCheckMarkDrawable(null);
                } else {
                    // Draw the disabled element in a disabled state.
                    convertView.setEnabled(false);
                }
            }
            return convertView;
        }

        @Override
        public boolean areAllItemsEnabled() {
            return false;
        }

        @Override
        public boolean isEnabled(int position) {
            if (position < 0 || position >= getCount()) {
                return false;
            }
            return mItemEnabled[position] == POPUP_ITEM_TYPE_ENABLED;
        }
    }

    private SelectPopupDialog(ChromeView chromeView, String[] labels, int[] enabled,
            boolean multiple, int[] selected) {
        mChromeView = chromeView;

        final ListView listView = new ListView(mChromeView.getContext());
        AlertDialog.Builder b = new AlertDialog.Builder(mChromeView.getContext())
                .setView(listView)
                .setCancelable(true)
                .setInverseBackgroundForced(true);

        if (multiple) {
            b.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mChromeView.selectPopupMenuItems(getSelectedIndices(listView));
                }});
            b.setNegativeButton(android.R.string.cancel,
                    new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mChromeView.selectPopupMenuItems(null);
            }});
        }
        mListBoxPopup = b.create();
        final SelectPopupArrayAdapter adapter = new SelectPopupArrayAdapter(labels, enabled,
                multiple);
        listView.setAdapter(adapter);
        listView.setFocusableInTouchMode(true);

        if (multiple) {
            listView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
            for (int i = 0; i < selected.length; ++i) {
                listView.setItemChecked(selected[i], true);
            }
        } else {
            listView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
            listView.setOnItemClickListener(new OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View v,
                        int position, long id) {
                    mChromeView.selectPopupMenuItems(getSelectedIndices(listView));
                    mListBoxPopup.dismiss();
                }
            });
            if (selected.length > 0) {
                listView.setSelection(selected[0]);
                listView.setItemChecked(selected[0], true);
            }
        }
        mListBoxPopup.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mChromeView.selectPopupMenuItems(null);
            }
        });
        mListBoxPopup.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                mListBoxPopup = null;
                sShownDialog = null;
            }
        });
    }

    private int[] getSelectedIndices(ListView listView) {
        SparseBooleanArray sparseArray = listView.getCheckedItemPositions();
        int selectedCount = 0;
        for (int i = 0; i < sparseArray.size(); ++i) {
            if (sparseArray.valueAt(i)) {
                selectedCount++;
            }
        }
        int[] indices = new int[selectedCount];
        for (int i = 0, j = 0; i < sparseArray.size(); ++i) {
            if (sparseArray.valueAt(i)) {
                indices[j++] = sparseArray.keyAt(i);
            }
        }
        return indices;
    }

    public static void show(ChromeView chromeView, String[] items, int[] enabled,
            boolean multiple, int[] selectedIndices) {
        // Hide the popup currently showing if any.  This could happen if the user pressed a select
        // and pressed it again before the popup was shown.  In that case, the previous popup is
        // irrelevant and can be hidden.
        hide(null);
        sShownDialog = new SelectPopupDialog(chromeView, items, enabled, multiple,
                selectedIndices);
        sShownDialog.mListBoxPopup.show();
    }

    /**
     * Hides the showing popup menu if any it was triggered by the passed ChromeView. If chromeView
     * is null, hides it regardless of which ChromeView triggered it.
     * @param chromeView
     */
    public static void hide(ChromeView chromeView) {
        if (sShownDialog != null &&
                (chromeView == null || sShownDialog.mChromeView == chromeView)) {
            sShownDialog.mListBoxPopup.dismiss();
        }
    }

    // The methods below are used by tests.
    static SelectPopupDialog getCurrent() {
        return sShownDialog;
    }
}
