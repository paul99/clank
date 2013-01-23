// Copyright (c) 2010 Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// The ExceptionHandler object installs signal handlers for a number of
// signals. We rely on the signal handler running on the thread which crashed
// in order to identify it. This is true of the synchronous signals (SEGV etc),
// but not true of ABRT. Thus, if you send ABRT to yourself in a program which
// uses ExceptionHandler, you need to use tgkill to direct it to the current
// thread.
//
// The signal flow looks like this:
//
//   SignalHandler (uses a global stack of ExceptionHandler objects to find
//        |         one to handle the signal. If the first rejects it, try
//        |         the second etc...)
//        V
//   HandleSignal ----------------------------| (clones a new process which
//        |                                   |  shares an address space with
//   (wait for cloned                         |  the crashed process. This
//     process)                               |  allows us to ptrace the crashed
//        |                                   |  process)
//        V                                   V
//   (set signal handler to             ThreadEntry (static function to bounce
//    SIG_DFL and rethrow,                    |      back into the object)
//    killing the crashed                     |
//    process)                                V
//                                          DoDump  (writes minidump)
//                                            |
//                                            V
//                                         sys_exit
//

// This code is a little fragmented. Different functions of the ExceptionHandler
// class run in a number of different contexts. Some of them run in a normal
// context and are easy to code, others run in a compromised context and the
// restrictions at the top of minidump_writer.cc apply: no libc and use the
// alternative malloc. Each function should have comment above it detailing the
// context which it runs in.

#include "client/linux/handler/exception_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#if !defined(__ANDROID__)
#include <sys/signal.h>
#include <sys/ucontext.h>
#include <sys/user.h>
#include <ucontext.h>
#include <linux/unistd.h>
#endif

#include <algorithm>
#include <utility>
#include <vector>

#include "common/linux/linux_libc_support.h"
#include "common/memory.h"
#include "client/linux/minidump_writer/linux_dumper.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "common/linux/guid_creator.h"
#include "common/linux/eintr_wrapper.h"

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include "linux/sched.h"

#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif

// A wrapper for the tgkill syscall: send a signal to a specific thread.
static int tgkill(pid_t tgid, pid_t tid, int sig) {
  return syscall(__NR_tgkill, tgid, tid, sig);
  return 0;
}

namespace google_breakpad {

namespace {
// The list of signals which we consider to be crashes. The default action for
// all these signals must be Core (see man 7 signal) because we rethrow the
// signal after handling it and expect that it'll be fatal.
static const int kExceptionSignals[] = {
  SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS
};
const int kNumHandledSignals = sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);
struct sigaction old_handlers[kNumHandledSignals] = {0};
bool handlers_installed = false;

// Create an alternative stack to run the signal handlers on. This is done since
// the signal might have been caused by a stack overflow.
bool CreateAlternateStack() {
  // SIGSTKSZ may be too small to prevent the signal handlers from overrunning
  // the alternative stack, use this instead.
  static const unsigned kSigStackSize = std::max(8192, SIGSTKSZ);

  stack_t stack;
  // Only set an alternative stack if there isn't already one, or if the current
  // one is too small.
  if (sys_sigaltstack(NULL, &stack) == -1 || !stack.ss_sp ||
      stack.ss_size < kSigStackSize) {
    memset(&stack, 0, sizeof(stack));
    stack.ss_sp = malloc(kSigStackSize);
    stack.ss_size = kSigStackSize;

    if (sys_sigaltstack(&stack, NULL) == -1)
      return false;
  }
}

}  // namespace

// We can stack multiple exception handlers. In that case, this is the global
// which holds the stack.
std::vector<ExceptionHandler*>* ExceptionHandler::handler_stack_ = NULL;
pthread_mutex_t ExceptionHandler::handler_stack_mutex_ =
    PTHREAD_MUTEX_INITIALIZER;

// Runs before crashing: normal context.
ExceptionHandler::ExceptionHandler(const std::string &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void *callback_context,
                                   bool install_handler)
  : filter_(filter),
    callback_(callback),
    callback_context_(callback_context),
    minidump_descriptor_(dump_path.c_str()),
    handler_installed_(install_handler)
{
  Init(dump_path, -1);
}

