// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The ChromeNotifierService works together with sync to maintain the state of
// user notifications, which can then be presented in the notification center,
// via the Notification UI Manager.

#include "chrome/browser/notifications/sync_notifier/chrome_notifier_service.h"

#include "base/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/browser/notifications/sync_notifier/chrome_notifier_delegate.h"
#include "chrome/browser/profiles/profile.h"
#include "googleurl/src/gurl.h"
#include "sync/api/sync_change.h"
#include "sync/api/sync_change_processor.h"
#include "sync/api/sync_error_factory.h"
#include "sync/protocol/sync.pb.h"
#include "sync/protocol/synced_notification_specifics.pb.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebTextDirection.h"

namespace notifier {

ChromeNotifierService::ChromeNotifierService(Profile* profile,
                                             NotificationUIManager* manager)
    : profile_(profile), notification_manager_(manager) {}
ChromeNotifierService::~ChromeNotifierService() {}

// Methods from ProfileKeyedService.
void ChromeNotifierService::Shutdown() {
}

// syncer::SyncableService implementation.

// This is called at startup to sync with the server.
// This code is not thread safe.
syncer::SyncMergeResult ChromeNotifierService::MergeDataAndStartSyncing(
      syncer::ModelType type,
      const syncer::SyncDataList& initial_sync_data,
      scoped_ptr<syncer::SyncChangeProcessor> sync_processor,
      scoped_ptr<syncer::SyncErrorFactory> error_handler) {
  // TODO(petewil): After I add the infrastructure for the test, add a check
  // that we are currently on the UI thread here.
  DCHECK_EQ(syncer::SYNCED_NOTIFICATIONS, type);
  syncer::SyncMergeResult merge_result(syncer::SYNCED_NOTIFICATIONS);

  // Mark all data we have now as local.  New incoming data will clear
  // the local flag, and merged data will clear the local flag.  We use
  // this to know what has to go back up to the server.
  for (std::vector<SyncedNotification*>::iterator it =
           notification_data_.begin();
      it != notification_data_.end();
      ++it) {
    (*it)->set_has_local_changes(true);
  }

  for (syncer::SyncDataList::const_iterator it = initial_sync_data.begin();
       it != initial_sync_data.end(); ++it) {
    const syncer::SyncData& sync_data = *it;
    DCHECK_EQ(syncer::SYNCED_NOTIFICATIONS, sync_data.GetDataType());

    // Build a local notification object from the sync data.
    scoped_ptr<SyncedNotification> incoming(CreateNotificationFromSyncData(
        sync_data));
    DCHECK(incoming.get());

    // Process each incoming remote notification.
    const std::string& id = incoming->notification_id();
    DCHECK_GT(id.length(), 0U);
    SyncedNotification* found = FindNotificationById(id);

    if (NULL == found) {
      // If there are no conflicts, copy in the data from remote.
      Add(incoming.Pass());
    } else {
      // If the remote and local data conflict (the read or deleted flag),
      // resolve the conflict.
      // TODO(petewil): Implement.
    }
  }

  // TODO(petewil): Implement the code that does the actual merging.
  // See chrome/browser/history/history.cc for an example to follow.

  // Make a list of local changes to send up to the sync server.
  syncer::SyncChangeList new_changes;
  for (std::vector<SyncedNotification*>::iterator it =
           notification_data_.begin();
      it != notification_data_.end();
      ++it) {
    if ((*it)->has_local_changes()) {
      syncer::SyncData sync_data = CreateSyncDataFromNotification(**it);
      new_changes.push_back(
          syncer::SyncChange(FROM_HERE,
                             syncer::SyncChange::ACTION_ADD,
                             sync_data));
    }
  }

  // Send up the changes that were made locally.
  if (new_changes.size() > 0) {
    merge_result.set_error(
        sync_processor.get()->ProcessSyncChanges(FROM_HERE, new_changes));
  }

  return merge_result;
}

void ChromeNotifierService::StopSyncing(syncer::ModelType type) {
  DCHECK_EQ(syncer::SYNCED_NOTIFICATIONS, type);
  // TODO(petewil): implement
}

syncer::SyncDataList ChromeNotifierService::GetAllSyncData(
      syncer::ModelType type) const {
  DCHECK_EQ(syncer::SYNCED_NOTIFICATIONS, type);
  syncer::SyncDataList sync_data;

  // Copy our native format data into a SyncDataList format.
  for (std::vector<SyncedNotification*>::const_iterator it =
          notification_data_.begin();
      it != notification_data_.end();
      ++it) {
    sync_data.push_back(CreateSyncDataFromNotification(**it));
  }

  return sync_data;
}

// This method is called when there is an incoming sync change from the server.
syncer::SyncError ChromeNotifierService::ProcessSyncChanges(
      const tracked_objects::Location& from_here,
      const syncer::SyncChangeList& change_list) {
  // TODO(petewil): add a check that we are called on the thread we expect.
  syncer::SyncError error;

  for (syncer::SyncChangeList::const_iterator it = change_list.begin();
       it != change_list.end(); ++it) {
    syncer::SyncData sync_data = it->sync_data();
    DCHECK_EQ(syncer::SYNCED_NOTIFICATIONS, sync_data.GetDataType());
    syncer::SyncChange::SyncChangeType change_type = it->change_type();

    scoped_ptr<SyncedNotification> new_notification(
        CreateNotificationFromSyncData(sync_data));
    if (!new_notification.get()) {
      NOTREACHED() << "Failed to read notification.";
      continue;
    }

    switch (change_type) {
      case syncer::SyncChange::ACTION_ADD:
        // TODO(petewil): Update the notification if it already exists
        // as opposed to adding it.
        Add(new_notification.Pass());
        break;
      // TODO(petewil): Implement code to add delete and update actions.

      default:
        break;
    }
  }

  return error;
}

// Support functions for data type conversion.

// Static method.  Get to the sync data in our internal format.
syncer::SyncData ChromeNotifierService::CreateSyncDataFromNotification(
    const SyncedNotification& notification) {
  return *notification.sync_data();
}

// Static Method.  Convert from SyncData to our internal format.
scoped_ptr<SyncedNotification>
    ChromeNotifierService::CreateNotificationFromSyncData(
        const syncer::SyncData& sync_data) {
  // Get a pointer to our data within the sync_data object.
  sync_pb::SyncedNotificationSpecifics specifics =
      sync_data.GetSpecifics().synced_notification();

  // Check for mandatory fields in the sync_data object.
  if (!specifics.has_coalesced_notification() ||
      !specifics.coalesced_notification().has_id() ||
      !specifics.coalesced_notification().id().has_app_id() ||
      specifics.coalesced_notification().id().app_id().empty() ||
      !specifics.coalesced_notification().has_render_info() ||
      !specifics.coalesced_notification().has_creation_time_msec()) {
    return scoped_ptr<SyncedNotification>();
  }

  // Create a new notification object based on the supplied sync_data.
  scoped_ptr<SyncedNotification> notification(
      new SyncedNotification(sync_data));

  return notification.Pass();
}

// This returns a pointer into a vector that we own.  Caller must not free it.
// Returns NULL if no match is found.
// This uses the <app_id/coalescing_key> pair as a key.
SyncedNotification* ChromeNotifierService::FindNotificationById(
    const std::string& id) {
  // TODO(petewil): We can make a performance trade off here.
  // While the vector has good locality of reference, a map has faster lookup.
  // Based on how bit we expect this to get, maybe change this to a map.
  for (std::vector<SyncedNotification*>::const_iterator it =
          notification_data_.begin();
      it != notification_data_.end();
      ++it) {
    SyncedNotification* notification = *it;
    if (id == notification->notification_id())
      return *it;
  }

  return NULL;
}

// Add a new notification to our data structure.  This takes ownership
// of the passed in pointer.
void ChromeNotifierService::Add(scoped_ptr<SyncedNotification> notification) {
  SyncedNotification* notification_copy = notification.get();
  // Take ownership of the object and put it into our local storage.
  notification_data_.push_back(notification.release());

  // Show it to the user.
  Show(notification_copy);
}

// Send the notification to the NotificationUIManager to show to the user.
void ChromeNotifierService::Show(SyncedNotification* notification) {
  // Set up the fields we need to send and create a Notification object.
  GURL origin_url(notification->origin_url());
  GURL icon_url(notification->icon_url());
  string16 title = UTF8ToUTF16(notification->title());
  string16 body = UTF8ToUTF16(notification->body());
  // TODO(petewil): What goes in the display source, is empty OK?
  string16 display_source;
  string16 replace_id = UTF8ToUTF16(notification->notification_id());
  // The delegate will eventually catch calls that the notification
  // was read or deleted, and send the changes back to the server.
  scoped_refptr<NotificationDelegate> delegate =
      new ChromeNotifierDelegate(notification->notification_id());

  Notification ui_notification(origin_url, icon_url, title, body,
                               WebKit::WebTextDirectionDefault,
                               display_source, replace_id, delegate);

  notification_manager_->Add(ui_notification, profile_);

  DVLOG(1) << "Synced Notification arrived! " << title << " " << body
           << " " << icon_url << " " << replace_id;

  return;
}

}  // namespace notifier
