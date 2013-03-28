/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// NaCl inter-module communication primitives.

#include <windows.h>

#include "native_client/src/shared/imc/nacl_imc.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

// This function is a no-op on Windows because there is no need to
// override the Windows definition of NaClCreateMemoryObject(): it
// already works inside the outer sandbox.
void NaClSetCreateMemoryObjectFunc(NaClCreateMemoryObjectFunc func) {
}

namespace nacl {

Handle CreateMemoryObject(size_t length, bool executable) {
  if (length % kMapPageSize) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return kInvalidHandle;
  }
  Handle memory = CreateFileMapping(
      INVALID_HANDLE_VALUE,
      NULL,
      executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE,
      static_cast<DWORD>(static_cast<unsigned __int64>(length) >> 32),
      static_cast<DWORD>(length & 0xFFFFFFFF), NULL);
  return (memory == NULL) ? kInvalidHandle : memory;
}

// TODO(mseaborn): Reduce duplication between this function and
// NaClHostDescMap().
void* Map(struct NaClDescEffector* effp,
          void* start, size_t length, int prot, int flags,
          Handle memory, off_t offset) {
  static DWORD prot_to_access[] = {
    0,  // NACL_ABI_PROT_NONE is not accepted: see below.
    FILE_MAP_READ,
    FILE_MAP_WRITE,
    FILE_MAP_ALL_ACCESS,
    FILE_MAP_EXECUTE,
    FILE_MAP_READ | FILE_MAP_EXECUTE,
    FILE_MAP_WRITE | FILE_MAP_EXECUTE,
    FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE
  };

  if (prot == NACL_ABI_PROT_NONE) {
    // There is no corresponding FILE_MAP_* option for PROT_NONE.  In
    // any case, this would not be very useful because the permissions
    // cannot later be increased beyond what was passed to
    // MapViewOfFileEx(), unlike in Unix.
    NaClLog(LOG_INFO, "nacl::Map: PROT_NONE not supported\n");
    SetLastError(ERROR_INVALID_PARAMETER);
    return kMapFailed;
  }

  if (!(flags & (kMapShared | kMapPrivate))) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return kMapFailed;
  }

  // Convert prot to the desired access type for MapViewOfFileEx().
  DWORD desired_access = prot_to_access[prot & 0x7];
  if (flags & kMapPrivate) {
    desired_access = FILE_MAP_COPY;
  }

  CHECK((flags & kMapFixed) != 0);
  for (size_t chunk_offset = 0;
       chunk_offset < length;
       chunk_offset += NACL_MAP_PAGESIZE) {
    uintptr_t chunk_addr = (uintptr_t) start + chunk_offset;

    (*effp->vtbl->UnmapMemory)(effp, chunk_addr, NACL_MAP_PAGESIZE);

    void* mapped = MapViewOfFileEx(memory, desired_access,
                                   0, (off_t) (offset + chunk_offset),
                                   NACL_MAP_PAGESIZE,
                                   (void*) chunk_addr);
    if (mapped != (void*) chunk_addr) {
      NaClLog(LOG_FATAL, "nacl::Map: MapViewOfFileEx() failed, error %d\n",
              GetLastError());
    }
  }
  return start;
}

int Unmap(void* start, size_t length) {
  // TODO(shiki): Try from start to start + length
  UnmapViewOfFile(start);
  return 0;
}

}  // namespace nacl
