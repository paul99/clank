// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_SESSION_STATE_OBSERVER_H_
#define ASH_WM_SESSION_STATE_OBSERVER_H_

#include "ash/ash_export.h"

namespace ash {

// Interface for classes that want to be notified by SessionStateController when
// session-related events occur.
class ASH_EXPORT SessionStateObserver {
 public:
  enum EventType {
    EVENT_PRELOCK_ANIMATION_STARTED,
    EVENT_LOCK_ANIMATION_STARTED,
    EVENT_LOCK_ANIMATION_FINISHED,
  };

  virtual void OnSessionStateEvent(EventType event) = 0;
  virtual ~SessionStateObserver() {};
};

}  // namespace ash

#endif  // ASH_WM_SESSION_STATE_CONTROLLER_H_
