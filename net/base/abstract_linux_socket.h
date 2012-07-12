// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unix domain socket server that handles IO asynchronously in the specified
// MessageLoop.

#ifndef NET_BASE_ABSTRACT_LINUX_SOCKET_H_
#define NET_BASE_ABSTRACT_LINUX_SOCKET_H_
#pragma once

#include "build/build_config.h"

#include <string>
#include "base/message_loop.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "net/base/listen_socket.h"
#include "net/base/net_export.h"

typedef int SOCKET;

namespace net {

// Implements abstract Linux socket interface
class NET_EXPORT AbstractLinuxSocket : public ListenSocket,
                                       public MessageLoopForIO::Watcher {
 public:
  class AbstractLinuxSocketAuthDelegate {
   public:
    virtual ~AbstractLinuxSocketAuthDelegate() {}
    virtual bool UserCanConnect(uid_t uid, gid_t gid) = 0;
  };

  static AbstractLinuxSocket* CreateAndListen(
      const std::string& path,
      ListenSocketDelegate *del,
      AbstractLinuxSocketAuthDelegate *auth_del);

 protected:
  enum WaitState {
    NOT_WAITING      = 0,
    WAITING_ACCEPT   = 1,
    WAITING_READ     = 2
  };

  static const SOCKET kInvalidSocket;
  static const int kSocketError;

  AbstractLinuxSocket(SOCKET s,
                      ListenSocketDelegate *del,
                      AbstractLinuxSocketAuthDelegate *auth_del);
  virtual ~AbstractLinuxSocket();
  static SOCKET CreateAndBind(const std::string& path);
  // if valid, returned SOCKET is non-blocking
  static SOCKET Accept(SOCKET s);

  virtual void SendInternal(const char* bytes, int len) OVERRIDE;

  virtual void Listen();
  virtual void Accept();
  virtual void Read();
  virtual void Close();
  virtual void CloseSocket(SOCKET s);

  void WatchSocket(WaitState state);
  void UnwatchSocket();

  // Called by MessagePumpLibevent when the socket is ready to do I/O
  virtual void OnFileCanReadWithoutBlocking(int fd) OVERRIDE;
  virtual void OnFileCanWriteWithoutBlocking(int fd) OVERRIDE;
  WaitState wait_state_;
  // The socket's libevent wrapper
  MessageLoopForIO::FileDescriptorWatcher watcher_;

  SOCKET socket_;
  AbstractLinuxSocketAuthDelegate *auth_delegate_;

 private:
  bool reads_paused_;
  bool has_pending_reads_;

  DISALLOW_COPY_AND_ASSIGN(AbstractLinuxSocket);
};

}  // namespace net

#endif  // NET_BASE_ABSTRACT_LINUX_SOCKET_H_
