// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MANAGED_MODE_MANAGED_USER_SERVICE_H_
#define CHROME_BROWSER_MANAGED_MODE_MANAGED_USER_SERVICE_H_

#include <vector>

#include "base/prefs/public/pref_change_registrar.h"
#include "base/string16.h"
#include "chrome/browser/extensions/management_policy.h"
#include "chrome/browser/managed_mode/managed_mode_url_filter.h"
#include "chrome/browser/profiles/profile_keyed_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class ManagedModeURLFilter;
class ManagedModeSiteList;
class PrefRegistrySyncable;
class Profile;

// This class handles all the information related to a given managed profile
// (e.g. the installed content packs, the default URL filtering behavior, or
// manual whitelist/blacklist overrides).
class ManagedUserService : public ProfileKeyedService,
                           public extensions::ManagementPolicy::Provider,
                           public content::NotificationObserver {
 public:
  typedef std::vector<string16> CategoryList;

  enum ManualBehavior {
    MANUAL_NONE = 0,
    MANUAL_ALLOW,
    MANUAL_BLOCK
  };

  explicit ManagedUserService(Profile* profile);
  virtual ~ManagedUserService();

  bool ProfileIsManaged() const;

  static void RegisterUserPrefs(PrefRegistrySyncable* registry);

  // Returns the URL filter for the IO thread, for filtering network requests
  // (in ManagedModeResourceThrottle).
  scoped_refptr<const ManagedModeURLFilter> GetURLFilterForIOThread();

  // Returns the URL filter for the UI thread, for filtering navigations and
  // classifying sites in the history view.
  ManagedModeURLFilter* GetURLFilterForUIThread();

  // Returns the URL's category, obtained from the installed content packs.
  int GetCategory(const GURL& url);

  // Returns the list of all known human-readable category names, sorted by ID
  // number. Called in the critical path of drawing the history UI, so needs to
  // be fast.
  void GetCategoryNames(CategoryList* list);

  // These methods allow querying and modifying the manual filtering behavior.
  // The manual behavior is set by the user and overrides all other settings
  // (whitelists or the default behavior).

  // Returns the manual behavior for the given host.
  ManualBehavior GetManualBehaviorForHost(const std::string& hostname);

  // Sets the manual behavior for the given host.
  void SetManualBehaviorForHosts(const std::vector<std::string>& hostnames,
                                 ManualBehavior behavior);

  // Returns the manual behavior for the given URL.
  ManualBehavior GetManualBehaviorForURL(const GURL& url);

  // Sets the manual behavior for the given URL.
  void SetManualBehaviorForURLs(const std::vector<GURL>& url,
                                ManualBehavior behavior);

  void SetElevatedForTesting(bool is_elevated);

  // Initializes this object. This method does nothing if the profile is not
  // managed. This method only needs to be called if the profile is set to be
  // managed after the ManagedUserService has been created (which happens when
  // creating a new profile).
  void Init();

  // ExtensionManagementPolicy::Provider implementation:
  virtual std::string GetDebugPolicyProviderName() const OVERRIDE;
  virtual bool UserMayLoad(const extensions::Extension* extension,
                           string16* error) const OVERRIDE;
  virtual bool UserMayModifySettings(const extensions::Extension* extension,
                                     string16* error) const OVERRIDE;

  // content::NotificationObserver implementation:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

 private:
  friend class ManagedUserServiceExtensionTest;

  // A bridge from ManagedMode (which lives on the UI thread) to the
  // ManagedModeURLFilters, one of which lives on the IO thread. This class
  // mediates access to them and makes sure they are kept in sync.
  class URLFilterContext {
   public:
    URLFilterContext();
    ~URLFilterContext();

    ManagedModeURLFilter* ui_url_filter() const;
    ManagedModeURLFilter* io_url_filter() const;

    void SetDefaultFilteringBehavior(
        ManagedModeURLFilter::FilteringBehavior behavior);
    void LoadWhitelists(ScopedVector<ManagedModeSiteList> site_lists);
    void SetManualHosts(scoped_ptr<std::map<std::string, bool> > host_map);
    void SetManualURLs(scoped_ptr<std::map<GURL, bool> > url_map);

   private:
    // ManagedModeURLFilter is refcounted because the IO thread filter is used
    // both by ProfileImplIOData and OffTheRecordProfileIOData (to filter
    // network requests), so they both keep a reference to it.
    // Clients should not keep references to the UI thread filter, however
    // (the filter will live as long as the profile lives, and afterwards it
    // should not be used anymore either).
    scoped_refptr<ManagedModeURLFilter> ui_url_filter_;
    scoped_refptr<ManagedModeURLFilter> io_url_filter_;

    DISALLOW_COPY_AND_ASSIGN(URLFilterContext);
  };

  // Internal implementation for ExtensionManagementPolicy::Delegate methods.
  // If |error| is not NULL, it will be filled with an error message if the
  // requested extension action (install, modify status, etc.) is not permitted.
  bool ExtensionManagementPolicyImpl(string16* error) const;

  // Returns a list of all installed and enabled site lists in the current
  // managed profile.
  ScopedVector<ManagedModeSiteList> GetActiveSiteLists();

  void OnDefaultFilteringBehaviorChanged();

  void UpdateSiteLists();

  // Updates the manual overrides for hosts in the URL filters when the
  // corresponding preference is changed.
  void UpdateManualHosts();

  // Updates the manual overrides for URLs in the URL filters when the
  // corresponding preference is changed.
  void UpdateManualURLs();

  // Owns us via the ProfileKeyedService mechanism.
  Profile* profile_;

  // If ManagedUserService is in an elevated state, a custodian user has
  // authorized making changes (to install additional content packs, for
  // example).
  bool is_elevated_;

  content::NotificationRegistrar registrar_;
  PrefChangeRegistrar pref_change_registrar_;

  URLFilterContext url_filter_context_;
};

#endif  // CHROME_BROWSER_MANAGED_MODE_MANAGED_USER_SERVICE_H_