ExceptionHandler::ExceptionHandler(const int minidump_fd,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void *callback_context,
                                   bool install_handler)
  : filter_(filter),
    callback_(callback),
    callback_context_(callback_context),
    minidump_descriptor_(minidump_fd),
    handler_installed_(install_handler) {
  Init(std::string(), -1);

  // The next call is needed to set next_minidump_id_c_ that is passed to the
  // callback.
  UpdateNextID();
}

ExceptionHandler::ExceptionHandler(const std::string &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void* callback_context,
                                   bool install_handler,
                                   const int server_fd)
  : filter_(filter),
    callback_(callback),
    callback_context_(callback_context),
    minidump_descriptor_(dump_path.c_str()),
    handler_installed_(install_handler)
{
  Init(dump_path, server_fd);
}

// Runs before crashing: normal context.
ExceptionHandler::~ExceptionHandler() {
  pthread_mutex_lock(&handler_stack_mutex_);
  std::vector<ExceptionHandler*>::iterator handler =
      std::find(handler_stack_->begin(), handler_stack_->end(), this);
  handler_stack_->erase(handler);
  if (handler_stack_->empty()) {
    RestoreHandlersLocked();
  }
  pthread_mutex_unlock(&handler_stack_mutex_);
}

void ExceptionHandler::Init(const std::string &dump_path,
                            const int server_fd)
{
  crash_handler_ = NULL;
  if (0 <= server_fd)
    crash_generation_client_.reset(CrashGenerationClient::TryCreate(server_fd));

  if (!IsOutOfProcess() && !dump_path.empty())
    set_dump_path(dump_path);

  if (minidump_descriptor_.IsFD()) {
    minidump_fd_duplicate_ = dup(minidump_descriptor_.fd());
  }  else {
    minidump_fd_duplicate_ = 0;
  }
  pthread_mutex_lock(&handler_stack_mutex_);
  if (handler_stack_ == NULL)
    handler_stack_ = new std::vector<ExceptionHandler *>;
  handler_stack_->push_back(this);
  if (handler_installed_) {
    CreateAlternateStack();
    InstallHandlersLocked();
  }
  pthread_mutex_unlock(&handler_stack_mutex_);
}

