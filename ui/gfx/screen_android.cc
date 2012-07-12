// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/screen.h"

#include "base/logging.h"

namespace gfx {

// static
gfx::Size Screen::GetPrimaryMonitorSize() {
  NOTIMPLEMENTED();
  // TODO(carlosvaldivia): This is a stopgap. Should eventually query Android
  // for this information. It is only safe because it should only be used for
  return gfx::Size(1, 1);
}

// static
int Screen::GetNumMonitors() {
  return 1;
}

}  // namespace gfx
