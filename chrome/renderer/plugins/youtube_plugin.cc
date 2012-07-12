// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/plugins/youtube_plugin.h"

#include "base/bind.h"
#include "base/string_piece.h"
#include "base/string_util.h"
#include "base/values.h"
#include "chrome/common/jstemplate_builder.h"
#include "content/public/renderer/render_view.h"
#include "grit/generated_resources.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebURLRequest.h"
#include "ui/base/resource/resource_bundle.h"
#include "webkit/plugins/webview_plugin.h"

using WebKit::WebFrame;
using WebKit::WebPluginParams;
using WebKit::WebURLRequest;
using webkit::WebViewPlugin;

// static
WebViewPlugin* YoutubePlugin::Create(content::RenderView* render_view,
                                     WebFrame* frame,
                                     const WebPluginParams& params,
                                     int template_id,
                                     const std::string& url_path) {
  const base::StringPiece template_html(
      ResourceBundle::GetSharedInstance().GetRawDataResource(template_id));

  DCHECK(!template_html.empty()) << "unable to load template. ID: "
                                 << template_id;
  DictionaryValue values;
  std::string html_data = jstemplate_builder::GetI18nTemplateHtml(
      template_html, &values);

  const std::string kSlashVSlash = "/v/";
  const std::string kSlashESlash = "/e/";

  std::string video_id = url_path.substr(kSlashVSlash.length());
  // Extract just the video id
  size_t video_id_end = video_id.find('&');
  if (video_id_end != std::string::npos)
    video_id = video_id.substr(0, video_id_end);

  std::string find_this("VIDEO_ID");
  ReplaceSubstringsAfterOffset(&html_data, 0, find_this, video_id);

  // |youtube_plugin| will destroy itself when its WebViewPlugin is going away.
  YoutubePlugin* youtube_plugin = new YoutubePlugin(
      render_view, frame, params, html_data, video_id);
  return youtube_plugin->plugin();
}

bool YoutubePlugin::isValidYouTubeVideo(const std::string& path) {
  const std::string kSlashVSlash = "/v/";
  const std::string kSlashESlash = "/e/";

  unsigned len = path.length();
  if (len <= kSlashVSlash.length()) // check for more than just /v/ or /e/
    return false;

  std::string str = StringToLowerASCII(path);
  // Youtube flash url can start with /v/ or /e/
  if (0 != str.compare(0, kSlashVSlash.length(), kSlashVSlash) &&
      0 != str.compare(0, kSlashESlash.length(), kSlashESlash))
    return false;

  // Start after /v/
  for (unsigned i = kSlashVSlash.length(); i < len; i++) {
    char c = str[i];
    if (isalpha(c) || isdigit(c) || c == '_' || c == '-')
      continue;
    // The url can have more parameters such as &hl=en after the video id.
    // Once we start seeing extra parameters we can return true.
    return c == '&' && i > kSlashVSlash.length();
  }
  return true;
}

bool YoutubePlugin::isYouTubeUrl(const GURL& url,
                                 const std::string& mime_type) {
  std::string host = url.host();
  std::string youtube = "youtube.com";
  std::string youtube_nocookie = "youtube-nocookie.com";
  bool is_youtube = EndsWith(host, youtube, true) ||
      EndsWith(host, youtube_nocookie, true);

  return is_youtube && isValidYouTubeVideo(url.path()) &&
      LowerCaseEqualsASCII(mime_type, "application/x-shockwave-flash");
}

YoutubePlugin::YoutubePlugin(content::RenderView* render_view,
                             WebFrame* frame,
                             const WebPluginParams& params,
                             const std::string& html_data,
                             const std::string& video_id)
    : PluginPlaceholder(render_view, frame, params, html_data),
      video_id_(video_id) {
}

YoutubePlugin::~YoutubePlugin() {
}

void YoutubePlugin::BindWebFrame(WebFrame* frame) {
  PluginPlaceholder::BindWebFrame(frame);
  BindCallback("openURL", base::Bind(&YoutubePlugin::OpenUrlCallback,
                                     base::Unretained(this)));
}

void YoutubePlugin::OpenUrlCallback(const CppArgumentList& args,
                                    CppVariant* result) {
  std::string youtube("vnd.youtube:");
  GURL url(youtube.append(video_id_));
  WebURLRequest request;
  request.initialize();
  request.setURL(url);
  render_view()->LoadURLExternally(
      frame(), request, WebKit::WebNavigationPolicyCurrentTab);
}
