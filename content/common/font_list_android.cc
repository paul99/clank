// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/font_list.h"

#include "base/logging.h"
#include "base/values.h"

namespace content {

ListValue* GetFontList_SlowBlocking() {
  return new ListValue;
}

}  // namespace content