// Runs before crashing: normal context.
bool ExceptionHandler::InstallHandlersLocked() {
  if (handlers_installed)
    return false;

  // Fail if unable to store all the old handlers.
  for (unsigned i = 0; i < kNumHandledSignals; ++i) {
    if (sigaction(kExceptionSignals[i], NULL, &old_handlers[i]) == -1)
      return false;
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sigemptyset(&sa.sa_mask);

  // Mask all exception signals when we're handling one of them.
  for (unsigned i = 0; i < kNumHandledSignals; ++i)
    sigaddset(&sa.sa_mask, kExceptionSignals[i]);

  sa.sa_sigaction = SignalHandler;
  sa.sa_flags = SA_ONSTACK | SA_SIGINFO;

  for (unsigned i = 0; i < kNumHandledSignals; ++i) {
    if (sigaction(kExceptionSignals[i], &sa, NULL) == -1) {
      // At this point it is impractical to back out changes, and so failure to
      // install a signal is intentionally ignored.
    }
  }
  handlers_installed = true;
  return true;
}

// This function runs in a compromised context: see the top of the file.
// Runs on the crashing thread.
void ExceptionHandler::RestoreHandlersLocked() {
  if (!handlers_installed)
    return;

  for (unsigned i = 0; i < kNumHandledSignals; ++i) {
    if (sigaction(kExceptionSignals[i], &old_handlers[i], NULL) == -1) {
      signal(kExceptionSignals[i], SIG_DFL);
    }
  }
  handlers_installed = false;
}

// Runs before crashing: normal context.
void ExceptionHandler::UpdateNextID() {
  GUID guid;
  char guid_str[kGUIDStringLength + 1];
  if (CreateGUID(&guid) && GUIDToString(&guid, guid_str, sizeof(guid_str))) {
    next_minidump_id_ = guid_str;
    next_minidump_id_c_ = next_minidump_id_.c_str();

    // When using a file descriptor, we don't deal with names.
    if (minidump_descriptor_.IsFD())
      return;

    char minidump_path[PATH_MAX];
    snprintf(minidump_path, sizeof(minidump_path), "%s/%s.dmp",
             minidump_descriptor_.path(),
             guid_str);

    next_minidump_path_ = minidump_path;
    next_minidump_path_c_ = next_minidump_path_.c_str();
  }
}

// void ExceptionHandler::set_crash_handler(HandlerCallback callback) {
//   crash_handler_ = callback;
// }

// This function runs in a compromised context: see the top of the file.
// Runs on the crashing thread.
// static
void ExceptionHandler::SignalHandler(int sig, siginfo_t* info, void* uc) {
  // A misbehaving library may have saved this handler and restored it using
  // signal instead of sigaction. In that case, the info and uc arguments that
  // are passed to this handler are junk. If that happened, the current signal
  // handler (i.e. this one) will not have the SA_SIGINFO flag set. If that is
  // the case, then reinstall this signal handler with the correct flags and
  // return. Then, sig will become unmasked and retriggered. This signal handler
  // will be called again, this time with valid info and uc arguments.
  struct sigaction this_handler;
  if (sigaction(sig, NULL, &this_handler) == -1) {
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad",
                        "Getting handler failed, assuming valid flags.");
  } else if (!(this_handler.sa_flags & SA_SIGINFO)){
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad",
                        "Signal handler does not have SIG_INFO flag."
                        "Re-installing signal handler.");
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    // Mask all exception signals when we're handling one of them.
    for (unsigned i = 0; i < kNumHandledSignals; ++i)
      sigaddset(&sa.sa_mask, kExceptionSignals[i]);

    sa.sa_sigaction = SignalHandler;
    sa.sa_flags = SA_ONSTACK | SA_SIGINFO;
    if (sigaction(sig, &sa, NULL) == -1) {
      // If reinstallation of this signal handler failed, set the handler to the
      // default so that this signal handler does not repeatedly get called
      // without the SA_SIGINFO flag set.
      signal(sig, SIG_DFL);
    }
    return;
  }

  // All the exception signals are blocked at this point.
  pthread_mutex_lock(&handler_stack_mutex_);

  bool handled = false;
  for (int i = handler_stack_->size() - 1; !handled && i >= 0; --i) {
    handled = (*handler_stack_)[i]->HandleSignal(sig, info, uc);
  }

  // Upon returning fromt this signal handler, sig will become unmasked and then
  // it will be retriggered. If one of the ExceptionHandlers handled it
  // successfully, restore the default handler. Otherwise, restore the
  // previously installed handler. Then, when the signal is retriggered, it will
  // be delivered to the appropriate handler.
  if (handled) {
    signal(sig, SIG_DFL);
  } else {
    RestoreHandlersLocked();
  }

  pthread_mutex_unlock(&handler_stack_mutex_);

  if (info->si_pid) {
    // This signal was triggered by somebody sending us the signal with kill().
    // In order to retrigger it, we have to queue a new signal by calling
    // kill() ourselves.
    if (tgkill(getpid(), syscall(__NR_gettid), sig) < 0) {
      // If we failed to kill ourselves (e.g. because a sandbox disallows us
      // to do so), we instead resort to terminating our process. This will
      // result in an incorrect exit code.
      _exit(1);
    }
  } else {
    // This was a synchronous signal triggered by a hard fault (e.g. SIGSEGV).
    // No need to reissue the signal. It will automatically trigger again,
    // when we return from the signal handler.
  }
}

struct ThreadArgument {
  pid_t pid;  // the crashing process
  const MinidumpDescriptor* minidump_descriptor;
  ExceptionHandler* handler;
  const void* context;  // a CrashContext structure
  size_t context_size;
};

// This is the entry function for the cloned process. We are in a compromised
// context here: see the top of the file.
// static
int ExceptionHandler::ThreadEntry(void *arg) {
  const ThreadArgument *thread_arg = reinterpret_cast<ThreadArgument*>(arg);

  // Block here until the crashing process unblocks us when
  // we're allowed to use ptrace
  thread_arg->handler->WaitForContinueSignal();
  return thread_arg->handler->DoDump(thread_arg->pid, thread_arg->context,
                                     thread_arg->context_size) == false;
}

