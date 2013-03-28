// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines all the "cc" command-line switches.

#ifndef CC_SWITCHES_H_
#define CC_SWITCHES_H_

#include "cc/cc_export.h"

// Since cc is used from the render process, anything that goes here also needs
// to be added to render_process_host_impl.cc.

namespace cc {
namespace switches {

CC_EXPORT extern const char kBackgroundColorInsteadOfCheckerboard[];
CC_EXPORT extern const char kDisableThreadedAnimation[];
CC_EXPORT extern const char kEnableCompositorFrameMessage[];
CC_EXPORT extern const char kEnableImplSidePainting[];
CC_EXPORT extern const char kEnablePartialSwap[];
CC_EXPORT extern const char kEnablePerTilePainting[];
CC_EXPORT extern const char kJankInsteadOfCheckerboard[];
CC_EXPORT extern const char kNumRasterThreads[];
CC_EXPORT extern const char kShowPropertyChangedRects[];
CC_EXPORT extern const char kShowSurfaceDamageRects[];
CC_EXPORT extern const char kShowScreenSpaceRects[];
CC_EXPORT extern const char kShowReplicaScreenSpaceRects[];
CC_EXPORT extern const char kShowOccludingRects[];
CC_EXPORT extern const char kShowNonOccludingRects[];
CC_EXPORT extern const char kTraceOverdraw[];

}  // namespace switches
}  // namespace cc

#endif  // CC_SWITCHES_H_
