// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_DEVTOOLS_SERVER_H_
#define CONTENT_BROWSER_ANDROID_DEVTOOLS_SERVER_H_

#include <set>
#include <vector>

#include "base/memory/singleton.h"
#include "net/base/abstract_linux_socket.h"

namespace content {
class DevToolsHttpHandler;
class WebContents;
}

// This class controls Chrome Developer Tools remote debugging server.
class DevToolsServer :
    public net::AbstractLinuxSocket::AbstractLinuxSocketAuthDelegate {
 public:
  // Returns the singleton instance (and initializes it if necessary).
  static DevToolsServer* GetInstance();

  // Opens linux abstract socket to be ready for remote debugging.
  void Start();

  // Closes debugging socket, stops debugging.
  void Stop();

  bool is_started() { return protocol_handler_; }

  // Adds inspectable |tabcontents| to the collection of the attachable
  // tabs.
  void AddInspectableTab(content::WebContents* contents);

  // Removes |tabcontents| from the collection of the attachable
  // tabs.
  void RemoveInspectableTab(content::WebContents* contents);

  virtual bool UserCanConnect(uid_t uid, gid_t gid) OVERRIDE;

 private:
  // Obtain an instance via GetInstance().
  DevToolsServer();
  friend struct DefaultSingletonTraits<DevToolsServer>;
  virtual ~DevToolsServer();

  content::DevToolsHttpHandler* protocol_handler_;
  std::set<content::WebContents*> inspectable_tabs_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsServer);
};

#endif  // CONTENT_BROWSER_ANDROID_DEVTOOLS_SERVER_H_