// This function runs in a compromised context: see the top of the file.
// Runs on the crashing thread.
bool ExceptionHandler::HandleSignal(int sig, siginfo_t* info, void* uc) {
  if (filter_ && !filter_(callback_context_))
    return false;

  // Allow ourselves to be dumped if the signal is trusted.
  bool signal_trusted = info->si_code > 0;
  bool signal_pid_trusted = info->si_code == SI_USER ||
      info->si_code == SI_TKILL;
  if (signal_trusted || (signal_pid_trusted && info->si_pid == getpid())) {
    sys_prctl(PR_SET_DUMPABLE, 1);
  }
  CrashContext context;
  memcpy(&context.siginfo, info, sizeof(siginfo_t));
  memcpy(&context.context, uc, sizeof(struct ucontext));
#if !defined(__ARM_EABI__)
  // FP state is not part of user ABI on ARM Linux.
  struct ucontext *uc_ptr = (struct ucontext*)uc;
  if (uc_ptr->uc_mcontext.fpregs) {
    memcpy(&context.float_state,
           uc_ptr->uc_mcontext.fpregs,
           sizeof(context.float_state));
  }
#endif
  context.tid = syscall(__NR_gettid);
  if (crash_handler_ != NULL) {
    if (crash_handler_(&context, sizeof(context),
                       callback_context_)) {
      return true;
    }
  }
  return GenerateDump(&context);
}

// This function may run in a compromised context: see the top of the file.
bool ExceptionHandler::GenerateDump(CrashContext *context) {
  if (IsOutOfProcess())
    return crash_generation_client_->RequestDump(context, sizeof(*context));

  static const unsigned kChildStackSize = 8000;
  PageAllocator allocator;
  uint8_t* stack = (uint8_t*) allocator.Alloc(kChildStackSize);
  if (!stack)
    return false;
  // clone() needs the top-most address. (scrub just to be safe)
  stack += kChildStackSize;
  my_memset(stack - 16, 0, 16);

  ThreadArgument thread_arg;
  thread_arg.handler = this;
  thread_arg.minidump_descriptor = &minidump_descriptor_;
  thread_arg.pid = getpid();
  thread_arg.context = context;
  thread_arg.context_size = sizeof(*context);

  // We need to explicitly enable ptrace of parent processes on some
  // kernels, but we need to know the PID of the cloned process before we
  // can do this. Create a pipe here which we can use to block the
  // cloned process after creating it, until we have explicitly enabled ptrace
  if(sys_pipe(fdes) == -1) {
    // Creating the pipe failed. We'll log an error but carry on anyway,
    // as we'll probably still get a useful crash report. All that will happen
    // is the write() and read() calls will fail with EBADF
    static const char no_pipe_msg[] = "ExceptionHandler::GenerateDump \
                                       sys_pipe failed:";
#if defined(__ANDROID__)
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad", no_pipe_msg);
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad", strerror(errno));
#else
    sys_write(2, no_pipe_msg, sizeof(no_pipe_msg) - 1);
    sys_write(2, strerror(errno), strlen(strerror(errno)));
    sys_write(2, "\n", 1);
#endif
  }
// clone is not available in Android NDK for x86.
// We have to fallback to using sys_clone.
// TODO(shashishekhar): Need to verify if a SIGILL is propogated for x86 also.
#if defined(__ANDROID__) && !defined(__i386__)
  // If sys_clone is used on Android, then a SIGILL will be propogated after the
  // successful conclusion of ThreadEntry.
  const pid_t child = clone(
      ThreadEntry, stack, CLONE_FILES | CLONE_FS | CLONE_UNTRACED,
      &thread_arg);
#else
  const pid_t child = sys_clone(
      ThreadEntry, stack, CLONE_FILES | CLONE_FS | CLONE_UNTRACED,
      &thread_arg, NULL, NULL, NULL);
#endif
  int r, status;
  // Allow the child to ptrace us
  sys_prctl(PR_SET_PTRACER, child);
  SendContinueSignalToChild();

  do {
    // Should work because in process_util_posix we have already set the SIGCLD
    // to not be ignored.
    r = sys_waitpid(child, &status, __WALL);
  } while (r == -1 && errno == EINTR);

  sys_close(fdes[0]);
  sys_close(fdes[1]);

  if (r == -1) {
    static const char msg[] = "ExceptionHandler::GenerateDump waitpid failed:";
#if defined(__ANDROID__)
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad", msg);
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad", strerror(errno));
#else
    sys_write(2, msg, sizeof(msg) - 1);
    sys_write(2, strerror(errno), strlen(strerror(errno)));
    sys_write(2, "\n", 1);
#endif
  }

  bool success = r != -1 && (WIFEXITED(status) && (WEXITSTATUS(status) == 0));
  if (callback_) {
    if (minidump_descriptor_.IsFD()) {
      // The generation of the minidump closes the FD. Make sure we provide a
      // valid one.
      minidump_descriptor_.set_fd(minidump_fd_duplicate_);
    }
    success = callback_(minidump_descriptor_, next_minidump_id_c_,
                        callback_context_, success);
  }
  return success;
}

