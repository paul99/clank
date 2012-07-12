// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_TAB_MODEL_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_TAB_MODEL_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "chrome/browser/sessions/session_id.h"
#include "chrome/browser/sync/glue/synced_window_delegate.h"
#include "content/browser/android/jni_helper.h"

class ChromeView;
class SyncedTabDelegate;
class Tab;
namespace browser_sync {
class SyncedTabDelegate;
class BrowserWindow;
}

// Native representation of ChromeViewListAdapter$ChromeTabModel which provides
// access to information about a tabstrip to native code and could potentially
// be used in place of Browser for some functionality in Clank.
class TabModel : public browser_sync::SyncedWindowDelegate {
 public:
  TabModel(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv* env, jobject obj);

  int GetWindowId(JNIEnv* env, jobject obj);

  // SyncedWindowDelegate:
  virtual bool HasWindow() const OVERRIDE;
  virtual SessionID::id_type GetSessionId() const OVERRIDE;
  virtual int GetTabCount() const OVERRIDE;
  virtual int GetActiveIndex() const OVERRIDE;
  virtual bool IsApp() const OVERRIDE;
  virtual bool IsTypeTabbed() const OVERRIDE;
  virtual bool IsTypePopup() const OVERRIDE;
  virtual bool IsTabPinned(
      const browser_sync::SyncedTabDelegate* tab) const OVERRIDE;
  virtual browser_sync::SyncedTabDelegate* GetTabAt(int index) const OVERRIDE;
  virtual SessionID::id_type GetTabIdAt(int index) const OVERRIDE;

  virtual bool is_incognito() const;

  // Used for restoring tabs from synced foreign sessions.
  virtual void CreateTab(content::WebContents* web_contents) const;

  // Returns true when all tabs have been restored from disk.
  virtual bool AreAllTabsLoaded() const OVERRIDE;

  // Instructs the TabModel to broadcast a notification that all tabs are now loaded from storage.
  virtual void BroadcastAllTabsLoaded(JNIEnv* env, jobject obj) const;

  virtual void OpenClearBrowsingData() const;

 private:
  virtual ~TabModel();

  virtual Tab* GetNativeTab(int index) const;

  // Unique identifier of this TabModel for session restore. This id is only
  // unique within the current session, and is not guaranteed to be unique
  // across sessions.
  SessionID session_id_;

  struct JavaObject;
  JavaObject* java_object_;
};

// Register the Tab's native methods through jni.
bool RegisterTabModel(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_TAB_MODEL_H_
