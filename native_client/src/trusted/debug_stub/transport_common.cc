/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <string>

#include "native_client/src/include/nacl_scoped_ptr.h"
#include "native_client/src/include/portability_sockets.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/transport.h"
#include "native_client/src/trusted/debug_stub/util.h"

using gdb_rsp::stringvec;
using gdb_rsp::StringSplit;

namespace port {

typedef int socklen_t;

class Transport : public ITransport {
 public:
  Transport()
    : buf_(new char[kBufSize]),
      pos_(0),
      size_(0) {
    handle_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  }

  explicit Transport(NaClSocketHandle s)
    : buf_(new char[kBufSize]),
      pos_(0),
      size_(0),
      handle_(s) {
  }

  ~Transport() {
    if (handle_ != NACL_INVALID_SOCKET) NaClCloseSocket(handle_);
  }

  // Read from this transport, return true on success.
  virtual bool Read(void *ptr, int32_t len);

  // Write to this transport, return true on success.
  virtual bool Write(const void *ptr, int32_t len);

  // Return true if there is data to read.
  virtual bool IsDataAvailable() {
    if (pos_ < size_) {
      return true;
    }
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(handle_, &fds);

    // We want a "non-blocking" check
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Check if this file handle can select on read
    int cnt = select(static_cast<int>(handle_) + 1, &fds, 0, 0, &timeout);

    // If we are ready, or if there is an error.  We return true
    // on error, to let the next IO request fail.
    if (cnt != 0) return true;

    return false;
  }

// On windows, the header that defines this has other definition
// colitions, so we define it outselves just in case
#ifndef SD_BOTH
#define SD_BOTH 2
#endif

  virtual void Disconnect() {
    // Shutdown the conneciton in both diections.  This should
    // always succeed, and nothing we can do if this fails.
    (void) ::shutdown(handle_, SD_BOTH);
  }

 protected:
  // Copy buffered data to *dst up to len bytes and update dst and len.
  void CopyFromBuffer(char **dst, int32_t *len);

