// Copyright (c) 2012 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "native_client/src/trusted/sel_universal/reverse_emulate.h"
#include <stdio.h>
#include <map>
#include <set>
#include <string>

#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/platform/scoped_ptr_refcount.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/trusted/desc/nacl_desc_wrapper.h"
#include "native_client/src/trusted/nonnacl_util/sel_ldr_launcher.h"
#include "native_client/src/trusted/reverse_service/reverse_service.h"
#include "native_client/src/trusted/sel_universal/rpc_universal.h"
#include "native_client/src/trusted/sel_universal/srpc_helper.h"


// Mock of ReverseInterface for use by nexes.
class ReverseEmulate : public nacl::ReverseInterface {
 public:
  ReverseEmulate();
  virtual ~ReverseEmulate();

  // debugging, messaging
  virtual void Log(nacl::string message);

  // Startup handshake
  virtual void StartupInitializationComplete();

  // Name service use.
  virtual bool EnumerateManifestKeys(std::set<nacl::string>* keys);
  virtual bool OpenManifestEntry(nacl::string url_key, int32_t* out_desc);
  virtual bool CloseManifestEntry(int32_t desc);
  virtual void ReportCrash();

  // The low-order 8 bits of the |exit_status| should be reported to
  // any interested parties.
  virtual void ReportExitStatus(int exit_status);

  // Send a string as a PostMessage to the browser.
  virtual void DoPostMessage(nacl::string message);

  // Create new service runtime process and return secure command
  // channel and untrusted application channel socket addresses.
  virtual int CreateProcess(nacl::DescWrapper** out_sock_addr,
                            nacl::DescWrapper** out_app_addr);

  // Request quota for a write to a file.
  virtual int64_t RequestQuotaForWrite(nacl::string file_id,
                                       int64_t offset,
                                       int64_t length);

  // covariant impl of Ref()
  ReverseEmulate* Ref() {  // down_cast
    return reinterpret_cast<ReverseEmulate*>(RefCountBase::Ref());
  }

 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(ReverseEmulate);
};

namespace {

typedef std::map<nacl::string, string> KeyToFileMap;

KeyToFileMap g_key_to_file;

nacl::scoped_ptr_refcount<nacl::ReverseService> g_reverse_service;

typedef std::map<nacl::Handle, nacl::SelLdrLauncherStandalone*>
  HandleToLauncherMap;

/*
 * TODO(phosek): Rather than using global ctor, SelLdrLauncher shall inherit
 * from RefCountBase class and we shall use nacl::scoped_ptr_refcount to
 * automatically manage the reference and dispose it once it's no longer
 * being used (http://code.google.com/p/nativeclient/issues/detail?id=3050).
 */
HandleToLauncherMap g_handle_to_launcher;

/*
 * TODO(phosek): These variables should be instance variables of Reverse
 * Emulate. However, we cannot make them such at the moment because they're
 * also being used by command handlers. This will require more significant
 * redesign/refactoring.
 */
int g_exited;
NaClMutex g_exit_mu;
NaClCondVar g_exit_cv;

}  // end namespace


