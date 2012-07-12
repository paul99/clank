// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PLUGINS_YOUTUBE_PLUGIN_H_
#define CHROME_RENDERER_PLUGINS_YOUTUBE_PLUGIN_H_
#pragma once

#include "chrome/renderer/plugins/plugin_placeholder.h"
#include "webkit/plugins/webplugininfo.h"

class YoutubePlugin : public PluginPlaceholder  {
 public:
  // Creates a new WebViewPlugin with a YoutubePlugin as a delegate.
  static webkit::WebViewPlugin* Create(content::RenderView* render_view,
                                       WebKit::WebFrame* frame,
                                       const WebKit::WebPluginParams& params,
                                       int resource_id,
                                       const std::string& url_path);

  static bool isYouTubeUrl(const GURL& url, const std::string& mime_type);
  static bool isValidYouTubeVideo(const std::string& path);

  YoutubePlugin(content::RenderView* render_view,
                WebKit::WebFrame* frame,
                const WebKit::WebPluginParams& params,
                const std::string& html_data,
                const std::string& video_id);

  // WebViewPlugin::Delegate methods:
  virtual void BindWebFrame(WebKit::WebFrame* frame) OVERRIDE;

 private:
  virtual ~YoutubePlugin();

  // Javascript callbacks:
  // Opens a youtube app in the current tab.
  void OpenUrlCallback(const CppArgumentList& args, CppVariant* result);

  std::string video_id_;
};

#endif  // CHROME_RENDERER_PLUGINS_YOUTUBE_PLUGIN_H_
