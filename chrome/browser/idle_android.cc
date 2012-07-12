// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "chrome/browser/idle.h"

void CalculateIdleState(unsigned int idle_threshold, IdleCallback notify) {
  NOTIMPLEMENTED();
  notify.Run(IDLE_STATE_ACTIVE);
}
