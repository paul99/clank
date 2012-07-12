// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/automation/testing_automation_provider_android.h"

#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/automation/automation_provider_observers.h"
#include "chrome/browser/automation/automation_tab_tracker.h"
#include "chrome/browser/automation/automation_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/automation_messages.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/common/referrer.h"

using content::WebContents;

// We have only one normal window/browser and one incognito window/browser.
int TestingAutomationProvider::normal_browser_handle_ = 0;
int TestingAutomationProvider::incognito_browser_handle_ = 0;

bool TestingAutomationProvider::enabled_ = false;

// See http://b/issue?id=5861833
#if defined(__clang__)
#define DEFINE_STATIC_GLOBAL(type, name) \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") \
    _Pragma("clang diagnostic ignored \"-Wexit-time-destructors\"") \
    type name; \
    _Pragma("clang diagnostic pop")
#else
#define DEFINE_STATIC_GLOBAL(type, name) \
    type name;
#endif  // __clang__

DEFINE_STATIC_GLOBAL(TestingAutomationProvider::TabRecorder,
    TestingAutomationProvider::normal_tab_recorder_);
DEFINE_STATIC_GLOBAL(TestingAutomationProvider::TabRecorder,
    TestingAutomationProvider::incognito_tab_recorder_);

TestingAutomationProvider::TestingAutomationProvider(Profile* profile)
    : AutomationProvider(profile) {
  tab_recorder_tracker_.reset(new AutomationTabRecorderTracker(this));
  tab_recorder_tracker_->Add(&normal_tab_recorder_);
  tab_recorder_tracker_->Add(&incognito_tab_recorder_);
  normal_browser_handle_ =
      tab_recorder_tracker_->GetHandle(&normal_tab_recorder_);
  incognito_browser_handle_ =
      tab_recorder_tracker_->GetHandle(&incognito_tab_recorder_);
  DCHECK(normal_browser_handle_ > 0 && incognito_browser_handle_ > 0 &&
         normal_browser_handle_ != incognito_browser_handle_);
  enabled_ = true;
}

TestingAutomationProvider::~TestingAutomationProvider() {
}

IPC::Channel::Mode TestingAutomationProvider::GetChannelMode(
    bool use_named_interface) {
  if (use_named_interface)
#if defined(OS_POSIX)
    return IPC::Channel::MODE_OPEN_NAMED_SERVER;
#else
    return IPC::Channel::MODE_NAMED_SERVER;
#endif
  else
    return IPC::Channel::MODE_CLIENT;
}

// static
void TestingAutomationProvider::OnTabAdded(TabContentsWrapper* tab_contents,
                                           bool incognito) {
  if (!enabled_)
    return;

  if (!(incognito ? incognito_tab_recorder_ : normal_tab_recorder_)
      .OnTabAdded(tab_contents)) {
    LOG(ERROR) << "Trying to add a tab that already exists: "
               << tab_contents->web_contents()->GetURL().spec();
  }
}

// static
void TestingAutomationProvider::OnTabRemoved(TabContentsWrapper* tab_contents) {
  if (!enabled_)
    return;

  if (!normal_tab_recorder_.OnTabRemoved(tab_contents) &&
      !incognito_tab_recorder_.OnTabRemoved(tab_contents)) {
    LOG(ERROR) << "Trying to remove a tab that doesn't exist: "
               << tab_contents->web_contents()->GetURL().spec();
  }
}

// static
void TestingAutomationProvider::OnTabActivated(int active_tab_index,
                                               bool incognito) {
  if (!enabled_)
    return;

  if (!(incognito ? incognito_tab_recorder_ : normal_tab_recorder_)
      .OnTabActivated(active_tab_index))
    LOG(ERROR) << "Trying to activate an invalid tab: " << active_tab_index;
}

