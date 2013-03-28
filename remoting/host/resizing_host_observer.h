// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_RESIZING_HOST_OBSERVER_H_
#define REMOTING_HOST_RESIZING_HOST_OBSERVER_H_

#include <string>

#include "remoting/host/host_status_observer.h"
#include "remoting/host/chromoting_host.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "third_party/skia/include/core/SkSize.h"

namespace remoting {

class DesktopResizer;

// Use the specified DesktopResizer to match host desktop size to the client
// view size as closely as is possible. When the connection closes, restore
// the original desktop size.
class ResizingHostObserver : public HostStatusObserver {
 public:
  ResizingHostObserver(DesktopResizer* desktop_resizer, ChromotingHost* host);
  virtual ~ResizingHostObserver();

  // HostStatusObserver interface
  virtual void OnClientAuthenticated(const std::string& jid) OVERRIDE;
  virtual void OnClientDisconnected(const std::string& jid) OVERRIDE;
  virtual void OnClientDimensionsChanged(const std::string& jid,
                                         const SkISize& size) OVERRIDE;

 private:
  DesktopResizer* const desktop_resizer_;
  scoped_refptr<ChromotingHost> host_;
  SkISize original_size_;
  SkISize previous_size_;
  std::string client_jid_;

  DISALLOW_COPY_AND_ASSIGN(ResizingHostObserver);
};

}  // namespace remoting

#endif  // REMOTING_HOST_RESIZING_HOST_OBSERVER_H_
