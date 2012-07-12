// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A sqlite implementation of a cookie monster persistent store.

#ifndef CHROME_BROWSER_NET_SQLITE_PERSISTENT_COOKIE_STORE_H_
#define CHROME_BROWSER_NET_SQLITE_PERSISTENT_COOKIE_STORE_H_
#pragma once

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#if defined(OS_ANDROID)
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#endif
#include "net/base/cookie_monster.h"

class FilePath;
class Task;

// Implements the PersistentCookieStore interface in terms of a SQLite database.
// For documentation about the actual member functions consult the documentation
// of the parent class |net::CookieMonster::PersistentCookieStore|.
class SQLitePersistentCookieStore
    : public net::CookieMonster::PersistentCookieStore
#if defined(OS_ANDROID)
    , public content::NotificationObserver
#endif
{
 public:
  SQLitePersistentCookieStore(const FilePath& path,
                              bool restore_old_session_cookies);

#if defined(OS_ANDROID)
  // Call this before Load() to prevent old session cookies being cleared out.
  void SetRestoreSessionOnLoad(bool restore_session);
#endif

  virtual ~SQLitePersistentCookieStore();

  virtual void Load(const LoadedCallback& loaded_callback) OVERRIDE;

  virtual void LoadCookiesForKey(const std::string& key,
      const LoadedCallback& callback) OVERRIDE;

  virtual void AddCookie(
      const net::CookieMonster::CanonicalCookie& cc) OVERRIDE;

  virtual void UpdateCookieAccessTime(
      const net::CookieMonster::CanonicalCookie& cc) OVERRIDE;

  virtual void DeleteCookie(
      const net::CookieMonster::CanonicalCookie& cc) OVERRIDE;

  virtual void SetClearLocalStateOnExit(bool clear_local_state) OVERRIDE;

  virtual void Flush(const base::Closure& callback) OVERRIDE;

 private:
#if defined(OS_ANDROID)
  // NotificationObserver method. We observe FLUSH_START and FLUSH_FINISH.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // We run this on the UI thread, as that's where flush notifications are sent.
  void RegisterForNotifications();

  content::NotificationRegistrar notification_registrar_;
#endif

  class Backend;

  scoped_refptr<Backend> backend_;

  DISALLOW_COPY_AND_ASSIGN(SQLitePersistentCookieStore);
};

#endif  // CHROME_BROWSER_NET_SQLITE_PERSISTENT_COOKIE_STORE_H_
