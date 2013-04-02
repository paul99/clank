// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layer_tree_debug_state.h"

#include "base/logging.h"

namespace cc {

// IMPORTANT: new fields must be added to equal() and unite()
LayerTreeDebugState::LayerTreeDebugState()
  : showFPSCounter(false)
  , showPlatformLayerTree(false)
  , showDebugBorders(false)
  , continuousPainting(false)
  , showPaintRects(false)
  , showPropertyChangedRects(false)
  , showSurfaceDamageRects(false)
  , showScreenSpaceRects(false)
  , showReplicaScreenSpaceRects(false)
  , showOccludingRects(false)
  , showNonOccludingRects(false)
  , slowDownRasterScaleFactor(0)
  , m_recordRenderingStats(false) { }

LayerTreeDebugState::~LayerTreeDebugState() {
}

void LayerTreeDebugState::setRecordRenderingStats(bool enabled) {
    m_recordRenderingStats = enabled;
}

bool LayerTreeDebugState::recordRenderingStats() const {
    return m_recordRenderingStats || continuousPainting;
}

bool LayerTreeDebugState::showHudInfo() const {
    return showFPSCounter || showPlatformLayerTree || continuousPainting || showHudRects();
}

bool LayerTreeDebugState::showHudRects() const {
    return showPaintRects || showPropertyChangedRects || showSurfaceDamageRects || showScreenSpaceRects || showReplicaScreenSpaceRects || showOccludingRects || showNonOccludingRects;
}

bool LayerTreeDebugState::hudNeedsFont() const {
    return showFPSCounter || showPlatformLayerTree || continuousPainting;
}

bool LayerTreeDebugState::equal(const LayerTreeDebugState& a, const LayerTreeDebugState& b) {
    return (a.showFPSCounter == b.showFPSCounter &&
            a.showPlatformLayerTree == b.showPlatformLayerTree &&
            a.showDebugBorders == b.showDebugBorders &&
            a.continuousPainting == b.continuousPainting &&
            a.showPaintRects == b.showPaintRects &&
            a.showPropertyChangedRects == b.showPropertyChangedRects &&
            a.showSurfaceDamageRects == b.showSurfaceDamageRects &&
            a.showScreenSpaceRects == b.showScreenSpaceRects &&
            a.showReplicaScreenSpaceRects == b.showReplicaScreenSpaceRects &&
            a.showOccludingRects == b.showOccludingRects &&
            a.showNonOccludingRects == b.showNonOccludingRects &&
            a.slowDownRasterScaleFactor == b.slowDownRasterScaleFactor &&
            a.m_recordRenderingStats == b.m_recordRenderingStats);
}

LayerTreeDebugState LayerTreeDebugState::unite(const LayerTreeDebugState& a, const LayerTreeDebugState& b) {
    LayerTreeDebugState r(a);

    r.showFPSCounter |= b.showFPSCounter;
    r.showPlatformLayerTree |= b.showPlatformLayerTree;
    r.showDebugBorders |= b.showDebugBorders;
    r.continuousPainting |= b.continuousPainting;

    r.showPaintRects |= b.showPaintRects;
    r.showPropertyChangedRects |= b.showPropertyChangedRects;
    r.showSurfaceDamageRects |= b.showSurfaceDamageRects;
    r.showScreenSpaceRects |= b.showScreenSpaceRects;
    r.showReplicaScreenSpaceRects |= b.showReplicaScreenSpaceRects;
    r.showOccludingRects |= b.showOccludingRects;
    r.showNonOccludingRects |= b.showNonOccludingRects;

    if (b.slowDownRasterScaleFactor)
      r.slowDownRasterScaleFactor = b.slowDownRasterScaleFactor;

    r.m_recordRenderingStats |= b.m_recordRenderingStats;

    return r;
}

}  // namespace cc
