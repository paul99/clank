// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

class AutofillData {
    public int mQueryId;
    public String mName;
    public String mLabel;
    public int mUniqueId;

    AutofillData(int queryId, String name, String label, int uniqueId) {
        mQueryId = queryId;
        mName = name;
        mLabel = label;
        mUniqueId = uniqueId;
    }

    public String getName() {
        return mName;
    }

    public String getLabel() {
        return mLabel;
    }
}