// This function runs in a compromised context: see the top of the file.
void ExceptionHandler::SendContinueSignalToChild() {
  static const char okToContinueMessage = 'a';
  int r;
  r = HANDLE_EINTR(sys_write(fdes[1], &okToContinueMessage, sizeof(char)));
  if(r == -1) {
    static const char msg[] = "ExceptionHandler::SendContinueSignalToChild \
                               sys_write failed:";
#if defined(__ANDROID__)
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad", msg);
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad", strerror(errno));
#else
    sys_write(2, msg, sizeof(msg) - 1);
    sys_write(2, strerror(errno), strlen(strerror(errno)));
    sys_write(2, "\n", 1);
#endif
  }
}

// This function runs in a compromised context: see the top of the file.
// Runs on the cloned process.
void ExceptionHandler::WaitForContinueSignal() {
  int r;
  char receivedMessage;
  r = HANDLE_EINTR(sys_read(fdes[0], &receivedMessage, sizeof(char)));
  if(r == -1) {
    static const char msg[] = "ExceptionHandler::WaitForContinueSignal \
                               sys_read failed:";
#if defined(__ANDROID__)
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad", msg);
    __android_log_write(ANDROID_LOG_WARN, "google-breakpad", strerror(errno));
#else
    sys_write(2, msg, sizeof(msg) - 1);
    sys_write(2, strerror(errno), strlen(strerror(errno)));
    sys_write(2, "\n", 1);
#endif
  }
}

// This function runs in a compromised context: see the top of the file.
// Runs on the cloned process.
bool ExceptionHandler::DoDump(pid_t crashing_process, const void* context,
                              size_t context_size) {
  return google_breakpad::WriteMinidump(minidump_descriptor_,
                                        crashing_process,
                                        context,
                                        context_size,
                                        mapping_list_);
}

// static
bool ExceptionHandler::WriteMinidump(const std::string &dump_path,
                                     MinidumpCallback callback,
                                     void* callback_context) {
  ExceptionHandler eh(dump_path, NULL, callback, callback_context, false);
  return eh.WriteMinidump();
}

bool ExceptionHandler::WriteMinidump() {
  // Android NDK does not have support for getcontext.
  // For now disable this for Android.
#if !( defined(__ARM_EABI__) || defined(__ANDROID__) )
  // Allow ourselves to be dumped.
  sys_prctl(PR_SET_DUMPABLE, 1);

  CrashContext context;
  int getcontext_result = getcontext(&context.context);
  if (getcontext_result)
    return false;
  memcpy(&context.float_state, context.context.uc_mcontext.fpregs,
         sizeof(context.float_state));
  context.tid = sys_gettid();

  bool success = GenerateDump(&context);
  UpdateNextID();
  return success;
#else
  return false;
#endif  // !defined(__ARM_EABI__)
}

void ExceptionHandler::AddMappingInfo(const std::string& name,
                                      const u_int8_t identifier[sizeof(MDGUID)],
                                      uintptr_t start_address,
                                      size_t mapping_size,
                                      size_t file_offset) {
  MappingInfo info;
  info.start_addr = start_address;
  info.size = mapping_size;
  info.offset = file_offset;
  strncpy(info.name, name.c_str(), sizeof(info.name) - 1);
  info.name[sizeof(info.name) - 1] = '\0';

  MappingEntry mapping;
  mapping.first = info;
  memcpy(mapping.second, identifier, sizeof(MDGUID));
  mapping_list_.push_back(mapping);
}

}  // namespace google_breakpad
