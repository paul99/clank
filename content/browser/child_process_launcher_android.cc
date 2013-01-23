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
#include "base/path_service.h"
#include "base/platform_file.h"
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
        terminate_child_on_shutdown_(true),
        starting_(true) {
  }

  void Launch(
      const base::environment_vector& environ,
      int ipcfd,
      CommandLine* cmd_line,
      Client* client) {
    client_ = client;
    CHECK(BrowserThread::GetCurrentThreadIdentifier(&client_thread_id_));
    BrowserThread::PostTask(
        BrowserThread::PROCESS_LAUNCHER, FROM_HERE,
        base::Bind(
            &Context::OpenPakFiles,
            make_scoped_refptr(this),
            environ,
            ipcfd,
            cmd_line,
            client_thread_id_));
  }

  void OpenPakFiles(const base::environment_vector& environ,
                    int ipcfd,
                    CommandLine* cmd_line,
                    BrowserThread::ID client_thread_id) {
    CHECK(BrowserThread::CurrentlyOn(BrowserThread::PROCESS_LAUNCHER));

    // Load the chrome.pak file.
    FilePath data_path;
    PathService::Get(base::DIR_ANDROID_APP_DATA, &data_path);
    DCHECK(!data_path.empty());
    data_path = data_path.AppendASCII("paks").AppendASCII("chrome.pak");
    int flags = base::PLATFORM_FILE_OPEN | base::PLATFORM_FILE_READ;
    base::PlatformFile chrome_pak_fd =
        base::CreatePlatformFile(data_path, flags, NULL, NULL);
    if (chrome_pak_fd == base::kInvalidPlatformFileValue)
      NOTREACHED() << "Failed to open " << data_path.value();

    // Load the local pak file.
    const std::string locale =
        content::GetContentClient()->browser()->GetApplicationLocale();
    // Ideally we would call ResourceBundle::GetLocaleFilePath() to get the
    // locale pak file path, but it would be a dependency violation to access
    // ui/resources.
    FilePath locale_pak_path;
    PathService::Get(base::DIR_ANDROID_APP_DATA, &locale_pak_path);
    locale_pak_path = locale_pak_path.Append(FILE_PATH_LITERAL("paks"));
    locale_pak_path = locale_pak_path.AppendASCII(locale + ".pak");
    base::PlatformFile locale_pak_fd =
        base::CreatePlatformFile(locale_pak_path, flags, NULL, NULL);
    if (locale_pak_fd == base::kInvalidPlatformFileValue)
      NOTREACHED() << "Failed to open " << locale_pak_path.value();

#if defined(USE_LINUX_BREAKPAD)
    minidump_fd_ = content::GetContentClient()->browser()->CreateMinidumpFile();
    if (minidump_fd_ == base::kInvalidPlatformFileValue) {
      LOG(ERROR) << "Failed to create file for minidump, crash reporting will be "
          "disabled for this process.";
    }
#else
    minidump_fd_ = base::kInvalidPlatformFileValue;
#endif


    // On Android it's safe to call through to Context.bindService from any
    // thread, so we don't bounce to the UI thread here.
    // (But note the current java-side implementation does actually do a
    // thread switch to call bindService(), see http://b/5694925.)
    LaunchInternal(environ, ipcfd, minidump_fd_, chrome_pak_fd, locale_pak_fd,
                   cmd_line, client_thread_id);
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
  virtual void OnSandboxedProcessStarted(BrowserThread::ID client_thread_id,
                                         base::ProcessHandle handle) {
    if (BrowserThread::CurrentlyOn(client_thread_id)) {
      // This is always invoked on the UI thread which is commonly the
      // |client_thread_id| so we can shortcut one PostTask.
      Notify(handle);
    } else {
      BrowserThread::PostTask(
          client_thread_id, FROM_HERE,
          base::Bind(
              &ChildProcessLauncher::Context::Notify,
              make_scoped_refptr(this),
              handle));
    }
  }

  int minidump_fd() const { return minidump_fd_; }

 private:
  friend class base::RefCountedThreadSafe<ChildProcessLauncher::Context>;
  friend class ChildProcessLauncher;

  virtual ~Context() {
    Terminate();
  }

  void LaunchInternal(
      const base::environment_vector& env,
      int ipcfd,
      int minidump_fd,
      int chrome_pak_fd,
      int locale_pak_fd,
      CommandLine* cmd_line,
      BrowserThread::ID client_thread_id) {
    scoped_ptr<CommandLine> cmd_line_deleter(cmd_line);
    // We need to close the client end of the IPC channel to reliably detect
    // child termination. We will close this fd after we create the child
    // process.
    ipcfd_ = ipcfd;
    content::browser::android::StartSandboxedProcess(
        cmd_line->argv(), ipcfd, minidump_fd, chrome_pak_fd, locale_pak_fd,
        base::Bind(&ChildProcessLauncher::Context::OnSandboxedProcessStarted,
            make_scoped_refptr(this), client_thread_id));
  }

  void Notify(base::ProcessHandle handle) {
    // Setup the ipcfd closer.
    file_util::ScopedFD ipcfd_closer(&ipcfd_);
    starting_ = false;
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

  static void SetProcessBackgrounded(base::ProcessHandle handle,
                                     bool background) {
    base::Process process(handle);
    process.SetProcessBackgrounded(background);
  }

  Client* client_;
  BrowserThread::ID client_thread_id_;
  base::Process process_;
  base::TerminationStatus termination_status_;
  int exit_code_;
  // Controls whether the child process should be terminated on browser
  // shutdown. Default behavior is to terminate the child.
  bool terminate_child_on_shutdown_;

  int minidump_fd_;

  // The fd to close after creating the process.
  int ipcfd_;
  bool starting_;
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
  return context_->starting_;
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
          GetHandle(),
          background));
}

void ChildProcessLauncher::SetTerminateChildOnShutdown(
  bool terminate_on_shutdown) {
  if (context_)
    context_->set_terminate_child_on_shutdown(terminate_on_shutdown);
}

int ChildProcessLauncher::GetMinidumpFD() {
  return context_->minidump_fd();
}