bool ReverseEmulateInit(NaClSrpcChannel* command_channel,
                        nacl::SelLdrLauncherStandalone* launcher) {
  // Do the SRPC to the command channel to set up the reverse channel.
  // This returns a NaClDesc* containing a socket address.
  NaClLog(1, "ReverseEmulateInit: launching reverse RPC service\n");
  NaClDesc* h;
  NaClSrpcResultCodes rpc_result =
      NaClSrpcInvokeBySignature(command_channel, "reverse_setup::h", &h);
  if (NACL_SRPC_RESULT_OK != rpc_result) {
    NaClLog(LOG_ERROR, "ReverseEmulateInit: reverse setup failed\n");
    return false;
  }
  // Make a nacl::DescWrapper* from the NaClDesc*
  nacl::scoped_ptr<nacl::DescWrapper> conn_cap(launcher->WrapCleanup(h));
  if (conn_cap == NULL) {
    NaClLog(LOG_ERROR, "ReverseEmulateInit: reverse desc wrap failed\n");
    return false;
  }
  // The implementation of the ReverseInterface is our emulator class.
  nacl::scoped_ptr<ReverseEmulate> reverse_interface(new ReverseEmulate());
  // Construct locks guarding exit status.
  NaClXMutexCtor(&g_exit_mu);
  NaClXCondVarCtor(&g_exit_cv);
  g_exited = false;
  // Create an instance of ReverseService, which connects to the socket
  // address and exports the services from our emulator.
  g_reverse_service.reset(new nacl::ReverseService(conn_cap.get(),
                                                   reverse_interface->Ref()));
  if (g_reverse_service == NULL) {
    NaClLog(LOG_ERROR, "ReverseEmulateInit: reverse service ctor failed\n");
    return false;
  }
  // Successful creation of ReverseService took ownership of these.
  reverse_interface.release();
  conn_cap.release();
  // Starts the RPC handler for the reverse interface.
  if (!g_reverse_service->Start()) {
    NaClLog(LOG_ERROR, "ReverseEmulateInit: reverse service start failed\n");
    return false;
  }
  return true;
}

void ReverseEmulateFini() {
  CHECK(g_reverse_service != NULL);
  g_reverse_service->WaitForServiceThreadsToExit();
  g_reverse_service.reset(NULL);
  NaClMutexDtor(&g_exit_mu);
  NaClCondVarDtor(&g_exit_cv);
}

bool HandlerReverseEmuAddManifestMapping(NaClCommandLoop* ncl,
                                         const std::vector<string>& args) {
  UNREFERENCED_PARAMETER(ncl);
  if (args.size() < 3) {
    NaClLog(LOG_ERROR, "not enough args\n");
    return false;
  }
  NaClLog(1, "HandlerReverseEmulateAddManifestMapping(%s) -> %s\n",
          args[1].c_str(), args[2].c_str());
  // Set the mapping for the key.
  g_key_to_file[args[1]] = args[2];
  return true;
}

bool HandlerReverseEmuDumpManifestMappings(NaClCommandLoop* ncl,
                                           const std::vector<string>& args) {
  UNREFERENCED_PARAMETER(ncl);
  if (args.size() != 1) {
    NaClLog(LOG_ERROR, "unexpected args\n");
    return false;
  }
  printf("ReverseEmulate manifest mappings:\n");
  for (KeyToFileMap::iterator i = g_key_to_file.begin();
       i != g_key_to_file.end();
       ++i) {
    printf("'%s': '%s'\n", i->first.c_str(), i->second.c_str());
  }
  return true;
}

bool HandlerWaitForExit(NaClCommandLoop* ncl,
                        const std::vector<string>& args) {
  UNREFERENCED_PARAMETER(ncl);
  UNREFERENCED_PARAMETER(args);

  NaClXMutexLock(&g_exit_mu);
  while (!g_exited) {
    NaClXCondVarWait(&g_exit_cv, &g_exit_mu);
  }
  NaClXMutexUnlock(&g_exit_mu);

  return true;
}

ReverseEmulate::ReverseEmulate() {
  NaClLog(1, "ReverseEmulate::ReverseEmulate\n");
}

ReverseEmulate::~ReverseEmulate() {
  NaClLog(1, "ReverseEmulate::~ReverseEmulate\n");
}

void ReverseEmulate::Log(nacl::string message) {
  NaClLog(1, "ReverseEmulate::Log (message=%s)\n", message.c_str());
}

void ReverseEmulate::StartupInitializationComplete() {
  NaClLog(1, "ReverseEmulate::StartupInitializationComplete ()\n");
}

