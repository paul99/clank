// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/npapi/webplugin_delegate_impl.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCursorInfo.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "webkit/plugins/npapi/plugin_instance.h"
#include "webkit/plugins/npapi/webplugin.h"
#include "webkit/glue/webkit_glue.h"

using WebKit::WebCursorInfo;
using WebKit::WebInputEvent;

namespace webkit {
namespace npapi {

WebPluginDelegateImpl::WebPluginDelegateImpl(
    gfx::PluginWindowHandle containing_view,
    PluginInstance* instance)
    : windowed_handle_(0),
      windowed_did_set_window_(false),
      windowless_(false),
      plugin_(NULL),
      instance_(instance),
      parent_(containing_view),
      quirks_(0),
      handle_event_depth_(0),
      first_set_window_call_(true) {
  memset(&window_, 0, sizeof(window_));
}

WebPluginDelegateImpl::~WebPluginDelegateImpl() {
}

bool WebPluginDelegateImpl::PlatformInitialize() {
  NOTIMPLEMENTED();
  return true;
}

void WebPluginDelegateImpl::PlatformDestroyInstance() {
  NOTIMPLEMENTED();
}

void WebPluginDelegateImpl::Paint(WebKit::WebCanvas* canvas,
                                  const gfx::Rect& rect) {
  NOTIMPLEMENTED();
}

bool WebPluginDelegateImpl::WindowedCreatePlugin() {
  NOTIMPLEMENTED();
  return false;
}

void WebPluginDelegateImpl::WindowedDestroyWindow() {
  NOTIMPLEMENTED();
}

bool WebPluginDelegateImpl::WindowedReposition(
    const gfx::Rect& window_rect,
    const gfx::Rect& clip_rect) {
  NOTIMPLEMENTED();
  return false;
}

void WebPluginDelegateImpl::WindowedSetWindow() {
  NOTIMPLEMENTED();
}

void WebPluginDelegateImpl::WindowlessUpdateGeometry(
    const gfx::Rect& window_rect,
    const gfx::Rect& clip_rect) {

}

void WebPluginDelegateImpl::WindowlessPaint(gfx::NativeDrawingContext hdc,
                                            const gfx::Rect& rect) {
  NOTIMPLEMENTED();
}

void WebPluginDelegateImpl::WindowlessSetWindow() {
  NOTIMPLEMENTED();
}

bool WebPluginDelegateImpl::PlatformHandleInputEvent(
    const WebInputEvent& event, WebCursorInfo* cursor_info) {
  NOTIMPLEMENTED();
  return false;
}

bool WebPluginDelegateImpl::PlatformSetPluginHasFocus(bool focused) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace npapi
}  // namespace webkit
