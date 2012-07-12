// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_DRAGDROP_DROP_TARGET_H_
#define UI_BASE_DRAGDROP_DROP_TARGET_H_
#pragma once

#include <objidl.h>

#include "base/memory/ref_counted.h"
#include "ui/base/ui_export.h"

// Windows interface.
struct IDropTargetHelper;

namespace ui {

// A DropTarget implementation that takes care of the nitty gritty
// of dnd. While this class is concrete, subclasses will most likely
// want to override various OnXXX methods.
//
// Because DropTarget is ref counted you shouldn't delete it directly,
// rather wrap it in a scoped_refptr. Be sure and invoke RevokeDragDrop(m_hWnd)
// before the HWND is deleted too.
//
// This class is meant to be used in a STA and is not multithread-safe.
class UI_EXPORT DropTarget : public IDropTarget {
 public:
  // Create a new DropTarget associating it with the given HWND.
  explicit DropTarget(HWND hwnd);
  virtual ~DropTarget();

  // When suspended is set to |true|, the drop target does not receive drops
  // from drags initiated within the owning HWND.
  bool suspended() const { return suspended_; }
  void set_suspended(bool suspended) { suspended_ = suspended; }

  // IDropTarget implementation:
  HRESULT __stdcall DragEnter(IDataObject* data_object,
                              DWORD key_state,
                              POINTL cursor_position,
                              DWORD* effect);
  HRESULT __stdcall DragOver(DWORD key_state,
                             POINTL cursor_position,
                             DWORD* effect);
  HRESULT __stdcall DragLeave();
  HRESULT __stdcall Drop(IDataObject* data_object,
                         DWORD key_state,
                         POINTL cursor_position,
                         DWORD* effect);

  // IUnknown implementation:
  HRESULT __stdcall QueryInterface(const IID& iid, void** object);
  ULONG __stdcall AddRef();
  ULONG __stdcall Release();

 protected:
  // Returns the hosting HWND.
  HWND GetHWND() { return hwnd_; }

  // Invoked when the cursor first moves over the hwnd during a dnd session.
  // This should return a bitmask of the supported drop operations:
  // DROPEFFECT_NONE, DROPEFFECT_COPY, DROPEFFECT_LINK and/or
  // DROPEFFECT_MOVE.
  virtual DWORD OnDragEnter(IDataObject* data_object,
                            DWORD key_state,
                            POINT cursor_position,
                            DWORD effect);

  // Invoked when the cursor moves over the window during a dnd session.
  // This should return a bitmask of the supported drop operations:
  // DROPEFFECT_NONE, DROPEFFECT_COPY, DROPEFFECT_LINK and/or
  // DROPEFFECT_MOVE.
  virtual DWORD OnDragOver(IDataObject* data_object,
                           DWORD key_state,
                           POINT cursor_position,
                           DWORD effect);

  // Invoked when the cursor moves outside the bounds of the hwnd during a
  // dnd session.
  virtual void OnDragLeave(IDataObject* data_object);

  // Invoked when the drop ends on the window. This should return the operation
  // that was taken.
  virtual DWORD OnDrop(IDataObject* data_object,
                       DWORD key_state,
                       POINT cursor_position,
                       DWORD effect);

 private:
  // Returns the cached drop helper, creating one if necessary. The returned
  // object is not addrefed. May return NULL if the object couldn't be created.
  static IDropTargetHelper* DropHelper();

  // The data object currently being dragged over this drop target.
  scoped_refptr<IDataObject> current_data_object_;

  // A helper object that is used to provide drag image support while the mouse
  // is dragging over the content area.
  //
  // DO NOT ACCESS DIRECTLY! Use DropHelper() instead, which will lazily create
  // this if it doesn't exist yet. This object can take tens of milliseconds to
  // create, and we don't want to block any window opening for this, especially
  // since often, DnD will never be used. Instead, we force this penalty to the
  // first time it is actually used.
  static IDropTargetHelper* cached_drop_target_helper_;

  // The HWND of the source. This HWND is used to determine coordinates for
  // mouse events that are sent to the renderer notifying various drag states.
  HWND hwnd_;

  // Whether or not we are currently processing drag notifications for drags
  // initiated in this window.
  bool suspended_;

  LONG ref_count_;

  DISALLOW_COPY_AND_ASSIGN(DropTarget);
};

}  // namespace ui

#endif  // UI_BASE_DRAGDROP_DROP_TARGET_H_