// static
void TestingAutomationProvider::OnTabReplaced(TabContentsWrapper* old_tab,
                                              TabContentsWrapper* new_tab) {
  if (!enabled_)
    return;

  if (!normal_tab_recorder_.OnTabReplaced(old_tab, new_tab) &&
      !incognito_tab_recorder_.OnTabReplaced(old_tab, new_tab))
    LOG(ERROR) << "Try to replace a tab which is not in tab list";
}

bool TestingAutomationProvider::IsTabCrashed(WebContents* contents) {
  // We can not rely on TabContents::is_crashed() because currently
  // renderer service process can be asked to quit normally on Android.
  // So in here we consider tab as crashed as long as the crashed_status
  // is less than TERMINATION_STATUS_STILL_RUNNING.
  return (contents->GetCrashedStatus() <
          base::TERMINATION_STATUS_STILL_RUNNING);
}

bool TestingAutomationProvider::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(TestingAutomationProvider, message)
    IPC_MESSAGE_HANDLER(AutomationMsg_ActivateTab, ActivateTab)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AutomationMsg_AppendTab, AppendTab)
    IPC_MESSAGE_HANDLER(AutomationMsg_ActiveTabIndex, GetActiveTabIndex)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AutomationMsg_CloseTab, CloseTab)
    IPC_MESSAGE_HANDLER(AutomationMsg_GetCookies, GetCookies)
    IPC_MESSAGE_HANDLER(AutomationMsg_SetCookie, SetCookie)
    IPC_MESSAGE_HANDLER(AutomationMsg_DeleteCookie, DeleteCookie)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(
        AutomationMsg_NavigateToURLBlockUntilNavigationsComplete,
        NavigateToURLBlockUntilNavigationsComplete)
    IPC_MESSAGE_HANDLER(AutomationMsg_NavigationAsync, NavigationAsync)
    IPC_MESSAGE_HANDLER(AutomationMsg_NavigationAsyncWithDisposition,
                        NavigationAsyncWithDisposition)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AutomationMsg_Reload, Reload)
    IPC_MESSAGE_HANDLER(AutomationMsg_BrowserWindowCount, GetBrowserWindowCount)
    IPC_MESSAGE_HANDLER(AutomationMsg_NormalBrowserWindowCount,
                        GetNormalBrowserWindowCount)
    IPC_MESSAGE_HANDLER(AutomationMsg_BrowserWindow, GetBrowserWindow)
    IPC_MESSAGE_HANDLER(AutomationMsg_GetBrowserLocale, GetBrowserLocale)
    IPC_MESSAGE_HANDLER(AutomationMsg_FindTabbedBrowserWindow,
                        FindTabbedBrowserWindow)
    IPC_MESSAGE_HANDLER(AutomationMsg_TabCount, GetTabCount)
    IPC_MESSAGE_HANDLER(AutomationMsg_Type, GetType)
    IPC_MESSAGE_HANDLER(AutomationMsg_Tab, GetTab)
    IPC_MESSAGE_HANDLER(AutomationMsg_TabProcessID, GetTabProcessID)
    IPC_MESSAGE_HANDLER(AutomationMsg_TabTitle, GetTabTitle)
    IPC_MESSAGE_HANDLER(AutomationMsg_TabIndex, GetTabIndex)
    IPC_MESSAGE_HANDLER(AutomationMsg_TabURL, GetTabURL)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AutomationMsg_DomOperation,
                                    ExecuteJavascript)
    IPC_MESSAGE_HANDLER(AutomationMsg_WindowForBrowser, GetWindowForBrowser)
    IPC_MESSAGE_HANDLER(AutomationMsg_BrowserForWindow, GetBrowserForWindow)
    IPC_MESSAGE_HANDLER(AutomationMsg_GetLastNavigationTime,
                        GetLastNavigationTime)
    IPC_MESSAGE_HANDLER(AutomationMsg_SetIntPreference, SetIntPreference)
    IPC_MESSAGE_HANDLER(AutomationMsg_SetStringPreference, SetStringPreference)
    IPC_MESSAGE_HANDLER(AutomationMsg_GetBooleanPreference,
                        GetBooleanPreference)
    IPC_MESSAGE_HANDLER(AutomationMsg_SetBooleanPreference,
                        SetBooleanPreference)
    IPC_MESSAGE_HANDLER(AutomationMsg_GetParentBrowserOfTab,
                        GetParentBrowserOfTab)

    IPC_MESSAGE_UNHANDLED(
        handled = AutomationProvider::OnMessageReceived(message))
  IPC_END_MESSAGE_MAP()
  return handled;
}

