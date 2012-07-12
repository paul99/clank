// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

// There is a known bug with certain versions of libc -- UNIX_PATH_MAX isn't
// defined in sys/un.h, but instead in linux/un.h. However, including both files
// will duplicate definition of sockaddr_un, so it's easier just to define
// UNIX_PATH_MAX ourselves.
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

#include "base/eintr_wrapper.h"
#include "base/threading/platform_thread.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "net/base/abstract_linux_socket.h"

namespace net {

namespace {

const int kReadBufSize = 4096;

}  // namespace

const SOCKET AbstractLinuxSocket::kInvalidSocket = -1;
const int AbstractLinuxSocket::kSocketError = -1;

AbstractLinuxSocket* AbstractLinuxSocket::CreateAndListen(
    const std::string& path,
    ListenSocket::ListenSocketDelegate *del,
    AbstractLinuxSocket::AbstractLinuxSocketAuthDelegate *auth_del) {
  SOCKET s = CreateAndBind(path);
  if (s == kInvalidSocket) {
    // TODO(mnaganov): error handling
  } else {
    AbstractLinuxSocket* sock = new AbstractLinuxSocket(s, del, auth_del);
    sock->Listen();
    return sock;
  }
  return NULL;
}

AbstractLinuxSocket::AbstractLinuxSocket(
    SOCKET s, ListenSocket::ListenSocketDelegate *del,
    AbstractLinuxSocket::AbstractLinuxSocketAuthDelegate *auth_del
) : ListenSocket(del),
    socket_(s),
    auth_delegate_(auth_del),
    reads_paused_(false),
    has_pending_reads_(false) {
  wait_state_ = NOT_WAITING;
}

AbstractLinuxSocket::~AbstractLinuxSocket() {
  CloseSocket(socket_);
}

SOCKET AbstractLinuxSocket::CreateAndBind(const std::string& path) {
  if (path.size() >= UNIX_PATH_MAX - 1) return kInvalidSocket;
  SOCKET s = socket(PF_UNIX, SOCK_STREAM, 0);
  if (s != kInvalidSocket) {
    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    socklen_t addr_len;
    // Convert the path given into abstract socket name. It must start with
    // the '\0' character, so we are adding it. |addr_len| must specify the
    // length of the structure exactly, as potentially the socket name may
    // have '\0' characters embedded (although we don't support this).
    snprintf(addr.sun_path, UNIX_PATH_MAX, "@%s", path.c_str());
    addr.sun_path[0] = '\0';
    addr_len = path.size() + offsetof(struct sockaddr_un, sun_path) + 1;
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), addr_len)) {
      close(s);
      s = kInvalidSocket;
    }
  }
  return s;
}

SOCKET AbstractLinuxSocket::Accept(SOCKET s) {
  sockaddr_un from;
  socklen_t from_len = sizeof(from);
  SOCKET conn =
      HANDLE_EINTR(accept(s, reinterpret_cast<sockaddr*>(&from), &from_len));
  if (conn != kInvalidSocket) {
    SetNonBlocking(conn);
  }
  return conn;
}

void AbstractLinuxSocket::SendInternal(const char* bytes, int len) {
  char* send_buf = const_cast<char *>(bytes);
  int len_left = len;
  while (true) {
    int sent = HANDLE_EINTR(send(socket_, send_buf, len_left, 0));
    if (sent == len_left) {  // A shortcut to avoid extraneous checks.
      break;
    }
    if (sent == kSocketError) {
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        LOG(ERROR) << "send failed: errno==" << errno;
        break;
      }
      // Otherwise we would block, and now we have to wait for a retry.
      // Fall through to PlatformThread::YieldCurrentThread()
    } else if (sent == 0) {
      // Socket was disconnected.
      Close();
    } else {
      // sent != len_left according to the shortcut above.
      // Shift the buffer start and send the remainder after a short while.
      send_buf += sent;
      len_left -= sent;
    }
    base::PlatformThread::YieldCurrentThread();
  }
}

void AbstractLinuxSocket::Listen() {
  int backlog = 10;  // TODO(mnaganov): maybe don't allow any backlog?
  listen(socket_, backlog);
  // TODO(mnaganov): error handling
  WatchSocket(WAITING_ACCEPT);
}

void AbstractLinuxSocket::Accept() {
  SOCKET conn = Accept(socket_);
  if (conn != kInvalidSocket) {
    if (auth_delegate_) {
      struct ucred ucred;
      socklen_t len = sizeof(ucred);
      if (getsockopt(conn, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1 ||
          !auth_delegate_->UserCanConnect(ucred.uid, ucred.gid)) {
        close(conn);
        return;
      }
    }
    scoped_refptr<AbstractLinuxSocket> sock(
        new AbstractLinuxSocket(conn, socket_delegate_, auth_delegate_));
    // it's up to the delegate to AddRef if it wants to keep it around
    sock->WatchSocket(WAITING_READ);
    socket_delegate_->DidAccept(this, sock);
  } else {
    // TODO(mnaganov): some error handling required here
  }
}

void AbstractLinuxSocket::Read() {
  char buf[kReadBufSize + 1];  // +1 for null termination
  int len;
  do {
    len = HANDLE_EINTR(recv(socket_, buf, kReadBufSize, 0));
    if (len == kSocketError) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        break;
      } else {
        // TODO(mnaganov): some error handling required here
        break;
      }
    } else if (len == 0) {
      Close();
    } else {
      // TODO(mnaganov): maybe change DidRead to take a length instead
      DCHECK_GT(len, 0);
      DCHECK_LE(len, kReadBufSize);
      buf[len] = 0;  // already create a buffer with +1 length
      socket_delegate_->DidRead(this, buf, len);
    }
  } while (len == kReadBufSize);
}

void AbstractLinuxSocket::Close() {
  if (wait_state_ == NOT_WAITING)
    return;
  wait_state_ = NOT_WAITING;
  UnwatchSocket();
  socket_delegate_->DidClose(this);
}

void AbstractLinuxSocket::CloseSocket(SOCKET s) {
  if (s && s != kInvalidSocket) {
    UnwatchSocket();
    close(s);
  }
}

void AbstractLinuxSocket::WatchSocket(WaitState state) {
  // Implicitly calls StartWatchingFileDescriptor().
  MessageLoopForIO::current()->WatchFileDescriptor(
      socket_, true, MessageLoopForIO::WATCH_READ, &watcher_, this);
  wait_state_ = state;
}

void AbstractLinuxSocket::UnwatchSocket() {
  watcher_.StopWatchingFileDescriptor();
}

void AbstractLinuxSocket::OnFileCanReadWithoutBlocking(int fd) {
  switch (wait_state_) {
    case WAITING_ACCEPT:
      Accept();
      break;
    case WAITING_READ:
      if (reads_paused_) {
        has_pending_reads_ = true;
      } else {
        Read();
      }
      break;
    default:
      // Close() is called by Read() in the Linux case.
      NOTREACHED();
      break;
  }
}

void AbstractLinuxSocket::OnFileCanWriteWithoutBlocking(int fd) {
  // MessagePumpLibevent callback, we don't listen for write events
  // so we shouldn't ever reach here.
  NOTREACHED();
}

}  // namespace net
