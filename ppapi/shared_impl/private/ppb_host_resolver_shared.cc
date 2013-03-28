// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/private/ppb_host_resolver_shared.h"

#include <cstddef>
#include <cstring>

#include "base/memory/scoped_ptr.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/private/net_address_private_impl.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {

PPB_HostResolver_Shared::PPB_HostResolver_Shared(PP_Instance instance)
    : Resource(OBJECT_IS_IMPL, instance),
      host_resolver_id_(GenerateHostResolverID()) {
}

PPB_HostResolver_Shared::PPB_HostResolver_Shared(
    const HostResource& resource)
    : Resource(OBJECT_IS_PROXY, resource),
      host_resolver_id_(GenerateHostResolverID()) {
}

PPB_HostResolver_Shared::~PPB_HostResolver_Shared() {
}

thunk::PPB_HostResolver_Private_API*
PPB_HostResolver_Shared::AsPPB_HostResolver_Private_API() {
  return this;
}

int32_t PPB_HostResolver_Shared::Resolve(
    const char* host,
    uint16_t port,
    const PP_HostResolver_Private_Hint* hint,
    scoped_refptr<TrackedCallback> callback) {
  if (!host)
    return PP_ERROR_BADARGUMENT;
  if (ResolveInProgress())
    return PP_ERROR_INPROGRESS;

  resolve_callback_ = callback;

  HostPortPair host_port;
  host_port.host = host;
  host_port.port = port;

  SendResolve(host_port, hint);
  return PP_OK_COMPLETIONPENDING;
}

PP_Var PPB_HostResolver_Shared::GetCanonicalName() {
  return StringVar::StringToPPVar(canonical_name_);
}

uint32_t PPB_HostResolver_Shared::GetSize() {
  if (ResolveInProgress())
    return 0;
  return static_cast<uint32_t>(net_address_list_.size());
}

bool PPB_HostResolver_Shared::GetNetAddress(uint32 index,
                                            PP_NetAddress_Private* address) {
  if (ResolveInProgress() || index >= GetSize())
    return false;
  *address = net_address_list_[index];
  return true;
}

void PPB_HostResolver_Shared::OnResolveCompleted(
    bool succeeded,
    const std::string& canonical_name,
    const std::vector<PP_NetAddress_Private>& net_address_list) {
  if (succeeded) {
    canonical_name_ = canonical_name;
    net_address_list_ = net_address_list;
  } else {
    canonical_name_.clear();
    net_address_list_.clear();
  }

  resolve_callback_->Run(succeeded ? PP_OK : PP_ERROR_FAILED);
}

uint32 PPB_HostResolver_Shared::GenerateHostResolverID() {
  static uint32 host_resolver_id = 0;
  return host_resolver_id++;
}

bool PPB_HostResolver_Shared::ResolveInProgress() const {
  return TrackedCallback::IsPending(resolve_callback_);
}

}  // namespace ppapi
