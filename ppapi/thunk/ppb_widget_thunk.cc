// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From dev/ppb_widget_dev.idl modified Fri Nov 16 11:26:06 2012.

#include "ppapi/c/dev/ppb_widget_dev.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/ppb_widget_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Bool IsWidget(PP_Resource resource) {
  EnterResource<PPB_Widget_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

PP_Bool Paint(PP_Resource widget,
              const struct PP_Rect* rect,
              PP_Resource image) {
  EnterResource<PPB_Widget_API> enter(widget, false);
  if (enter.failed())
    return PP_FALSE;
  return enter.object()->Paint(rect, image);
}

PP_Bool HandleEvent(PP_Resource widget, PP_Resource input_event) {
  EnterResource<PPB_Widget_API> enter(widget, false);
  if (enter.failed())
    return PP_FALSE;
  return enter.object()->HandleEvent(input_event);
}

PP_Bool GetLocation(PP_Resource widget, struct PP_Rect* location) {
  EnterResource<PPB_Widget_API> enter(widget, false);
  if (enter.failed())
    return PP_FALSE;
  return enter.object()->GetLocation(location);
}

void SetLocation(PP_Resource widget, const struct PP_Rect* location) {
  EnterResource<PPB_Widget_API> enter(widget, false);
  if (enter.succeeded())
    enter.object()->SetLocation(location);
}

void SetScale(PP_Resource widget, float scale) {
  EnterResource<PPB_Widget_API> enter(widget, false);
  if (enter.succeeded())
    enter.object()->SetScale(scale);
}

const PPB_Widget_Dev_0_3 g_ppb_widget_dev_thunk_0_3 = {
  &IsWidget,
  &Paint,
  &HandleEvent,
  &GetLocation,
  &SetLocation,
};

const PPB_Widget_Dev_0_4 g_ppb_widget_dev_thunk_0_4 = {
  &IsWidget,
  &Paint,
  &HandleEvent,
  &GetLocation,
  &SetLocation,
  &SetScale,
};

}  // namespace

const PPB_Widget_Dev_0_3* GetPPB_Widget_Dev_0_3_Thunk() {
  return &g_ppb_widget_dev_thunk_0_3;
}

const PPB_Widget_Dev_0_4* GetPPB_Widget_Dev_0_4_Thunk() {
  return &g_ppb_widget_dev_thunk_0_4;
}

}  // namespace thunk
}  // namespace ppapi
