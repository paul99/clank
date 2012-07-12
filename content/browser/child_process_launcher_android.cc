// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The Android specific child_process_launcher implementation is quite different
// to other platforms as it operates via the Java Sandboxed Service API.
// TODO(joth): Aside from strict process launch, the other functionality of this
// API such as background status and termination control are mostly the same as
// the posix variant, and could potentially be factored out for sharing.

#include "content/browser/child_process_launcher.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "content/browser/android/sandboxed_process_launcher.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"

using content::BrowserThread;

// Having the functionality of ChildProcessLauncher be in an internal
// ref counted object allows us to automatically terminate the process when the
// parent class destructs, while still holding on to state that we need.
class ChildProcessLauncher::Context
    : public base::RefCountedThreadSafe<ChildProcessLauncher::Context>,
      public content::browser::android::SandboxedProcessClient {
 public:
  Context()
      : client_(NULL),
        client_thread_id_(BrowserThread::UI),
        termination_status_(base::TERMINATION_STATUS_NORMAL_TERMINATION),
        exit_code_(content::RESULT_CODE_NORMAL_EXIT),
        terminate_child_on_shutdown_(true) {
  }

  void Launch(
      const base::environment_vector& environ,
      int ipcfd,
      CommandLine* cmd_line,
      Client* client) {
    client_ = client;
    CHECK(BrowserThread::GetCurrentThreadIdentifier(&client_thread_id_));
    // On Android it's safe to call through to Context.bindService from any
    // thread, so we don't impose any additional restrictions here. Happily it's
    // also better performing to do this directly on the client (e.g. UI) thread
    // than introduce additional thread bouncing here; see http://b/5035061.
    // (But note the current java-side implementation does actually do a
    // thread switch to call bindService(), see http://b/5694925.)
    LaunchInternal(environ, ipcfd, cmd_line);
  }

  void ResetClient() {
    // No need for locking as this function gets called on the same thread that
    // client_ would be used.
    CHECK(BrowserThread::CurrentlyOn(client_thread_id_));
    client_ = NULL;
  }

  void set_terminate_child_on_shutdown(bool terminate_on_shutdown) {
    terminate_child_on_shutdown_ = terminate_on_shutdown;
  }

  // SandboxedProcessClient
  virtual void OnSandboxedProcessStarted(base::ProcessHandle handle) {
    // Only invoke the client callback synchronously if we're already on the
    // correct thread and the pending connection exists (i.e. we're not still
    // inside the synchronous call into StartSandboxedProcess).
    if (BrowserThread::CurrentlyOn(client_thread_id_) &&
        pending_connection_.obj()) {
      Notify(handle);
    } else {
      BrowserThread::PostTask(
          client_thread_id_, FROM_HERE,
          base::Bind(
              &ChildProcessLauncher::Context::Notify,
              this,
              handle));
    }
  }

 private:
  friend class base::RefCountedThreadSafe<ChildProcessLauncher::Context>;
  friend class ChildProcessLauncher;

  virtual ~Context() {
    if (pending_connection_.obj()) {
      file_util::ScopedFD ipcfd_closer(&ipcfd_);
      content::browser::android::CancelStartSandboxedProcess(
          pending_connection_);
      pending_connection_.Reset();
    }
    Terminate();
  }

  void LaunchInternal(
      const base::environment_vector& env,
      int ipcfd,
      CommandLine* cmd_line) {
    int crashfd =
        content::GetContentClient()->browser()->GetCrashSignalFD(*cmd_line);

    scoped_ptr<CommandLine> cmd_line_deleter(cmd_line);
    // We need to close the client end of the IPC channel to reliably detect
    // child termination. We will close this fd after we create the child
    // process.
    ipcfd_ = ipcfd;
    pending_connection_.Reset(content::browser::android::StartSandboxedProcess(
        cmd_line->argv(), ipcfd, crashfd, this));
  }

  void Notify(base::ProcessHandle handle) {
    // Setup the ipcfd closer.
    file_util::ScopedFD ipcfd_closer(&ipcfd_);
    pending_connection_.Reset();
    process_.set_handle(handle);
    if (client_) {
      client_->OnProcessLaunched();
    } else {
      Terminate();
    }
  }

  void Terminate() {
    if (!process_.handle())
      return;

    if (!terminate_child_on_shutdown_)
      return;

    LOG(INFO) << "ChromeProcess: Stopping process with handle " << process_.handle();
    BrowserThread::PostTask(
        BrowserThread::PROCESS_LAUNCHER, FROM_HERE,
        base::Bind(
            &content::browser::android::StopSandboxedProcess,
            process_.handle()));
    process_.set_handle(base::kNullProcessHandle);
  }

  void SetProcessBackgrounded(bool background) {
    DCHECK(!pending_connection_.obj());
    process_.SetProcessBackgrounded(background);
  }

  Client* client_;
  BrowserThread::ID client_thread_id_;
  base::Process process_;
  base::TerminationStatus termination_status_;
  int exit_code_;
  // Controls whether the child process should be terminated on browser
  // shutdown. Default behavior is to terminate the child.
  bool terminate_child_on_shutdown_;

  // |pending_connection_| is used as an opaque handle, only valid while the
  // new child process is being started. Like other members, it is only modified
  // on the client thread.
  base::android::ScopedJavaGlobalRef<jobject> pending_connection_;
  // The fd to close after creating the process.
  int ipcfd_;
};


ChildProcessLauncher::ChildProcessLauncher(
    bool use_zygote,  // Ignored on android.
    const base::environment_vector& environ,
    int ipcfd,
    CommandLine* cmd_line,
    Client* client) {
  context_ = new Context();
  context_->Launch(environ, ipcfd, cmd_line, client);
}

ChildProcessLauncher::~ChildProcessLauncher() {
  context_->ResetClient();
}

bool ChildProcessLauncher::IsStarting() {
  return context_->pending_connection_.obj();
}

base::ProcessHandle ChildProcessLauncher::GetHandle() {
  DCHECK(!IsStarting());
  return context_->process_.handle();
}

base::TerminationStatus ChildProcessLauncher::GetChildTerminationStatus(
    int* exit_code) {
  base::ProcessHandle handle = context_->process_.handle();
  if (handle == base::kNullProcessHandle) {
    // Process is already gone, so return the cached termination status.
    if (exit_code)
      *exit_code = context_->exit_code_;
    return context_->termination_status_;
  }
  context_->termination_status_ =
      base::GetTerminationStatus(handle, &context_->exit_code_);

  if (exit_code)
    *exit_code = context_->exit_code_;

  // POSIX: If the process crashed, then the kernel closed the socket
  // for it and so the child has already died by the time we get
  // here. Since GetTerminationStatus called waitpid with WNOHANG,
  // it'll reap the process.  However, if GetTerminationStatus didn't
  // reap the child (because it was still running), we'll need to
  // Terminate via ProcessWatcher. So we can't close the handle here.
  if (context_->termination_status_ != base::TERMINATION_STATUS_STILL_RUNNING)
    context_->process_.Close();

  return context_->termination_status_;
}

void ChildProcessLauncher::SetProcessBackgrounded(bool background) {
  BrowserThread::PostTask(
      BrowserThread::PROCESS_LAUNCHER, FROM_HERE,
      base::Bind(
          &ChildProcessLauncher::Context::SetProcessBackgrounded,
          context_.get(),
          background));
}

void ChildProcessLauncher::SetTerminateChildOnShutdown(
  bool terminate_on_shutdown) {
  if (context_)
    context_->set_terminate_child_on_shutdown(terminate_on_shutdown);
}
