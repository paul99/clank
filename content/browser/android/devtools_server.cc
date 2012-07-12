// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/devtools_server.h"

#include <pwd.h>

#include "base/stringprintf.h"
#include "chrome/browser/net/chrome_url_request_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_version_info.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/devtools_http_handler_delegate.h"
#include "grit/devtools_discovery_page_resources.h"
#include "ui/base/resource/resource_bundle.h"

using content::WebContents;

namespace android {

const char* kFrontEndURL = "http://chrome-devtools-frontend.appspot.com/static/%s/devtools.html";
const char* kSocketName = "chrome_devtools_remote";

class TabContentsProvider : public content::DevToolsHttpHandlerDelegate {
 public:
  TabContentsProvider(std::set<WebContents*>* inspectable_tabs)
      : inspectable_tabs_(inspectable_tabs) {
  }
  virtual ~TabContentsProvider() {}

  // DevToolsHttpHandler::TabContentsProvider implementation
  content::DevToolsHttpHandlerDelegate::InspectableTabs GetInspectableTabs() {
    return std::vector<WebContents*>(inspectable_tabs_->begin(),
                                     inspectable_tabs_->end());
  }

  virtual std::string GetDiscoveryPageHTML() {
    return ResourceBundle::GetSharedInstance().GetRawDataResource(
        IDR_DEVTOOLS_DISCOVERY_PAGE_HTML).as_string();
  }

  virtual net::URLRequestContext* GetURLRequestContext() {
    net::URLRequestContextGetter* getter =
        Profile::Deprecated::GetDefaultRequestContext();
    return getter ? getter->GetURLRequestContext() : NULL;
  }

  virtual bool BundlesFrontendResources() {
    NOTIMPLEMENTED();
    return false;
  }

  virtual std::string GetFrontendResourcesBaseURL() {
    NOTIMPLEMENTED();
    return NULL;
  }

 private:
  std::set<WebContents*>* inspectable_tabs_;
};

}  // anonymous namespace

// static
DevToolsServer* DevToolsServer::GetInstance() {
  return Singleton<DevToolsServer>::get();
}

void DevToolsServer::Start() {
  Stop();
  chrome::VersionInfo info;
  std::string url = StringPrintf(android::kFrontEndURL, info.Version().c_str());
  protocol_handler_ = content::DevToolsHttpHandler::Start(
      android::kSocketName,
      url,
      new android::TabContentsProvider(&inspectable_tabs_),
      this);
}

void DevToolsServer::Stop() {
  if (protocol_handler_) {
    protocol_handler_->Stop();
    protocol_handler_ = NULL;
  }
}

void DevToolsServer::AddInspectableTab(WebContents* contents) {
  inspectable_tabs_.insert(contents);
}

void DevToolsServer::RemoveInspectableTab(WebContents* contents) {
  inspectable_tabs_.erase(contents);
}

bool DevToolsServer::UserCanConnect(uid_t uid, gid_t gid) {
  struct passwd *creds = getpwuid(uid);
  if (!creds || !creds->pw_name) {
    LOG(WARNING) << "DevToolsServer: can't obtain creds for uid " << uid;
    return false;
  }
  if (gid == uid &&
      (strcmp("root", creds->pw_name) == 0 ||
       strcmp("shell", creds->pw_name) == 0)) {
    return true;
  } else {
    LOG(WARNING) << "DevToolsServer: a connection attempt from " <<
        creds->pw_name;
    return false;
  }
}

DevToolsServer::DevToolsServer()
    : protocol_handler_(NULL) {
}

DevToolsServer::~DevToolsServer() {
  Stop();
}