void TestingAutomationProvider::ActivateTab(int handle,
                                            int at_index,
                                            int* status) {
  if (at_index >= 0) {
    TabRecorder* tab_recorder = tab_recorder_tracker_->GetResource(handle);
    // Return success if at_index equals to the current active tab index.
    // TODO(jnd): May remove these lines when we implement the following TODO.
    if (tab_recorder && tab_recorder->GetActiveTabIndex() == at_index) {
      *status = 0;
      return;
    }
  }
  *status = -1;
  // TODO(jnd): find a way to notify java UI to activate the tab.
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::AppendTab(int handle,
                                          const GURL& url,
                                          IPC::Message* reply_message) {
  // TODO(jnd): find a way to notify java UI to append a tab.
  AutomationMsg_AppendTab::WriteReplyParams(reply_message, -1);
  Send(reply_message);
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::GetActiveTabIndex(int handle,
                                                  int* active_tab_index) {
  TabRecorder* tab_recorder = tab_recorder_tracker_->GetResource(handle);
  *active_tab_index = tab_recorder ? tab_recorder->GetActiveTabIndex() : -1;
}

void TestingAutomationProvider::CloseTab(int tab_handle,
                                         bool wait_until_closed,
                                         IPC::Message* reply_message) {
  if (tab_tracker_->ContainsHandle(tab_handle)) {
    content::NavigationController* controller = tab_tracker_->GetResource(tab_handle);
    WebContents* contents = controller->GetWebContents();
    new TabClosedNotificationObserver(this, wait_until_closed, reply_message);
    contents->GetDelegate()->CloseContents(contents);
    return;
  }

  AutomationMsg_CloseTab::WriteReplyParams(reply_message, false);
  Send(reply_message);
}

void TestingAutomationProvider::GetCookies(const GURL& url, int handle,
                                           int* value_size,
                                           std::string* value) {
  WebContents* contents = tab_tracker_->ContainsHandle(handle) ?
      tab_tracker_->GetResource(handle)->GetWebContents() : NULL;
  if (IsTabCrashed(contents)) {
    LOG(ERROR) << "Tab is crashed when getting cookies.";
    contents = NULL;
  }
  automation_util::GetCookies(url, contents, value_size, value);
}

void TestingAutomationProvider::SetCookie(const GURL& url,
                                          const std::string& value,
                                          int handle,
                                          int* response_value) {
  WebContents* contents = tab_tracker_->ContainsHandle(handle) ?
      tab_tracker_->GetResource(handle)->GetWebContents() : NULL;
  if (IsTabCrashed(contents)) {
    LOG(ERROR) << "Tab is crashed when setting cookies.";
    contents = NULL;
  }
  automation_util::SetCookie(url, value, contents, response_value);
}

void TestingAutomationProvider::DeleteCookie(const GURL& url,
                                             const std::string& cookie_name,
                                             int handle, bool* success) {
  WebContents* contents = tab_tracker_->ContainsHandle(handle) ?
      tab_tracker_->GetResource(handle)->GetWebContents() : NULL;
  if (IsTabCrashed(contents)) {
    LOG(ERROR) << "Tab is crashed when deleting cookie.";
    contents = NULL;
  }
  automation_util::DeleteCookie(url, cookie_name, contents, success);
}

void TestingAutomationProvider::NavigateToURLBlockUntilNavigationsComplete(
    int handle, const GURL& url, int number_of_navigations,
    IPC::Message* reply_message) {
  if (tab_tracker_->ContainsHandle(handle)) {
    content::NavigationController* tab = tab_tracker_->GetResource(handle);
    // TODO(jnd): find a way to notify java UI to activate the tab.
    new NavigationNotificationObserver(tab, this, reply_message,
                                       number_of_navigations, false, false);
    tab->LoadURL(url, content::Referrer(), content::PAGE_TRANSITION_TYPED,
                 std::string());
    return;
  }

  AutomationMsg_NavigateToURLBlockUntilNavigationsComplete::WriteReplyParams(
      reply_message, AUTOMATION_MSG_NAVIGATION_ERROR);
  Send(reply_message);
}

void TestingAutomationProvider::NavigationAsync(int handle,
                                                const GURL& url,
                                                bool* status) {
  NavigationAsyncWithDisposition(handle, url, CURRENT_TAB, status);
}

void TestingAutomationProvider::NavigationAsyncWithDisposition(
    int handle,
    const GURL& url,
    WindowOpenDisposition disposition,
    bool* status) {
  *status = false;

  if (tab_tracker_->ContainsHandle(handle)) {
    content::NavigationController* tab = tab_tracker_->GetResource(handle);
    tab->LoadURL(url, content::Referrer(), content::PAGE_TRANSITION_TYPED,
                 std::string());
    *status = true;
  }
}

void TestingAutomationProvider::Reload(int handle,
                                       IPC::Message* reply_message) {
  // TODO(jnd): find a way to notify java UI to reload a tab.
  AutomationMsg_Reload::WriteReplyParams(
      reply_message, AUTOMATION_MSG_NAVIGATION_ERROR);
  Send(reply_message);
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::GetBrowserWindowCount(int* window_count) {
  *window_count = incognito_tab_recorder_.GetTabCount() ? 2 : 1;
}

void TestingAutomationProvider::GetNormalBrowserWindowCount(int* window_count) {
  *window_count = 1;
}

void TestingAutomationProvider::GetBrowserWindow(int index, int* handle) {
  *handle = index == 0 ? normal_browser_handle_ :
            index == 1 ? incognito_browser_handle_ : 0;
}

void TestingAutomationProvider::FindTabbedBrowserWindow(int* handle) {
  *handle = normal_browser_handle_;
}

void TestingAutomationProvider::GetBrowserLocale(string16* locale) {
  *locale = ASCIIToUTF16(g_browser_process->GetApplicationLocale());
}

void TestingAutomationProvider::GetTabCount(int handle, int* tab_count) {
  TabRecorder* tab_recorder = tab_recorder_tracker_->GetResource(handle);
  *tab_count = tab_recorder ? tab_recorder->GetTabCount() : -1;
}

void TestingAutomationProvider::GetType(int handle, int* type_as_int) {
  *type_as_int = -1;  // -1 is the error code
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::GetTab(int handle,
                                       int tab_index,
                                       int* tab_handle) {
  TabRecorder* tab_recorder = tab_recorder_tracker_->GetResource(handle);
  WebContents* contents =
      tab_recorder ? tab_recorder->GetTabContentsAt(tab_index) : NULL;
  *tab_handle =
      contents ? tab_tracker_->Add(&contents->GetController()) : 0;
}

void TestingAutomationProvider::GetTabProcessID(int handle, int* process_id) {
  *process_id = -1;
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::GetTabTitle(int handle,
                                            int* title_string_size,
                                            std::wstring* title) {
  *title_string_size = -1;  // -1 is the error code
  if (tab_tracker_->ContainsHandle(handle)) {
    content::NavigationController* tab = tab_tracker_->GetResource(handle);
    content::NavigationEntry* entry = tab->GetActiveEntry();
    if (entry != NULL) {
      *title = UTF16ToWideHack(entry->GetTitleForDisplay(""));
    } else {
      *title = std::wstring();
    }
    *title_string_size = static_cast<int>(title->size());
  }
}

void TestingAutomationProvider::GetTabIndex(int handle, int* tabstrip_index) {
  *tabstrip_index = -1;  // -1 is the error code
  if (tab_tracker_->ContainsHandle(handle)) {
    WebContents* contents =
        tab_tracker_->GetResource(handle)->GetWebContents();
    *tabstrip_index = normal_tab_recorder_.GetIndexOfTabContents(contents);
    if (*tabstrip_index == -1) {
      *tabstrip_index = incognito_tab_recorder_.GetIndexOfTabContents(
          contents);
    }
  }
}

void TestingAutomationProvider::GetTabURL(int handle,
                                          bool* success,
                                          GURL* url) {
  *success = false;
  if (tab_tracker_->ContainsHandle(handle)) {
    content::NavigationController* tab = tab_tracker_->GetResource(handle);
    // Return what the user would see in the location bar.
    *url = tab->GetActiveEntry()->GetVirtualURL();
    *success = true;
  }
}

void TestingAutomationProvider::ExecuteJavascript(
    int handle,
    const std::wstring& frame_xpath,
    const std::wstring& script,
    IPC::Message* reply_message) {
  content::WebContents* web_contents = GetWebContentsForHandle(handle, NULL);
  if (!web_contents) {
    AutomationMsg_DomOperation::WriteReplyParams(reply_message, std::string());
    Send(reply_message);
    return;
  }

  // Set the routing id of this message with the controller.
  // This routing id needs to be remembered for the reverse
  // communication while sending back the response of
  // this javascript execution.
  std::string set_automation_id;
  base::SStringPrintf(&set_automation_id,
                      "window.domAutomationController.setAutomationId(%d);",
                      reply_message->routing_id());

  new DomOperationMessageSender(this, reply_message, false);
  web_contents->GetRenderViewHost()->ExecuteJavascriptInWebFrame(
      WideToUTF16Hack(frame_xpath), UTF8ToUTF16(set_automation_id));
  web_contents->GetRenderViewHost()->ExecuteJavascriptInWebFrame(
      WideToUTF16Hack(frame_xpath), WideToUTF16Hack(script));
}

void TestingAutomationProvider::GetWindowForBrowser(int browser_handle,
                                                    bool* success,
                                                    int* window_handle) {
  // We treat browser handles the same as window handles.
  *window_handle = browser_handle;
  *success = true;
}

void TestingAutomationProvider::GetBrowserForWindow(int window_handle,
                                                    bool* success,
                                                    int* browser_handle) {
  // We treat browser handles the same as window handles.
  *browser_handle = window_handle;
  *success = true;
}

void TestingAutomationProvider::GetLastNavigationTime(
    int handle,
    int64* last_navigation_time) {
  base::Time time(tab_tracker_->GetLastNavigationTime(handle));
  *last_navigation_time = time.ToInternalValue();
}

PrefService* TestingAutomationProvider::GetBrowserPrefs(int browser_handle) {
  if (normal_browser_handle_ == browser_handle)
    return profile_->GetPrefs();
  if (incognito_browser_handle_ == browser_handle)
    return profile_->GetOffTheRecordProfile()->GetPrefs();
  return NULL;
}

void TestingAutomationProvider::SetIntPreference(int handle,
                                                 const std::string& name,
                                                 int value,
                                                 bool* success) {
  *success = false;
  PrefService* prefs = GetBrowserPrefs(handle);
  if (prefs) {
    prefs->SetInteger(name.c_str(), value);
    *success = true;
  }
}

void TestingAutomationProvider::SetStringPreference(int handle,
                                                    const std::string& name,
                                                    const std::string& value,
                                                    bool* success) {
  *success = false;
  PrefService* prefs = GetBrowserPrefs(handle);
  if (prefs) {
    prefs->SetString(name.c_str(), value);
    *success = true;
  }
}

void TestingAutomationProvider::GetBooleanPreference(int handle,
                                                     const std::string& name,
                                                     bool* success,
                                                     bool* value) {
  *success = false;
  PrefService* prefs = GetBrowserPrefs(handle);
  if (prefs) {
    *value = prefs->GetBoolean(name.c_str());
    *success = true;
  }
}

void TestingAutomationProvider::SetBooleanPreference(int handle,
                                                     const std::string& name,
                                                     bool value,
                                                     bool* success) {
  *success = false;
  PrefService* prefs = GetBrowserPrefs(handle);
  if (prefs) {
    prefs->SetBoolean(name.c_str(), value);
    *success = true;
  }
}

void TestingAutomationProvider::GetParentBrowserOfTab(int tab_handle,
                                                      int* browser_handle,
                                                      bool* success) {
  *browser_handle = 0;
  *success = false;

  if (tab_tracker_->ContainsHandle(tab_handle)) {
    WebContents* contents =
        tab_tracker_->GetResource(tab_handle)->GetWebContents();
    if (normal_tab_recorder_.GetIndexOfTabContents(contents) != -1) {
      *success = true;
      *browser_handle = normal_browser_handle_;
    } else if (incognito_tab_recorder_.GetIndexOfTabContents(contents) != -1) {
      *success = true;
      *browser_handle = incognito_browser_handle_;
    }
  }
}

TestingAutomationProvider::TabRecorder::TabRecorder()
    : active_tab_index_(0) {
}

bool TestingAutomationProvider::TabRecorder::OnTabAdded(
    TabContentsWrapper* tab_contents) {
  TabList::iterator iter = std::find(tabs_.begin(), tabs_.end(), tab_contents);
  if (iter != tabs_.end())
    return false;
  tabs_.push_back(tab_contents);
  return true;
}

bool TestingAutomationProvider::TabRecorder::OnTabRemoved(
    TabContentsWrapper* tab_contents) {
  TabList::iterator iter = std::find(tabs_.begin(), tabs_.end(), tab_contents);
  if (iter == tabs_.end())
    return false;
  tabs_.erase(iter);
  return true;
}

bool TestingAutomationProvider::TabRecorder::OnTabActivated(
    int active_tab_index) {
  unsigned uindex = static_cast<unsigned int>(active_tab_index);
  if (uindex >= tabs_.size())
    return false;
  active_tab_index_ = uindex;
  return true;
}

bool TestingAutomationProvider::TabRecorder::OnTabReplaced(
    TabContentsWrapper* old_tab, TabContentsWrapper* new_tab) {
  TabList::iterator iter = std::find(tabs_.begin(), tabs_.end(), old_tab);
  if (iter == tabs_.end())
    return false;
  *iter = new_tab;
  return true;
}

int TestingAutomationProvider::TabRecorder::GetTabCount() const {
  return tabs_.size();
}

int TestingAutomationProvider::TabRecorder::GetActiveTabIndex() const {
  return active_tab_index_ < tabs_.size() ? active_tab_index_ : -1;
}

int TestingAutomationProvider::TabRecorder::GetIndexOfTabContents(
    WebContents* contents) const {
  for (size_t i = 0; i < tabs_.size(); i++) {
    if (tabs_[i]->web_contents() == contents)
      return i;
  }
  return -1;
}

WebContents* TestingAutomationProvider::TabRecorder::GetTabContentsAt(
    int index) const {
  unsigned uindex = static_cast<unsigned int>(index);
  return uindex < tabs_.size() ? tabs_[uindex]->web_contents() : NULL;
}

TabContentsWrapper*
TestingAutomationProvider::TabRecorder::GetTabContentsWrapperAt(
    int index) const {
  unsigned uindex = static_cast<unsigned int>(index);
  return uindex < tabs_.size() ? tabs_[uindex] : NULL;
}

WebContents*
TestingAutomationProvider::TabRecorder::GetActiveTabContents() const {
  return active_tab_index_ < tabs_.size() ?
      tabs_[active_tab_index_]->web_contents() : NULL;
}

TabContentsWrapper*
TestingAutomationProvider::TabRecorder::GetActiveTabContentsWrapper() const {
  return active_tab_index_ < tabs_.size() ? tabs_[active_tab_index_] : NULL;
}
