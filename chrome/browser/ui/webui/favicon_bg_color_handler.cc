// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/favicon_bg_color_handler.h"

#include "base/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/img_bg_color_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"

using content::BrowserThread;

namespace {
  const SkColor kDefaultBgColor = SK_ColorWHITE;
  const int kDefaultId = -1;
} // namespace

FaviconBgColorHandler::FaviconBgColorHandler(Profile* profile)
    : profile_(profile) {
}

FaviconBgColorHandler::~FaviconBgColorHandler() {
}

void FaviconBgColorHandler::RegisterMessages() {
  // Register ourselves as the handler for the "getRecommendedFaviconBgColor"
  // message from Javascript.
  web_ui()->RegisterMessageCallback("getRecommendedFaviconBgColor",
      base::Bind(&FaviconBgColorHandler::HandleGetRecommendedBgColor,
                 base::Unretained(this)));
}

void FaviconBgColorHandler::HandleGetRecommendedBgColor(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  FaviconService* favicon_service = NULL;

  if (profile_ != NULL) {
    favicon_service = profile_->GetFaviconService(Profile::EXPLICIT_ACCESS);
  }

  // TODO (dtrainor) figure out why Javascript is only passing double instead
  // of int... even with parseInt in front.
  double id = kDefaultId;
  std::string path;

  if (favicon_service && args && !args->empty() && args->GetString(0, &path)
      && args->GetDouble(1, &id)) {
    FaviconService::Handle handle;

    // Currently we have to load the favicon image data in order to process
    // the image and grab the background color.
    GURL page_url(path);
    handle = favicon_service->GetFaviconForURL(
        page_url,
        history::FAVICON | history::TOUCH_ICON,
        &cancelable_consumer_,
        base::Bind(&FaviconBgColorHandler::FaviconImageDataAvailable,
                   base::Unretained(this)));

    cancelable_consumer_.SetClientData(favicon_service, handle, (int)id);
  } else {
    SendBackColor(kDefaultId, kDefaultBgColor);
  }
}

void FaviconBgColorHandler::FaviconImageDataAvailable(
    FaviconService::Handle request_handle,
    history::FaviconData favicon) {
  SkColor color = kDefaultBgColor;

  FaviconService* favicon_service =
      profile_->GetFaviconService(Profile::EXPLICIT_ACCESS);

  int id = cancelable_consumer_.GetClientData(favicon_service,
                                                 request_handle);

  if (favicon.known_icon &&
      favicon.image_data.get() &&
      favicon.image_data->size()) {
    // This is a somewhat expensive call.  It must decode the PNG and iterate
    // over the pixels multiple times to calculate an appropriate background
    // color.
    color = CalculateRecommendedBgColorForPNG(favicon.image_data);
  }

  SendBackColor(id, color);
}

void FaviconBgColorHandler::SendBackColor(int id, SkColor color) {
  scoped_ptr<Value> id_value(Value::CreateIntegerValue(id));
  scoped_ptr<Value> r_value(Value::CreateIntegerValue(SkColorGetR(color)));
  scoped_ptr<Value> g_value(Value::CreateIntegerValue(SkColorGetG(color)));
  scoped_ptr<Value> b_value(Value::CreateIntegerValue(SkColorGetB(color)));
  web_ui()->CallJavascriptFunction("recommendedFaviconBgColor",
                                  *id_value,
                                  *r_value,
                                  *g_value,
                                  *b_value);
}
