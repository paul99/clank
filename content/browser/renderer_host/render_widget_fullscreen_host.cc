// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_fullscreen_host.h"
#include "content/public/browser/render_process_host.h"

RenderWidgetFullscreenHost::RenderWidgetFullscreenHost(
    content::RenderProcessHost* process, int routing_id)
    : RenderWidgetHost(process, routing_id) {
}

