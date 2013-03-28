// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/image_cursors.h"

#include "base/logging.h"
#include "ui/base/cursor/cursor_loader.h"
#include "ui/base/cursor/cursor.h"
#include "ui/gfx/point.h"
#include "grit/ash_resources.h"
#include "grit/ui_resources.h"

namespace {

const int kAnimatedCursorFrameDelayMs = 25;

struct HotPoint {
  int x;
  int y;
};

struct CursorData {
  int id;
  int resource_id;
  HotPoint hot_1x;
  HotPoint hot_2x;
};

// TODO(oshima): Remove this comment (http://crbug.com/141586).
// The cursor's hot points are defined in chromeos cursor images at:
// http://folder/kuscher/projects/Chrome_OS/Pointers/focuspoint
const CursorData kImageCursors[] = {
  {ui::kCursorNull, IDR_AURA_CURSOR_PTR, {4, 4}, {8, 9}},
  {ui::kCursorPointer, IDR_AURA_CURSOR_PTR, {4, 4}, {8, 9}},
  {ui::kCursorNoDrop, IDR_AURA_CURSOR_NO_DROP, {4, 4}, {8, 9}},
  {ui::kCursorNotAllowed, IDR_AURA_CURSOR_NO_DROP, {4, 4}, {8, 9}},
  {ui::kCursorCopy, IDR_AURA_CURSOR_COPY, {4, 4}, {8, 9}},
  {ui::kCursorHand, IDR_AURA_CURSOR_HAND, {9, 4}, {19, 8}},
  {ui::kCursorMove, IDR_AURA_CURSOR_MOVE, {11, 11}, {23, 23}},
  {ui::kCursorNorthEastResize, IDR_AURA_CURSOR_NORTH_EAST_RESIZE,
   {12, 11}, {25, 23}},
  {ui::kCursorSouthWestResize, IDR_AURA_CURSOR_SOUTH_WEST_RESIZE,
   {12, 11}, {25, 23}},
  {ui::kCursorSouthEastResize, IDR_AURA_CURSOR_SOUTH_EAST_RESIZE,
   {11, 11}, {24, 23}},
  {ui::kCursorNorthWestResize, IDR_AURA_CURSOR_NORTH_WEST_RESIZE,
   {11, 11}, {24, 23}},
  {ui::kCursorNorthResize, IDR_AURA_CURSOR_NORTH_RESIZE, {11, 12}, {23, 23}},
  {ui::kCursorSouthResize, IDR_AURA_CURSOR_SOUTH_RESIZE, {11, 12}, {23, 23}},
  {ui::kCursorEastResize, IDR_AURA_CURSOR_EAST_RESIZE, {12, 11}, {25, 23}},
  {ui::kCursorWestResize, IDR_AURA_CURSOR_WEST_RESIZE, {12, 11}, {25, 23}},
  {ui::kCursorIBeam, IDR_AURA_CURSOR_IBEAM, {12, 12}, {24, 25}},
  {ui::kCursorAlias, IDR_AURA_CURSOR_ALIAS, {8, 6}, {15, 11}},
  {ui::kCursorCell, IDR_AURA_CURSOR_CELL, {11, 11}, {24, 23}},
  {ui::kCursorContextMenu, IDR_AURA_CURSOR_CONTEXT_MENU, {4, 4}, {8, 9}},
  {ui::kCursorCross, IDR_AURA_CURSOR_CROSSHAIR, {12, 12}, {25, 23}},
  {ui::kCursorHelp, IDR_AURA_CURSOR_HELP, {4, 4}, {8, 9}},
  {ui::kCursorVerticalText, IDR_AURA_CURSOR_XTERM_HORIZ, {12, 11}, {26, 23}},
  {ui::kCursorZoomIn, IDR_AURA_CURSOR_ZOOM_IN, {10, 10}, {20, 20}},
  {ui::kCursorZoomOut, IDR_AURA_CURSOR_ZOOM_OUT, {10, 10}, {20, 20}},
  {ui::kCursorRowResize, IDR_AURA_CURSOR_ROW_RESIZE, {11, 12}, {23, 23}},
  {ui::kCursorColumnResize, IDR_AURA_CURSOR_COL_RESIZE, {12, 11}, {25, 23}},
  {ui::kCursorEastWestResize, IDR_AURA_CURSOR_EAST_WEST_RESIZE,
   {12, 11}, {25, 23}},
  {ui::kCursorNorthSouthResize, IDR_AURA_CURSOR_NORTH_SOUTH_RESIZE,
   {11, 12}, {23, 23}},
  {ui::kCursorNorthEastSouthWestResize,
   IDR_AURA_CURSOR_NORTH_EAST_SOUTH_WEST_RESIZE, {12, 11}, {25, 23}},
  {ui::kCursorNorthWestSouthEastResize,
   IDR_AURA_CURSOR_NORTH_WEST_SOUTH_EAST_RESIZE, {11, 11}, {24, 23}},
  {ui::kCursorGrab, IDR_AURA_CURSOR_GRAB, {8, 5}, {16, 10}},
  {ui::kCursorGrabbing, IDR_AURA_CURSOR_GRABBING, {9, 9}, {18, 18}},
};

const CursorData kAnimatedCursors[] = {
  {ui::kCursorWait, IDR_THROBBER, {7, 7}, {14, 14}},
  {ui::kCursorProgress, IDR_THROBBER, {7, 7}, {14, 14}},
};

}  // namespace

namespace ash {

ImageCursors::ImageCursors() {
}

ImageCursors::~ImageCursors() {
}

float ImageCursors::GetDeviceScaleFactor() const {
  if (!cursor_loader_.get()) {
    NOTREACHED();
    // Returning 1.0f on release build as it's not serious enough to crash
    // even if this ever happens.
    return 1.0f;
  }
  return cursor_loader_->device_scale_factor();
}

bool ImageCursors::SetDeviceScaleFactor(float device_scale_factor) {
  if (!cursor_loader_.get())
    cursor_loader_.reset(ui::CursorLoader::Create());
  else if (GetDeviceScaleFactor() == device_scale_factor)
    return false;

  cursor_loader_->UnloadAll();
  cursor_loader_->set_device_scale_factor(device_scale_factor);

  for (size_t i = 0; i < arraysize(kImageCursors); ++i) {
    const HotPoint& hot = device_scale_factor == 1.0f ?
        kImageCursors[i].hot_1x : kImageCursors[i].hot_2x;
    cursor_loader_->LoadImageCursor(kImageCursors[i].id,
                                    kImageCursors[i].resource_id,
                                    gfx::Point(hot.x, hot.y));
  }
  for (size_t i = 0; i < arraysize(kAnimatedCursors); ++i) {
    const HotPoint& hot = device_scale_factor == 1.0f ?
        kAnimatedCursors[i].hot_1x : kAnimatedCursors[i].hot_2x;
    cursor_loader_->LoadAnimatedCursor(kAnimatedCursors[i].id,
                                       kAnimatedCursors[i].resource_id,
                                       gfx::Point(hot.x, hot.y),
                                       kAnimatedCursorFrameDelayMs);
  }
  return true;
}

void ImageCursors::SetPlatformCursor(gfx::NativeCursor* cursor) {
  cursor_loader_->SetPlatformCursor(cursor);
}

}  // namespace ash
