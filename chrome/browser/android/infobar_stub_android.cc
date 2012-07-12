// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "chrome/browser/infobars/infobar_delegate.h"
#include "chrome/browser/tab_contents/link_infobar_delegate.h"

// LinkInfoBarDelegate, InfoBarDelegate overrides: -----------------------------

InfoBar* LinkInfoBarDelegate::CreateInfoBar(InfoBarTabHelper* owner) {
  NOTIMPLEMENTED();
  return NULL;
}
