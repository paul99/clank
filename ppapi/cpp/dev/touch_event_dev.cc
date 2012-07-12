// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/dev/touch_event_dev.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_TouchInputEvent_Dev>() {
  return PPB_TOUCH_INPUT_EVENT_DEV_INTERFACE;
}

} // namespace

TouchInputEvent_Dev::TouchInputEvent_Dev(const InputEvent& event)
                                       : InputEvent() {

  // Type check the input event before setting it.
  if (!has_interface<PPB_TouchInputEvent_Dev>())
    return;
  if (get_interface<PPB_TouchInputEvent_Dev>()->IsTouchInputEvent(
      event.pp_resource())) {
    Module::Get()->core()->AddRefResource(event.pp_resource());
    PassRefFromConstructor(event.pp_resource());
  }
}

uint32_t TouchInputEvent_Dev::GetTouchCount(PP_TouchListType list) const {
  if (!has_interface<PPB_TouchInputEvent_Dev>())
    return 0;
  return get_interface<PPB_TouchInputEvent_Dev>()->GetTouchCount(pp_resource(),
                                                                 list);
}

TouchPoint_Dev TouchInputEvent_Dev::GetTouchById(PP_TouchListType list,
                                                 uint32_t id) const {
  if (!has_interface<PPB_TouchInputEvent_Dev>())
    return TouchPoint_Dev();
  return TouchPoint_Dev(get_interface<PPB_TouchInputEvent_Dev>()->
                        GetTouchById(pp_resource(), list, id));
}

TouchPoint_Dev TouchInputEvent_Dev::GetTouchByIndex(PP_TouchListType list,
                                                    uint32_t index) const {
  if (!has_interface<PPB_TouchInputEvent_Dev>())
    return TouchPoint_Dev();
  return TouchPoint_Dev(get_interface<PPB_TouchInputEvent_Dev>()->
                        GetTouchByIndex(pp_resource(), list, index));
}

} // namespace pp