  static const int kBufSize = 4096;
  nacl::scoped_array<char> buf_;
  int32_t pos_;
  int32_t size_;
  NaClSocketHandle handle_;
};

void Transport::CopyFromBuffer(char **dst, int32_t *len) {
  int32_t copy_bytes = std::min(*len, size_ - pos_);
  memcpy(*dst, buf_.get() + pos_, copy_bytes);
  pos_ += copy_bytes;
  *len -= copy_bytes;
  *dst += copy_bytes;
}

bool Transport::Read(void *ptr, int32_t len) {
  char *dst = static_cast<char *>(ptr);
  if (pos_ < size_) {
    CopyFromBuffer(&dst, &len);
  }
  while (len > 0) {
    int result = ::recv(handle_, buf_.get(), kBufSize, 0);
    if (result > 0) {
      pos_ = 0;
      size_ = result;
      CopyFromBuffer(&dst, &len);
      continue;
    }
    if (result == 0) {
      return false;
    }
    if (NaClSocketGetLastError() != EINTR) {
      return false;
    }
  }
  return true;
}

bool Transport::Write(const void *ptr, int32_t len) {
  const char *src = static_cast<const char *>(ptr);
  while (len > 0) {
    int result = ::send(handle_, src, len, 0);
    if (result > 0) {
      src += result;
      len -= result;
      continue;
    }
    if (result == 0) {
      return false;
    }
    if (NaClSocketGetLastError() != EINTR) {
      return false;
    }
  }
  return true;
}

// Convert string in the form of [addr][:port] where addr is a
// IPv4 address or host name, and port is a 16b tcp/udp port.
// Both portions are optional, and only the portion of the address
// provided is updated.  Values are provided in network order.
static bool StringToIPv4(const std::string &instr, uint32_t *addr,
                         uint16_t *port) {
  // Make a copy so the are unchanged unless we succeed
  uint32_t outaddr = *addr;
  uint16_t outport = *port;

  // Substrings of the full ADDR:PORT
  std::string addrstr;
  std::string portstr;

  // We should either have one or two tokens in the form of:
  //  IP - IP, NUL
  //  IP: -  IP, NUL
  //  :PORT - NUL, PORT
  //  IP:PORT - IP, PORT

  // Search for the port marker
  size_t portoff = instr.find(':');

  // If we found a ":" before the end, get both substrings
  if ((portoff != std::string::npos) && (portoff + 1 < instr.size())) {
    addrstr = instr.substr(0, portoff);
    portstr = instr.substr(portoff + 1, std::string::npos);
  } else {
    // otherwise the entire string is the addr portion.
    addrstr = instr;
    portstr = "";
  }

  // If the address portion was provided, update it
  if (addrstr.size()) {
    // Special case 0.0.0.0 which means any IPv4 interface
    if (addrstr == "0.0.0.0") {
      outaddr = 0;
    } else {
      struct hostent *host = gethostbyname(addrstr.data());

      // Check that we found an IPv4 host
      if ((NULL == host) || (AF_INET != host->h_addrtype)) return false;

      // Make sure the IP list isn't empty.
      if (0 == host->h_addr_list[0]) return false;

      // Use the first address in the array of address pointers.
      uint32_t **addrarray = reinterpret_cast<uint32_t**>(host->h_addr_list);
      outaddr = *addrarray[0];
    }
  }

  // if the port portion was provided, then update it
  if (portstr.size()) {
    int val = atoi(portstr.data());
    if ((val < 0) || (val > 65535)) return false;
    outport = ntohs(static_cast<uint16_t>(val));
  }

  // We haven't failed, so set the values
  *addr = outaddr;
  *port = outport;
  return true;
}

static bool BuildSockAddr(const char *addr, struct sockaddr_in *sockaddr) {
  std::string addrstr = addr;
  uint32_t *pip = reinterpret_cast<uint32_t*>(&sockaddr->sin_addr.s_addr);
  uint16_t *pport = reinterpret_cast<uint16_t*>(&sockaddr->sin_port);

  sockaddr->sin_family = AF_INET;
  return StringToIPv4(addrstr, pip, pport);
}

SocketBinding::SocketBinding(NaClSocketHandle socket_handle)
    : socket_handle_(socket_handle) {
}

SocketBinding *SocketBinding::Bind(const char *addr) {
  NaClSocketHandle socket_handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_handle == NACL_INVALID_SOCKET) {
    NaClLog(LOG_ERROR, "Failed to create socket.\n");
    return NULL;
  }
  struct sockaddr_in saddr;
  // Clearing sockaddr_in first appears to be necessary on Mac OS X.
  memset(&saddr, 0, sizeof(saddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(0x7F000001);
  saddr.sin_port = htons(4014);

  // Override portions address that are provided
  if (addr) BuildSockAddr(addr, &saddr);

  // This is necessary to ensure that the TCP port is released
  // promptly when sel_ldr exits.  Without this, the TCP port might
  // only be released after a timeout, and later processes can fail
  // to bind it.
  int reuse_address = 1;
  if (setsockopt(socket_handle, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<char *>(&reuse_address),
                 sizeof(reuse_address))) {
    NaClLog(LOG_WARNING, "Failed to set SO_REUSEADDR option.\n");
  }
  // Do not delay sending small packets.  This significantly speeds up
  // remote debugging.  Debug stub uses buffering to send outgoing packets so
  // they are not split into more TCP packets than necessary.
  int nodelay = 1;
  if (setsockopt(socket_handle, IPPROTO_TCP, TCP_NODELAY,
                 reinterpret_cast<char *>(&nodelay),
                 sizeof(nodelay))) {
    NaClLog(LOG_WARNING, "Failed to set TCP_NODELAY option.\n");
  }

  struct sockaddr *psaddr = reinterpret_cast<struct sockaddr *>(&saddr);
  if (bind(socket_handle, psaddr, addrlen)) {
    NaClLog(LOG_ERROR, "Failed to bind server.\n");
    return NULL;
  }

  if (listen(socket_handle, 1)) {
    NaClLog(LOG_ERROR, "Failed to listen.\n");
    return NULL;
  }
  return new SocketBinding(socket_handle);
}

ITransport *SocketBinding::AcceptConnection() {
  NaClSocketHandle socket = ::accept(socket_handle_, NULL, 0);
  if (socket != NACL_INVALID_SOCKET)
    return new Transport(socket);
  return NULL;
}

}  // namespace port
