// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_FILEAPI_MEDIA_MTP_DEVICE_MAP_SERVICE_H_
#define WEBKIT_FILEAPI_MEDIA_MTP_DEVICE_MAP_SERVICE_H_

#include <map>

#include "base/file_path.h"
#include "base/lazy_instance.h"
#include "base/synchronization/lock.h"
#include "webkit/storage/webkit_storage_export.h"

namespace fileapi {

class MTPDeviceDelegate;

// This class provides media transfer protocol (MTP) device delegate to
// complete media file system operations. ScopedMTPDeviceMapEntry class
// manages the device map entries.
class WEBKIT_STORAGE_EXPORT MTPDeviceMapService {
 public:
  static MTPDeviceMapService* GetInstance();

  // Adds the MTP device delegate to the map service. |device_location|
  // specifies the mount location of the MTP device.
  // Called on a media task runner thread.
  void AddDelegate(const FilePath::StringType& device_location,
                   MTPDeviceDelegate* delegate);

  // Removes the MTP device delegate from the map service. |device_location|
  // specifies the mount location of the MTP device.
  // Called on the UI thread.
  void RemoveDelegate(const FilePath::StringType& device_location);

  // Gets the media device delegate associated with |filesystem_id|.
  // Return NULL if the |filesystem_id| is no longer valid (e.g. because the
  // corresponding device is detached, etc).
  // Called on a media task runner thread.
  // TODO(thestig) DCHECK AddDelegate() and GetMTPDeviceDelegate() are actually
  // called on the same task runner.
  MTPDeviceDelegate* GetMTPDeviceDelegate(const std::string& filesystem_id);

 private:
  friend struct base::DefaultLazyInstanceTraits<MTPDeviceMapService>;

  // Mapping of device_location and MTPDeviceDelegate* object. It is safe to
  // store and access the raw pointer. This class operates on the IO thread.
  typedef std::map<FilePath::StringType, MTPDeviceDelegate*> DelegateMap;

  // Get access to this class using GetInstance() method.
  MTPDeviceMapService();
  ~MTPDeviceMapService();

  // Map of attached mtp device delegates.
  DelegateMap delegate_map_;
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(MTPDeviceMapService);
};

}  // namespace fileapi

#endif  // WEBKIT_FILEAPI_MEDIA_MTP_DEVICE_MAP_SERVICE_H_
