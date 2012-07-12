// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListAdapter;
import android.widget.ListPopupWindow;
import android.widget.PopupWindow;
import android.widget.TextView;

class AutofillWindow extends ListPopupWindow {

    private ChromeView mChromeView;
    private int mLayoutResourceId;
    private int mNameResourceId;
    private int mLabelResourceId;
    private int mDesiredWidth;
    private AnchorView mAnchorView;

    private static class AutofillListAdapter extends ArrayAdapter<AutofillData> {
        Context mContext;
        int mLayoutResourceId;
        int mNameResourceId;
        int mLabelResourceId;
        AutofillListAdapter(Context context, int layoutResourceId, int nameResourceId,
                int labelResourceId, AutofillData[] objects) {
            super(context, layoutResourceId, nameResourceId, objects);
            mContext = context;
            mLayoutResourceId = layoutResourceId;
            mNameResourceId = nameResourceId;
            mLabelResourceId = labelResourceId;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            LayoutInflater inflater =
                (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            View layout = inflater.inflate(mLayoutResourceId, null);
            TextView nameView = (TextView) layout.findViewById(mNameResourceId);
            nameView.setText(getItem(position).getName());
            TextView labelView = (TextView) layout.findViewById(mLabelResourceId);
            labelView.setText(getItem(position).getLabel());

            return layout;
        }
    }

    // ListPopupWindow needs an anchor view to determine it's size and
    // position.  We create a view with the given desired width at the text
    // edit area as a stand-in.  This is "Fake" in the sense that it
    // draws nothing, accepts no input, and thwarts all attempts at
    // laying it out "properly".
    private static class AnchorView extends View {

        AnchorView(Context c) {
            super(c);
        }

        public void setSize(Rect r, int desiredWidth) {
            setLeft(r.left);
            // If the desired width is smaller than the text edit box, use the
            // width of the text editbox.
            if (desiredWidth < r.right - r.left) {
                setRight(r.right);
            } else {
                // Make sure that the anchor view does not go outside the screen.
                WindowManager wm =
                    (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
                Display display = wm.getDefaultDisplay();
                if (r.left + desiredWidth > display.getWidth()) {
                    setRight(display.getWidth());
                } else {
                    setRight(r.left + desiredWidth);
                }
            }
            setBottom(r.bottom);
            setTop(r.top);
        }
    }

    AutofillWindow(ChromeView chromeView, int layoutResourceId, int nameResourceId,
            int labelResourceId, AutofillData[] data) {
        super(chromeView.getContext());
        mChromeView = chromeView;
        mLayoutResourceId = layoutResourceId;
        mNameResourceId = nameResourceId;
        mLabelResourceId = labelResourceId;
        mDesiredWidth = 0;
        setAutofillData(data);
        setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    try {
                        ListAdapter adapter = (ListAdapter) parent.getAdapter();
                        AutofillData data = (AutofillData) adapter.getItem(position);
                        mChromeView.autofillPopupSelected(data.mQueryId, position, data.mUniqueId);
                    } catch (ClassCastException e) {
                        Log.w("ChromeView", "error in onItemClick", e);
                        assert false;
                    }
                }
            });
        setWidth(ListPopupWindow.WRAP_CONTENT);
    }

    // Set the position of the autofill popup "around" the input rectangle.
    void setPositionAround(Rect r) {
        if (mAnchorView == null) {
            mAnchorView = new AnchorView(mChromeView.getContext());
            mChromeView.addView(mAnchorView);
            // When the popup goes away so should the fake anchor view.
            setOnDismissListener(new PopupWindow.OnDismissListener() {
                    @Override
                    public void onDismiss() {
                        mChromeView.removeView(mAnchorView);
                        mAnchorView = null;
                    }
                });
        }
        mAnchorView.setSize(r, mDesiredWidth);
        setAnchorView(mAnchorView);
    }

    void setAutofillData(AutofillData[] data) {
        mDesiredWidth = getDesiredWidth(data);
        setAdapter(new AutofillListAdapter(mChromeView.getContext(), mLayoutResourceId,
                mNameResourceId, mLabelResourceId, data));
    }

    // Get the desired popup window width by calculating the maximum text length
    // from Autofill data.
    int getDesiredWidth(AutofillData[] data) {
        int maxTextWidth = 0;
        for (int i = 0; i < data.length; ++i) {
            Paint paint = new Paint();
            Rect bounds = new Rect();

            String text = data[i].getName() + data[i].getLabel();
            // TODO (qinmin): We should inflate the layout and get the text size
            // from the textview.
            paint.setTextSize(18);

            paint.getTextBounds(text, 0, text.length(), bounds);
            if (bounds.width() > maxTextWidth) {
                maxTextWidth = bounds.width();
            }
        }
        // Add padding bytes.
        return maxTextWidth + 30;
    }
}
