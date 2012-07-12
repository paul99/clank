// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/notifier/sync_notifier_impl_android.h"

#include "base/logging.h"
#include "chrome/browser/sync/notifier/android_notification_listener.h"
#include "chrome/browser/sync/notifier/invalidation_util.h"
#include "chrome/browser/sync/notifier/sync_notifier_observer.h"
#include "chrome/browser/sync/syncable/model_type.h"
#include "chrome/browser/sync/syncable/model_type_payload_map.h"
#include "content/public/browser/browser_thread.h"

namespace sync_notifier {

SyncNotifierImplAndroid::SyncNotifierImplAndroid(
    const base::WeakPtr<AndroidNotificationListener>& listener)
        : observer_list_(new ObserverListThreadSafe<SyncNotifierObserver>),
          listener_(listener) {
  CHECK(listener_.get());
  listener_->AddObserver(this);
}

SyncNotifierImplAndroid::~SyncNotifierImplAndroid() {
  if (listener_.get()) {
    listener_->RemoveObserver(this);
  } else {
    NOTREACHED();
  }
}

void SyncNotifierImplAndroid::AddObserver(SyncNotifierObserver* observer) {
  observer_list_->AddObserver(observer);
}

void SyncNotifierImplAndroid::RemoveObserver(SyncNotifierObserver* observer) {
  observer_list_->RemoveObserver(observer);
}

void SyncNotifierImplAndroid::UpdateCredentials(
    const std::string& email, const std::string& token) {
}

void SyncNotifierImplAndroid::SetUniqueId(const std::string& unique_id) {
  unique_id_ = unique_id;
}

void SyncNotifierImplAndroid::SetState(const std::string& state) {
  state_ = state;
}

void SyncNotifierImplAndroid::UpdateEnabledTypes(
    const syncable::ModelTypeSet types) {
  enabled_types_ = types;
}

void SyncNotifierImplAndroid::SendNotification(
    const syncable::ModelTypeSet changed_types) {
  // Do nothing. Android device will never send notifications.

}

// TODO(yfriedman): This is based on ChromeInvalidationClient::Invalidate. We
// should probably unify the code more, when it's upstreamed.
void SyncNotifierImplAndroid::OnNotificationReceived(
    const invalidation::Invalidation& invalidation) {
  syncable::ModelType model_type;
  if (!ObjectIdToRealModelType(invalidation.object_id(), &model_type)) {
    LOG(WARNING) << "Could not get invalidation model type; "
                 << "invalidating enabled types: " << ModelTypeSetToString(enabled_types_);
    syncable::ModelTypePayloadMap model_types_with_payloads =
        syncable::ModelTypePayloadMapFromEnumSet(enabled_types_, std::string());
    observer_list_->Notify(&SyncNotifierObserver::OnIncomingNotification,
                           model_types_with_payloads, REMOTE_NOTIFICATION);
    return;
  }

  // The invalidation API spec allows for the possibility of redundant
  // invalidations, so keep track of the max versions and drop
  // invalidations with old versions.
  //
  // TODO(akalin): Now that we keep track of registered types, we
  // should drop invalidations for unregistered types.  We may also
  // have to filter it at a higher level, as invalidations for
  // newly-unregistered types may already be in flight.
  //
  // TODO(akalin): Persist |max_invalidation_versions_| somehow.
  // TODO(yfriedman): Figure out handling invalidation->version() == UNKNOWN.  Had to
  // remove check due to compilation issues during merge.
  if (invalidation.version() != ipc::invalidation::Constants::UNKNOWN) {
    std::map<syncable::ModelType, int64>::const_iterator it =
        max_invalidation_versions_.find(model_type);
    if ((it != max_invalidation_versions_.end()) &&
        (invalidation.version() <= it->second)) {
      VLOG(1) << "Dropping redundant invalidation with version " << invalidation.version();
      return;
    }
    max_invalidation_versions_[model_type] = invalidation.version();
  }

  std::string payload;
  // payload() CHECK()'s has_payload(), so we must check it ourselves first.
  if (invalidation.has_payload())
    payload = invalidation.payload();

  syncable::ModelTypeSet types;
  types.Put(model_type);
  syncable::ModelTypePayloadMap model_types_with_payloads =
      syncable::ModelTypePayloadMapFromEnumSet(types, payload);
  observer_list_->Notify(&SyncNotifierObserver::OnIncomingNotification,
                         model_types_with_payloads, REMOTE_NOTIFICATION);
}

}  // namespace sync_notifier
