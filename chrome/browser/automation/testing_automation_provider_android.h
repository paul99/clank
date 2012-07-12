// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOMATION_TESTING_AUTOMATION_PROVIDER_ANDROID_H_
#define CHROME_BROWSER_AUTOMATION_TESTING_AUTOMATION_PROVIDER_ANDROID_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/automation/automation_provider.h"
#include "chrome/browser/automation/automation_resource_tracker.h"
#include "webkit/glue/window_open_disposition.h"

class PrefService;
class TabContentsWrapper;

namespace content {
class WebContents;
}

// This is an automation provider containing testing calls.
class TestingAutomationProvider : public AutomationProvider {
 public:
  explicit TestingAutomationProvider(Profile* profile);

  virtual IPC::Channel::Mode GetChannelMode(bool use_named_interface);

  // IPC::Channel::Listener:
  virtual bool OnMessageReceived(const IPC::Message& msg) OVERRIDE;

  // Static methods for the ChromeView to inform us the status change of tabs.
  static void OnTabAdded(TabContentsWrapper*, bool incognito);
  static void OnTabRemoved(TabContentsWrapper*);
  static void OnTabActivated(int active_tab_index, bool incognito);
  static void OnTabReplaced(TabContentsWrapper* old_tab,
                            TabContentsWrapper* new_tab);

 private:
  virtual ~TestingAutomationProvider();

  // IPC Message callbacks.
  void ActivateTab(int handle, int at_index, int* status);
  void AppendTab(int handle, const GURL& url, IPC::Message* reply_message);
  void GetActiveTabIndex(int handle, int* active_tab_index);
  void CloseTab(int tab_handle, bool wait_until_closed,
                IPC::Message* reply_message);
  void GetCookies(const GURL& url, int handle, int* value_size,
                  std::string* value);
  void SetCookie(const GURL& url,
                 const std::string& value,
                 int handle,
                 int* response_value);
  void DeleteCookie(const GURL& url, const std::string& cookie_name,
                    int handle, bool* success);
  void NavigateToURLBlockUntilNavigationsComplete(int handle, const GURL& url,
                                                  int number_of_navigations,
                                                  IPC::Message* reply_message);
  void NavigationAsync(int handle, const GURL& url, bool* status);
  void NavigationAsyncWithDisposition(int handle,
                                      const GURL& url,
                                      WindowOpenDisposition disposition,
                                      bool* status);
  void Reload(int handle, IPC::Message* reply_message);
  void GetBrowserWindowCount(int* window_count);
  void GetNormalBrowserWindowCount(int* window_count);
  // Be aware that the browser window returned might be of non TYPE_NORMAL
  // or in incognito mode.
  void GetBrowserWindow(int index, int* handle);
  void FindTabbedBrowserWindow(int* handle);
  void GetLastActiveBrowserWindow(int* handle);
  void GetBrowserLocale(string16* locale);
  void GetTabCount(int handle, int* tab_count);
  void GetType(int handle, int* type_as_int);
  void GetTab(int handle, int tab_index, int* tab_handle);
  void GetTabProcessID(int handle, int* process_id);
  void GetTabTitle(int handle, int* title_string_size, std::wstring* title);
  void GetTabIndex(int handle, int* tabstrip_index);
  void GetTabURL(int handle, bool* success, GURL* url);

  void ExecuteJavascript(int handle,
                         const std::wstring& frame_xpath,
                         const std::wstring& script,
                         IPC::Message* reply_message);

  // Retrieves a Browser from a Window and vice-versa.
  void GetWindowForBrowser(int browser_handle, bool* success,
                           int* window_handle);
  void GetBrowserForWindow(int window_handle, bool* success,
                           int* browser_handle);

  // Retrieves the last time a navigation occurred for the tab.
  void GetLastNavigationTime(int handle, int64* last_navigation_time);

  // Utility function for the following preference accessing functions.
  PrefService* GetBrowserPrefs(int browser_handle);

  // Sets the int value for preference with name |name|.
  void SetIntPreference(int handle,
                        const std::string& name,
                        int value,
                        bool* success);

  // Sets the string value for preference with name |name|.
  void SetStringPreference(int handle,
                           const std::string& name,
                           const std::string& value,
                           bool* success);

  // Gets the bool value for preference with name |name|.
  void GetBooleanPreference(int handle,
                            const std::string& name,
                            bool* success,
                            bool* value);

  // Sets the bool value for preference with name |name|.
  void SetBooleanPreference(int handle,
                            const std::string& name,
                            bool value,
                            bool* success);

  // Gets the browser that contains the given tab.
  void GetParentBrowserOfTab(
      int tab_handle, int* browser_handle, bool* success);

  // Returns true if the given tab is crashed.
  bool IsTabCrashed(content::WebContents* contents);

  static bool enabled_;

  // Records the status of all tabs of a window/browser.
  class TabRecorder {
   public:
    TabRecorder();

    bool OnTabAdded(TabContentsWrapper*);
    bool OnTabRemoved(TabContentsWrapper*);
    bool OnTabActivated(int active_tab_index);
    bool OnTabReplaced(TabContentsWrapper* old_tab,
                       TabContentsWrapper* new_tab);

    int GetTabCount() const;
    int GetActiveTabIndex() const;
    int GetIndexOfTabContents(content::WebContents* contents) const;
    content::WebContents* GetTabContentsAt(int index) const;
    TabContentsWrapper* GetTabContentsWrapperAt(int index) const;
    content::WebContents* GetActiveTabContents() const;
    TabContentsWrapper* GetActiveTabContentsWrapper() const;

   private:
    typedef std::vector<TabContentsWrapper*> TabList;
    TabList tabs_;
    unsigned int active_tab_index_;

    DISALLOW_COPY_AND_ASSIGN(TabRecorder);
  };

  class AutomationTabRecorderTracker
      : public AutomationResourceTracker<TabRecorder*> {
   public:
    explicit AutomationTabRecorderTracker(IPC::Message::Sender* automation)
        : AutomationResourceTracker<TabRecorder*>(automation) {}
    virtual ~AutomationTabRecorderTracker() {};
    // Chrome has two tab lists to track normal tabs and incognito tabs
    // on Android, there are not real browser/window, so we don't need to
    // observe the close notification for them.
    virtual void AddObserver(TabRecorder* tab_recorder) {}
    virtual void RemoveObserver(TabRecorder* tab_recorder) {}
  };

  scoped_ptr<AutomationTabRecorderTracker> tab_recorder_tracker_;

  // We have only one normal window/browser and one incognito window/browser.
  static int normal_browser_handle_;
  static int incognito_browser_handle_;
  static TabRecorder normal_tab_recorder_;
  static TabRecorder incognito_tab_recorder_;

  DISALLOW_COPY_AND_ASSIGN(TestingAutomationProvider);
};

#endif  // CHROME_BROWSER_AUTOMATION_TESTING_AUTOMATION_PROVIDER_ANDROID_H_
