/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// NaCl inter-module communication primitives.
//
// This file implements common parts of IMC for "unix like systems" (i.e. not
// used on Windows).

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#if NACL_ANDROID
#include <linux/ashmem.h>
#endif

#include <algorithm>

#include "native_client/src/include/atomic_ops.h"

#include "native_client/src/shared/imc/nacl_imc.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"


namespace {

const char kNaClTempPrefixVar[] = "NACL_TMPFS_PREFIX";

// The pathname or SHM-namespace prefixes for memory objects created
// by CreateMemoryObject().
const char kShmTempPrefix[] = "/tmp/google-nacl-shm-";
const char kShmOpenPrefix[] = "/google-nacl-shm-";

NaClCreateMemoryObjectFunc g_create_memory_object_func = NULL;

}  // namespace


// Duplicate a file descriptor.
NaClHandle NaClDuplicateNaClHandle(NaClHandle handle) {
  return dup(handle);
}

void NaClSetCreateMemoryObjectFunc(NaClCreateMemoryObjectFunc func) {
  g_create_memory_object_func = func;
}

namespace nacl {

#if NACL_ANDROID
#define ASHMEM_DEVICE "/dev/ashmem"

static int AshmemCreateRegion(size_t size) {
  int fd;

  fd = open(ASHMEM_DEVICE, O_RDWR);
  if (fd < 0)
    return -1;

  if (ioctl(fd, ASHMEM_SET_SIZE, size) < 0) {
    close(fd);
    return -1;
  }

  return fd;
}
#endif

bool WouldBlock() {
  return (errno == EAGAIN) ? true : false;
}

int GetLastErrorString(char* buffer, size_t length) {
#if NACL_LINUX && !NACL_ANDROID
  // Note some Linux distributions provide only GNU version of strerror_r().
  if (buffer == NULL || length == 0) {
    errno = ERANGE;
    return -1;
  }
  char* message = strerror_r(errno, buffer, length);
  if (message != buffer) {
    size_t message_bytes = strlen(message) + 1;
    length = std::min(message_bytes, length);
    memmove(buffer, message, length);
    buffer[length - 1] = '\0';
  }
  return 0;
#else
  return strerror_r(errno, buffer, length);
#endif
}

#if !NACL_ANDROID
static Atomic32 memory_object_count = 0;

static int TryShmOrTempOpen(size_t length, const char* prefix, bool use_temp) {
  if (0 == length) {
    return -1;
  }

  char name[PATH_MAX];
  for (;;) {
    snprintf(name, sizeof name, "%s-%u.%u", prefix,
             getpid(),
             static_cast<uint32_t>(AtomicIncrement(&memory_object_count, 1)));
    int m;
    if (use_temp) {
      m = open(name, O_RDWR | O_CREAT | O_EXCL, 0);
    } else {
      m = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0);
    }
    if (0 <= m) {
      if (use_temp) {
        int rc = unlink(name);
        DCHECK(rc == 0);
      } else {
        int rc = shm_unlink(name);
        DCHECK(rc == 0);
      }
      if (ftruncate(m, length) == -1) {
        close(m);
        m = -1;
      }
      return m;
    }
    if (errno != EEXIST) {
      return -1;
    }
    // Retry only if we got EEXIST.
  }
}
#endif

Handle CreateMemoryObject(size_t length, bool executable) {
  if (0 == length) {
    return -1;
  }
  int fd;

  if (g_create_memory_object_func != NULL) {
    fd = g_create_memory_object_func(length, executable);
    if (fd >= 0)
      return fd;
  }

#if NACL_ANDROID
  return AshmemCreateRegion(length);
#else
  // /dev/shm is not always available on Linux.
  // Sometimes it's mounted as noexec.
  // To handle this case, sel_ldr can take a path
  // to tmpfs from the environment.
#if NACL_LINUX && defined(NACL_ENABLE_TMPFS_REDIRECT_VAR)
  if (NACL_ENABLE_TMPFS_REDIRECT_VAR) {
    const char* prefix = getenv(kNaClTempPrefixVar);
    if (prefix != NULL) {
      fd = TryShmOrTempOpen(length, prefix, true);
      if (fd >= 0) {
        return fd;
      }
    }
  }
#endif

  if (NACL_OSX && executable) {
    // On Mac OS X, shm_open() gives us file descriptors that the OS
    // won't mmap() with PROT_EXEC, which is no good for the dynamic
    // code region, so we must use /tmp instead.
    return TryShmOrTempOpen(length, kShmTempPrefix, true);
  }

  // Try shm_open().
  return TryShmOrTempOpen(length, kShmOpenPrefix, false);
#endif  // !NACL_ANDROID
}

void* Map(struct NaClDescEffector* effp,
          void* start, size_t length, int prot, int flags,
          Handle memory, off_t offset) {
  UNREFERENCED_PARAMETER(effp);

  static const int kPosixProt[] = {
    PROT_NONE,
    PROT_READ,
    PROT_WRITE,
    PROT_READ | PROT_WRITE,
    PROT_EXEC,
    PROT_READ | PROT_EXEC,
    PROT_WRITE | PROT_EXEC,
    PROT_READ | PROT_WRITE | PROT_EXEC
  };

  int adjusted = 0;
  if (flags & kMapShared) {
    adjusted |= MAP_SHARED;
  }
  if (flags & kMapPrivate) {
    adjusted |= MAP_PRIVATE;
  }
  if (flags & kMapFixed) {
    adjusted |= MAP_FIXED;
  }
  return mmap(start, length, kPosixProt[prot & 7], adjusted, memory, offset);
}

int Unmap(void* start, size_t length) {
  return munmap(start, length);
}

}  // namespace nacl
