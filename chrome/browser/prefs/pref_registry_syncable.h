// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_PREF_REGISTRY_SYNCABLE_H_
#define CHROME_BROWSER_PREFS_PREF_REGISTRY_SYNCABLE_H_

#include <set>
#include <string>

#include "base/prefs/pref_registry.h"

namespace base {
class DictionaryValue;
class FilePath;
class ListValue;
class Value;
}

// A PrefRegistry that forces users to choose whether each registered
// preference is syncable or not.
class PrefRegistrySyncable : public PrefRegistry {
 public:
  typedef base::Callback<void(const char* path)> SyncableRegistrationCallback;

  // Enum used when registering preferences to determine if it should
  // be synced or not.
  enum PrefSyncStatus {
    UNSYNCABLE_PREF,
    SYNCABLE_PREF
  };

  PrefRegistrySyncable();

  // Retrieve the set of syncable preferences currently registered.
  const std::set<std::string>& syncable_preferences() const;

  // Exactly one callback can be set for the event of a syncable
  // preference being registered. It will be fired after the
  // registration has occurred.
  //
  // Calling this method after a callback has already been set will
  // make the object forget the previous callback and use the new one
  // instead.
  void SetSyncableRegistrationCallback(const SyncableRegistrationCallback& cb);

  void RegisterBooleanPref(const char* path,
                           bool default_value,
                           PrefSyncStatus sync_status);
  void RegisterIntegerPref(const char* path,
                           int default_value,
                           PrefSyncStatus sync_status);
  void RegisterDoublePref(const char* path,
                          double default_value,
                          PrefSyncStatus sync_status);
  void RegisterStringPref(const char* path,
                          const std::string& default_value,
                          PrefSyncStatus sync_status);
  void RegisterFilePathPref(const char* path,
                            const base::FilePath& default_value,
                            PrefSyncStatus sync_status);
  void RegisterListPref(const char* path,
                        PrefSyncStatus sync_status);
  void RegisterDictionaryPref(const char* path,
                              PrefSyncStatus sync_status);
  void RegisterListPref(const char* path,
                        base::ListValue* default_value,
                        PrefSyncStatus sync_status);
  void RegisterDictionaryPref(const char* path,
                              base::DictionaryValue* default_value,
                              PrefSyncStatus sync_status);
  void RegisterLocalizedBooleanPref(const char* path,
                                    int locale_default_message_id,
                                    PrefSyncStatus sync_status);
  void RegisterLocalizedIntegerPref(const char* path,
                                    int locale_default_message_id,
                                    PrefSyncStatus sync_status);
  void RegisterLocalizedDoublePref(const char* path,
                                   int locale_default_message_id,
                                   PrefSyncStatus sync_status);
  void RegisterLocalizedStringPref(const char* path,
                                   int locale_default_message_id,
                                   PrefSyncStatus sync_status);
  void RegisterInt64Pref(const char* path,
                         int64 default_value,
                         PrefSyncStatus sync_status);
  void RegisterUint64Pref(const char* path,
                          uint64 default_value,
                          PrefSyncStatus sync_status);

  // Returns a new PrefRegistrySyncable that uses the same defaults
  // store.
  scoped_refptr<PrefRegistrySyncable> ForkForIncognito();

 private:
  virtual ~PrefRegistrySyncable();

  void RegisterSyncablePreference(const char* path,
                                  base::Value* default_value,
                                  PrefSyncStatus sync_status);

  SyncableRegistrationCallback callback_;

  // Contains the names of all registered preferences that are syncable.
  std::set<std::string> syncable_preferences_;

  DISALLOW_COPY_AND_ASSIGN(PrefRegistrySyncable);
};

#endif  // CHROME_BROWSER_PREFS_PREF_REGISTRY_SYNCABLE_H_
