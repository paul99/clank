// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_DEV_TOUCH_EVENT_DEV_H_
#define PPAPI_CPP_DEV_TOUCH_EVENT_DEV_H_

#include "ppapi/c/dev/ppb_touch_event_dev.h"
#include "ppapi/cpp/input_event.h"

namespace pp {

class FloatPoint;

/// Wrapper class for PP_TouchPoint.
class TouchPoint_Dev {
 public:
  TouchPoint_Dev() {}

  /// @return The identifier for this TouchPoint. This corresponds to the order
  /// in which the points were pressed. For example, the first point to be
  /// pressed has an id of 0, the second has an id of 1, and so on. An id can be
  /// reused when a touch point is released.  For example, if two fingers are
  /// down, with id 0 and 1, and finger 0 releases, the next finger to be
  /// pressed can be assigned to id 0.
  uint32_t id() const { return touch_point_.id; }

  /// @return The x-y coordinates of this TouchPoint, in DOM coordinate space.
  FloatPoint position() const {
    return pp::FloatPoint(touch_point_.position);
  }

  /// @return The elliptical radii, in screen pixels, in the x and y direction
  /// of this TouchPoint.
  FloatPoint radii() const { return pp::FloatPoint(touch_point_.radius); }

  /// @return The angle of rotation of the elliptical model of this TouchPoint
  /// from the y-axis.
  float rotation_angle() const { return touch_point_.rotation_angle; }

  /// @return The pressure applied to this TouchPoint.  This is typically a
  /// value between 0 and 1, with 0 indicating no pressure and 1 indicating
  /// some maximum pressure, but scaling differs depending on the hardware and
  /// the value is not guaranteed to stay within that range.
  float pressure() const { return touch_point_.pressure; }

 private:
  friend class TouchInputEvent_Dev;
  TouchPoint_Dev(const PP_TouchPoint_Dev& point) : touch_point_(point) {}
  PP_TouchPoint_Dev touch_point_;
};

class TouchInputEvent_Dev : public InputEvent {
 public:
  /// Constructs an is_null() touch input event object.
  TouchInputEvent_Dev();

  /// Constructs a touch input event object from the given generic input event.
  /// If the given event is itself is_null() or is not a touch input event, the
  /// touch object will be is_null().
  explicit TouchInputEvent_Dev(const InputEvent& event);

  /// @return The number of TouchPoints in this TouchList
  uint32_t GetTouchCount(PP_TouchListType list) const;

  /// @return The TouchPoint at the given index of the given list, or an empty
  /// TouchPoint if the index is out of range
  TouchPoint_Dev GetTouchByIndex(PP_TouchListType list, uint32_t index) const;

  /// @return The TouchPoint in the given list with the given identifier, or an
  /// empty TouchPoint if the list does not contain a TouchPoint with that
  /// identifier.
  TouchPoint_Dev GetTouchById(PP_TouchListType list, uint32_t id) const;
};

}  // namespace pp

#endif  /* PPAPI_CPP_DEV_TOUCH_EVENT_DEV_H_ */
