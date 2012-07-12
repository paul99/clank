// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/pref_names.h"
#include "chrome/browser/debugger/devtools_window.h"
#include "chrome/browser/extensions/api/webrequest/webrequest_api.h"
#include "chrome/browser/extensions/apps_promo.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_devtools_bridge.h"
#include "chrome/browser/extensions/extension_devtools_manager.h"
#include "chrome/browser/extensions/extension_event_router.h"
#include "chrome/browser/extensions/extension_function_dispatcher.h"
#include "chrome/browser/extensions/extension_info_map.h"
#include "chrome/browser/extensions/extension_message_service.h"
#include "chrome/browser/extensions/extension_process_manager.h"
#include "chrome/browser/extensions/extension_protocols.h"
#include "chrome/browser/extensions/extension_proxy_api.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_web_ui.h"
#include "chrome/browser/extensions/extensions_quota_service.h"
#include "chrome/browser/extensions/image_loading_tracker.h"
#include "chrome/browser/extensions/process_map.h"
#include "chrome/browser/notifications/desktop_notification_service_factory.h"
#include "chrome/browser/notifications/desktop_notification_service.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/browser/omnibox_search_hint.h"
#include "chrome/browser/plugin_prefs.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/sessions/tab_restore_service_delegate.h"
#include "chrome/browser/tabs/pinned_tab_codec.h"
#include "chrome/browser/task_manager/task_manager.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/webui/options/extension_settings_handler.h"
#include "chrome/browser/ui/webui/plugins_ui.h"
#include "chrome/browser/ui/webui/sync_promo/sync_promo_ui.h"
#include "chrome/browser/web_resource/promo_resource_service.h"
#include "content/browser/browser_main.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_error_job.h"

//////////////////////////////////////////////////////////////////////////
// EVERYTHING HERE IS A TEMPORARY BINSIZE HACK
//////////////////////////////////////////////////////////////////////////

// The default theme if we haven't installed a theme yet or if we've clicked
// the "Use Classic" button.
const char* ThemeService::kDefaultThemeID = "";

// static
void AppsPromo::RegisterPrefs(PrefService* local_state) {
  // don't need prefs for things we don't use
}

// static
void AppsPromo::RegisterUserPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use
}

namespace content {

bool ExitedMainMessageLoop() {
  // TODO(merge): fix our browsermain to actually know this
  return false;
}

}

gfx::NativeWindow Browser::BrowserShowHtmlDialog(
    HtmlDialogUIDelegate* delegate,
    gfx::NativeWindow parent_window,
    DialogStyle dialog_style) {
  // only sync setup tries to call this in a case that won't happen.
  NOTIMPLEMENTED();
  return NULL;
}

// static
Browser* Browser::GetBrowserForController(
    const content::NavigationController* controller, int* index) {
  // we do not use Browser.  Need to return null.
  NOTIMPLEMENTED();
  return NULL;
}

int Browser::tab_count() const {
  NOTIMPLEMENTED();
  return 0;
}

int Browser::active_index() const {
  NOTIMPLEMENTED();
  return 0;
}

TabContentsWrapper* Browser::GetSelectedTabContentsWrapper() const {
  // only download manager tries to call this in a case that won't happen.
  NOTIMPLEMENTED();
  return NULL;
}

content::WebContents* Browser::GetWebContentsAt(int index) const {
  NOTIMPLEMENTED();
  return NULL;
}

void Browser::ShowSingletonTab(const GURL& url) {
  NOTIMPLEMENTED();
}

void Browser::OpenPrivacyDashboardTabAndActivate() {
  // we don't have the privacy dashboard.
  NOTIMPLEMENTED();
}

// static
void Browser::RegisterPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
  prefs->RegisterBooleanPref(prefs::kPrintPreviewDisabled,
          true,
          PrefService::UNSYNCABLE_PREF);
}

