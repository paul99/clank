// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TransientDeviceIds implementation.

#include "chrome/browser/media_gallery/transient_device_ids.h"

#include "base/guid.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "chrome/browser/system_monitor/media_storage_util.h"

namespace chrome {

TransientDeviceIds::TransientDeviceIds() {}

TransientDeviceIds::~TransientDeviceIds() {}

std::string TransientDeviceIds::GetTransientIdForDeviceId(
    const std::string& device_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(MediaStorageUtil::IsRemovableDevice(device_id));

  if (!ContainsKey(device_id_map_, device_id)) {
    std::string transient_id;
    do {
      transient_id = base::GenerateGUID();
    } while (ContainsKey(transient_id_map_, transient_id));

    device_id_map_[device_id] = transient_id;
    transient_id_map_[transient_id] = device_id;
  }
  return device_id_map_[device_id];
}

}  // namespace chrome