bool ReverseEmulate::EnumerateManifestKeys(std::set<nacl::string>* keys) {
  NaClLog(1, "ReverseEmulate::StartupInitializationComplete ()\n");
  // Enumerate the keys.
  std::set<nacl::string> manifest_keys;
  for (KeyToFileMap::iterator i = g_key_to_file.begin();
       i != g_key_to_file.end();
       ++i) {
    manifest_keys.insert(i->first);
  }
  *keys = manifest_keys;
  return true;
}

bool ReverseEmulate::OpenManifestEntry(nacl::string url_key,
                                       int32_t* out_desc) {
  NaClLog(1, "ReverseEmulate::OpenManifestEntry (url_key=%s)\n",
          url_key.c_str());
  *out_desc = -1;
  // Find the pathname for the key.
  if (g_key_to_file.find(url_key) == g_key_to_file.end()) {
    NaClLog(1, "ReverseEmulate::OpenManifestEntry: no pathname for key.\n");
    return false;
  }
  nacl::string pathname = g_key_to_file[url_key];
  NaClLog(1, "ReverseEmulate::OpenManifestEntry: pathname is %s.\n",
          pathname.c_str());
  *out_desc = OPEN(pathname.c_str(), O_RDONLY);
  return *out_desc >= 0;
}

bool ReverseEmulate::CloseManifestEntry(int32_t desc) {
  NaClLog(1, "ReverseEmulate::CloseManifestEntry (desc=%d)\n", desc);
  CLOSE(desc);
  return true;
}

void ReverseEmulate::ReportCrash() {
  NaClLog(1, "ReverseEmulate::ReportCrash\n");
  NaClXMutexLock(&g_exit_mu);
  g_exited = true;
  NaClXCondVarBroadcast(&g_exit_cv);
  NaClXMutexUnlock(&g_exit_mu);
}

void ReverseEmulate::ReportExitStatus(int exit_status) {
  NaClLog(1, "ReverseEmulate::ReportExitStatus (exit_status=%d)\n",
          exit_status);
  NaClXMutexLock(&g_exit_mu);
  g_exited = true;
  NaClXCondVarBroadcast(&g_exit_cv);
  NaClXMutexUnlock(&g_exit_mu);
}

void ReverseEmulate::DoPostMessage(nacl::string message) {
  NaClLog(1, "ReverseEmulate::DoPostMessage (message=%s)\n", message.c_str());
}

int ReverseEmulate::CreateProcess(nacl::DescWrapper** out_sock_addr,
                                  nacl::DescWrapper** out_app_addr) {
  NaClLog(1, "ReverseEmulate::CreateProcess)\n");
  vector<nacl::string> command_prefix;
  vector<nacl::string> sel_ldr_argv;
  vector<nacl::string> app_argv;

  nacl::SelLdrLauncherStandalone* launcher =
    new nacl::SelLdrLauncherStandalone();
  if (!launcher->StartViaCommandLine(command_prefix, sel_ldr_argv, app_argv)) {
    NaClLog(LOG_FATAL,
            "ReverseEmulate::CreateProcess: failed to launch sel_ldr\n");
  }
  g_handle_to_launcher[launcher->child_process()] = launcher;

  if (!launcher->ConnectBootstrapSocket()) {
    NaClLog(LOG_ERROR,
            "ReverseEmulate::CreateProcess:"
            " failed to connect boostrap socket\n");
    return -NACL_ABI_EAGAIN;
  }

  if (!launcher->RetrieveSockAddr()) {
    NaClLog(LOG_ERROR,
            "ReverseEmulate::CreateProcess: failed to obtain socket addr\n");
    return -NACL_ABI_EAGAIN;
  }
  *out_sock_addr = launcher->socket_addr();
  *out_app_addr = launcher->socket_addr();

  return 0;
}

int64_t ReverseEmulate::RequestQuotaForWrite(nacl::string file_id,
                                             int64_t offset,
                                             int64_t length) {
  NaClLog(1, "ReverseEmulate::RequestQuotaForWrite (file_id=%s, offset=%"
          NACL_PRId64", length=%"NACL_PRId64")\n", file_id.c_str(), offset,
          length);
  return length;
}