// static
void Browser::RegisterUserPrefs(PrefService* prefs) {
  // This set of prefs from the real Browser::RegisterUserPrefs needs to be
  // registered for Clank to work; they are used all over the place and there's
  // no clear place to move them to.
  // TODO(torne): find a better way to deal with this
  prefs->RegisterBooleanPref(prefs::kDisable3DAPIs,
                             false,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterBooleanPref(prefs::kEnableHyperlinkAuditing,
                             true,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterBooleanPref(prefs::kEnableReferrers,
                             true,
                             PrefService::UNSYNCABLE_PREF);
  // Initialize the disk cache prefs.
  prefs->RegisterFilePathPref(prefs::kDiskCacheDir,
                              FilePath(),
                              PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kDiskCacheSize,
                             0,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kMediaCacheSize,
                             0,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterBooleanPref(prefs::kEnableMemoryInfo,
                             false,
                             PrefService::UNSYNCABLE_PREF);
}

void Browser::ShowOptionsTab(const std::string& sub_page) {
  // we don't have webui options.
  NOTIMPLEMENTED();
}

// static
void BrowserList::AddObserver(BrowserList::Observer* observer) {
  // used by incognito to check for last incognito window closing.
  NOTIMPLEMENTED();
}

// static
size_t BrowserList::GetBrowserCount(Profile* p) {
  // used by incognito to check for last incognito window closing.
  NOTIMPLEMENTED();
  return 0;
}

Browser* BrowserList::GetLastActive() {
  // used to determine which browser to call various other stubbed methods on.
  NOTIMPLEMENTED();
  return NULL;
}

// static
Browser* BrowserList::GetLastActiveWithProfile(Profile* p) {
  // used to determine which browser to call various other stubbed methods on.
  NOTIMPLEMENTED();
  return NULL;
}

// static
void BrowserList::AttemptRestart() {
  // used to automatically restart chrome when it is shutdown.
  NOTIMPLEMENTED();
}

// static
void BrowserList::RemoveObserver(BrowserList::Observer* observer) {
  // used by incognito to check for last incognito window closing.
  NOTIMPLEMENTED();
}

// static
void BrowserList::CloseAllBrowsersWithProfile(Profile* profile) {
  // used by profile manager to close browsers tied to a specific profile.
  NOTIMPLEMENTED();
}

// static
Browser* BrowserList::FindTabbedBrowser(Profile* profile,
                                        bool match_incognito) {
  NOTIMPLEMENTED();
  return NULL;
}

// static
RenderViewHost* DevToolsWindow::GetRenderViewHost() {
  // we don't have any local devtools windows.
  NOTIMPLEMENTED();
  return NULL;
}

// static
bool DevToolsWindow::IsDevToolsWindow(RenderViewHost* window_rvh) {
  // we don't have any local devtools windows.
  NOTIMPLEMENTED();
  return false;
}

// static
void DevToolsWindow::RegisterUserPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

void extensions::ComponentLoader::RegisterUserPrefs(PrefService*) {
  // don't need prefs for things we don't use.
}

ExtensionDevToolsManager::~ExtensionDevToolsManager() {
  // we don't have extensions.
}

void ExtensionEventRouter::AddEventListener(
    const std::string& event_name,
    content::RenderProcessHost* process,
    const std::string& extension_id) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

void ExtensionEventRouter::RemoveEventListener(
    const std::string& event_name,
    content::RenderProcessHost* process,
    const std::string& extension_id) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

void ExtensionFunctionDispatcher::GetAllFunctionNames(
    std::vector<std::string>* names) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

ExtensionsQuotaService::~ExtensionsQuotaService() {
  // we don't have extensions.
}

bool ExtensionInfoMap::SecurityOriginHasAPIPermission(
    GURL const&,
    int,
    ExtensionAPIPermission::ID) const {
  return true;
}

ExtensionInfoMap::~ExtensionInfoMap() {
  // we don't have extensions.
}

void ExtensionInfoMap::AddExtension(
    const Extension* extension,
    base::Time install_time,
    bool incognito_enabled) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

void ExtensionInfoMap::RemoveExtension(const std::string& extension_id,
                                       const extension_misc::UnloadedExtensionReason reason) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

base::Time ExtensionInfoMap::GetInstallTime(const std::string& extension_id) const {
  NOTIMPLEMENTED();
  return base::Time::Now();
}

bool ExtensionInfoMap::IsIncognitoEnabled(const std::string& extension_id) const {
  NOTIMPLEMENTED();
  return false;
}

bool ExtensionInfoMap::CanCrossIncognito(const Extension* extension) {
  NOTIMPLEMENTED();
  return false;
}

void ExtensionInfoMap::UnregisterExtensionProcess(
    const std::string& extension_id,
    int process_id,
    int site_instance_id) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

void ExtensionInfoMap::RegisterExtensionProcess(
    const std::string& extension_id,
    int process_id,
    int site_instance_id) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

bool extensions::ProcessMap::Insert(
    const std::string& extension_id,
    int process_id,
    int site_instance_id) {
  NOTIMPLEMENTED();
  // we don't have extensions.
  return false;
}

bool extensions::ProcessMap::Remove(
    const std::string& extension_id,
    int process_id,
    int site_instance_id) {
  NOTIMPLEMENTED();
  // we don't have extensions.
  return false;
}

bool extensions::ProcessMap::Contains(int process_id) const {
  NOTIMPLEMENTED();
  // we don't have extensions.
  return false;
}

bool extensions::ProcessMap::Contains(
    const std::string& extension_id,
    int process_id) const {
  NOTIMPLEMENTED();
  // we don't have extensions.
  return false;
}

void ExtensionMessageService::CloseChannel(int port_id) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

void ExtensionMessageService::DestroyingProfile() {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

void ExtensionMessageService::PostMessageFromRenderer(
    int source_port_id, const std::string& message) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

// static
void ExtensionPrefs::RegisterUserPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

// static
ExtensionProcessManager* ExtensionProcessManager::Create(Profile* profile) {
  // we don't have extensions.
  NOTIMPLEMENTED();
  return NULL;
}

// static
ExtensionProxyEventRouter* ExtensionProxyEventRouter::GetInstance() {
  // we don't have extensions.
  NOTIMPLEMENTED();
  return NULL;
}

void ExtensionProxyEventRouter::OnProxyError(
    ExtensionEventRouterForwarder* event_router,
    void* profile,
    int error_code) {
  NOTIMPLEMENTED();
  // we don't have extensions.
}

bool ExtensionService::ExtensionBindingsAllowed(const GURL& url) {
  // we don't have extensions.
  NOTIMPLEMENTED();
  return false;
}

const Extension* ExtensionService::GetInstalledApp(const GURL& url) {
  // we don't have extensions.
  NOTIMPLEMENTED();
  return NULL;
}

bool ExtensionService::IsDownloadFromGallery(const GURL& download_url,
                                             const GURL& referrer_url) {
  // we don't have extensions.
  NOTIMPLEMENTED();
  return false;
}

bool ExtensionService::IsInstalledApp(const GURL& url) {
  // we don't have extensions.
  NOTIMPLEMENTED();
  return false;
}

void ExtensionService::SetInstalledAppForRenderer(int, Extension const*) {
  // we don't have extensions.
  NOTIMPLEMENTED();
}

const Extension* ExtensionService::GetInstalledAppForRenderer(int) {
  // we don't have extensions.
  NOTIMPLEMENTED();
  return NULL;
}

bool ExtensionService::CanLoadInIncognito(const Extension* extension) const {
  // we don't have extensions.
  NOTIMPLEMENTED();
  return false;
}

// static
void ExtensionSettingsHandler::RegisterUserPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

// static
ExtensionWebRequestEventRouter* ExtensionWebRequestEventRouter::GetInstance() {
  // we don't have extensions.
  return NULL;
}

int ExtensionWebRequestEventRouter::OnBeforeRequest(
    void* profile,
    ExtensionInfoMap* info_map,
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  // we don't have extensions.
  return net::OK;
}

void ExtensionWebRequestEventRouter::OnResponseStarted(
    void* profile,
    ExtensionInfoMap* extension_info_map,
    net::URLRequest* request) {
    // we don't have extensions.
}

void ExtensionWebRequestEventRouter::OnBeforeRedirect(
    void* profile,
    ExtensionInfoMap* extension_info_map,
    net::URLRequest* request,
    const GURL& new_location) {
    // we don't have extensions.
}

void ExtensionWebRequestEventRouter::OnSendHeaders (
    void* profile,
    ExtensionInfoMap* extension_info_map,
    net::URLRequest* request,
    const net::HttpRequestHeaders& request_headers) {
    // we don't have extensions.
}

bool ExtensionWebUI::HandleChromeURLOverride(GURL* url,
                                             content::BrowserContext* browser_context) {
  // we don't have extensions.
  return false;
}

bool ExtensionWebUI::HandleChromeURLOverrideReverse(
    GURL* url, content::BrowserContext* browser_context) {
  return false;
}

// static
void ExtensionWebUI::RegisterUserPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

void ExtensionWebUI::GetFaviconForURL(Profile* profile,
                                      FaviconService::GetFaviconRequest* request,
                                      const GURL& page_url) {
  NOTIMPLEMENTED();
}

 void ExtensionWebUI::RegisterChromeURLOverrides(Profile* profile,
                                                 const Extension::URLOverrideMap& overrides) {
   NOTIMPLEMENTED();
 }

 void ExtensionWebUI::UnregisterChromeURLOverrides(Profile* profile,
                                                   const Extension::URLOverrideMap& overrides) {
   NOTIMPLEMENTED();
 }

void ExtensionWebUI::UnregisterChromeURLOverride(const std::string& page,
                                                 Profile* profile,
                                                 base::Value* override) {
  NOTIMPLEMENTED();
}

ImageLoadingTracker::Observer::~Observer() {
  // we don't have extensions.
}

// static
void NotificationUIManager::RegisterPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

OmniboxSearchHint::OmniboxSearchHint(TabContentsWrapper* tab) {
  // we don't hint about omnibox searching.
  NOTIMPLEMENTED();
}

OmniboxSearchHint::~OmniboxSearchHint() {
  // we don't hint about omnibox searching.
  NOTIMPLEMENTED();
}

// static
bool OmniboxSearchHint::IsEnabled(Profile* profile) {
  // we don't hint about omnibox searching.
  return false;
}

void OmniboxSearchHint::Observe(int type,
                                const content::NotificationSource& source,
                                const content::NotificationDetails& details) {
  // we don't hint about omnibox searching.
  NOTIMPLEMENTED();
}

// static
void PinnedTabCodec::RegisterUserPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

// static
PluginPrefs* PluginPrefs::GetForProfile(Profile* profile) {
  return NULL;
}

bool PluginPrefs::IsPluginEnabled(const webkit::WebPluginInfo& plugin) const {
  return true;
}

// static
void PromoResourceService::RegisterPrefs(PrefService* local_state) {
  // don't need prefs for things we don't use.
}

// static
void PromoResourceService::RegisterUserPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

namespace PromoResourceServiceUtil {

bool CanShowPromo(Profile* profile) {
  // don't have apps
  NOTREACHED();
  return false;
}

}

namespace browser {

void ShowHtmlBugReportView(Browser* browser,
                           const std::string& description_template,
                           size_t issue_type) {
  // We do not support bug reports through chrome here.
  NOTIMPLEMENTED();
}

}

SessionService::Handle SessionService::GetLastSession(
    CancelableRequestConsumerBase* consumer,
    const SessionCallback& callback) {
  // we don't store previous sessions this way.
  NOTIMPLEMENTED();
  return 0;
}

// static
SessionService* SessionServiceFactory::GetForProfile(Profile* profile) {
  NOTIMPLEMENTED();
  return 0;
}

// static
void SyncPromoUI::RegisterUserPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

// static
TabRestoreServiceDelegate* TabRestoreServiceDelegate::Create(Profile* profile) {
  // We don't restore tabs using TabRestoreService yet.
  NOTIMPLEMENTED();
  return NULL;
}

// static
TabRestoreServiceDelegate* TabRestoreServiceDelegate::FindDelegateForController(
    const content::NavigationController* controller,
    int* index) {
  // We don't restore tabs using TabRestoreService yet.
  return NULL;
}

// static
TabRestoreServiceDelegate* TabRestoreServiceDelegate::FindDelegateWithID(
    SessionID::id_type desired_id) {
  // We don't restore tabs using TabRestoreService yet.
  NOTIMPLEMENTED();
  return NULL;
}

// static
void TaskManager::RegisterPrefs(PrefService* prefs) {
  // don't need prefs for things we don't use.
}

// static
DesktopNotificationService*
    DesktopNotificationServiceFactory::GetForProfile(Profile* profile) {
  NOTIMPLEMENTED();
  return NULL;
}

// static
DesktopNotificationServiceFactory*
    DesktopNotificationServiceFactory::GetInstance() {
  NOTIMPLEMENTED();
  return NULL;
}

void DesktopNotificationService::RequestPermission(
    const GURL& origin, int process_id, int route_id, int callback_context,
    content::WebContents* tab) {
  NOTIMPLEMENTED();
}

ThemeService* ThemeServiceFactory::GetForProfile(Profile* profile) {
  return NULL;
}

class DummyExtensionProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  virtual net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request) const OVERRIDE;
};

net::URLRequestJob*
DummyExtensionProtocolHandler::MaybeCreateJob(net::URLRequest* request) const {
  LOG(ERROR) << "chrome-extension:// urls are not supported";
  return new net::URLRequestErrorJob(request, net::ERR_ADDRESS_UNREACHABLE);
}

net::URLRequestJobFactory::ProtocolHandler* CreateExtensionProtocolHandler(
    bool is_incognito,
    ExtensionInfoMap* extension_info_map) {
  return new DummyExtensionProtocolHandler();
}
