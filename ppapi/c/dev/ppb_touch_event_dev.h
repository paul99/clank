/* Copyright (c) 2011 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef PPAPI_C_DEV_PPB_TOUCH_EVENT_DEV_H_
#define PPAPI_C_DEV_PPB_TOUCH_EVENT_DEV_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_TOUCH_INPUT_EVENT_DEV_INTERFACE_0_1 "PPB_TouchInputEvent(Dev);0.1"
#define PPB_TOUCH_INPUT_EVENT_DEV_INTERFACE PPB_TOUCH_INPUT_EVENT_DEV_INTERFACE_0_1

/**
 * Represents a single touch point.
 */
struct PP_TouchPoint_Dev {
  /**
   * The identifier for this TouchPoint. This corresponds to the order
   * in which the points were pressed. For example, the first point to be
   * pressed has an id of 0, the second has an id of 1, and so on. An id can be
   * reused when a touch point is released.  For example, if two fingers are
   * down, with id 0 and 1, and finger 0 releases, the next finger to be
   * pressed can be assigned to id 0.
   */
  uint32_t id;

  /**
   * The x-y coordinates of this TouchPoint, in DOM coordinate space.
   */
  struct PP_FloatPoint position;
  /**
   * The elliptical radii, in screen pixels, in the x and y direction of this
   * TouchPoint.
   */
  struct PP_FloatPoint radius;
  /**
   * The angle of rotation in degrees of the elliptical model of this TouchPoint
   * clockwise from "up."
   */
  float rotation_angle;
  /**
   * The pressure applied to this TouchPoint.  This is typically a
   * value between 0 and 1, with 0 indicating no pressure and 1 indicating
   * some maximum pressure, but scaling differs depending on the hardware and
   * the value is not guaranteed to stay within that range.
   */
  float pressure;
};

typedef enum {

  /**
   * The list of all TouchPoints which are currently down.
   */
  PP_TOUCHLIST_TYPE_TOUCHES,

  /**
   * The list of all TouchPoints whose state has changed since the last
   * TouchInputEvent.
   */
  PP_TOUCHLIST_TYPE_CHANGEDTOUCHES,

  /**
   * The list of all TouchPoints which are targeting this plugin.  This is a
   * subset of Touches.
   */
  PP_TOUCHLIST_TYPE_TARGETTOUCHES
} PP_TouchListType;

struct PPB_TouchInputEvent_Dev {
  /**
   * Determines if a resource is a touch event.
   *
   * @return PP_TRUE if the given resource is a valid touch input event.
   */
  PP_Bool (*IsTouchInputEvent)(PP_Resource resource);

  /**
   * @return The number of TouchPoints in the given list
   */
  uint32_t (*GetTouchCount)(PP_Resource resource, PP_TouchListType list);

  /**
   * @return The TouchPoint at the given index, or an empty TouchPoint if the
   * given index is out of range
   */
  PP_TouchPoint_Dev (*GetTouchByIndex)(PP_Resource resource,
                                       PP_TouchListType list,
                                       uint32_t index);

  /**
   * @return The TouchPoint with the given identifier, or an empty TouchPoint
   * if this TouchList does not contain a TouchPoint with that identifier.
   */
  PP_TouchPoint_Dev (*GetTouchById)(PP_Resource resource,
                                    PP_TouchListType list,
                                    uint32_t id);
};

#endif  /* PPAPI_C_DEV_PPB_TOUCH_EVENT_DEV_H_ */
