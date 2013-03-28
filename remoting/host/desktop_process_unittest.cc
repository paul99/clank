// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/desktop_process.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_message.h"
#include "remoting/base/auto_thread.h"
#include "remoting/base/auto_thread_task_runner.h"
#include "remoting/host/chromoting_messages.h"
#include "remoting/host/desktop_process.h"
#include "remoting/host/host_exit_codes.h"
#include "testing/gmock_mutant.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::AnyNumber;
using testing::InSequence;

namespace remoting {

namespace {

class MockDaemonListener : public IPC::Listener {
 public:
  MockDaemonListener() {}
  virtual ~MockDaemonListener() {}

  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  MOCK_METHOD1(OnDesktopAttached, void(IPC::PlatformFileForTransit));
  MOCK_METHOD1(OnChannelConnected, void(int32));
  MOCK_METHOD0(OnChannelError, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDaemonListener);
};

class MockNetworkListener : public IPC::Listener {
 public:
  MockNetworkListener() {}
  virtual ~MockNetworkListener() {}

  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  MOCK_METHOD1(OnDesktopAttached, void(IPC::PlatformFileForTransit));
  MOCK_METHOD1(OnChannelConnected, void(int32));
  MOCK_METHOD0(OnChannelError, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockNetworkListener);
};

bool MockDaemonListener::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MockDaemonListener, message)
    IPC_MESSAGE_HANDLER(ChromotingDesktopDaemonMsg_DesktopAttached,
                        OnDesktopAttached)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  EXPECT_TRUE(handled);
  return handled;
}

bool MockNetworkListener::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;

  // TODO(alexeypa): handle received messages here.

  EXPECT_TRUE(handled);
  return handled;
}

}  // namespace

class DesktopProcessTest : public testing::Test {
 public:
  DesktopProcessTest();
  virtual ~DesktopProcessTest();

  // testing::Test overrides
  virtual void SetUp() OVERRIDE;
  virtual void TearDown() OVERRIDE;

  // MockDaemonListener mocks
  void ConnectNetworkChannel(IPC::PlatformFileForTransit desktop_process);
  void OnDesktopAttached(IPC::PlatformFileForTransit desktop_process);

  // Disconnects the daemon-to-desktop channel causing the desktop process to
  // exit.
  void DisconnectChannels();

  // Runs the desktop process code in a separate thread.
  void RunDesktopProcess();

  // Creates the desktop process and sends a crash request to it.
  void RunDeathTest();

  // Sends a crash request to the desktop process.
  void SendCrashRequest();

 protected:
  // The daemon's end of the daemon-to-desktop channel.
  scoped_ptr<IPC::ChannelProxy> daemon_channel_;

  // Delegate that is passed to |daemon_channel_|.
  MockDaemonListener daemon_listener_;

  // Runs the daemon's end of the channel.
  MessageLoop message_loop_;

  scoped_refptr<AutoThreadTaskRunner> io_task_runner_;

  // The network's end of the network-to-desktop channel.
  scoped_ptr<IPC::ChannelProxy> network_channel_;

  // Delegate that is passed to |network_channel_|.
  MockNetworkListener network_listener_;
};


DesktopProcessTest::DesktopProcessTest()
    : message_loop_(MessageLoop::TYPE_UI) {
}

DesktopProcessTest::~DesktopProcessTest() {
}

void DesktopProcessTest::SetUp() {
}

void DesktopProcessTest::TearDown() {
}

void DesktopProcessTest::ConnectNetworkChannel(
    IPC::PlatformFileForTransit desktop_process) {

#if defined(OS_POSIX)
  IPC::ChannelHandle channel_handle("", desktop_process);
#elif defined(OS_WIN)
  IPC::ChannelHandle channel_handle(desktop_process);
#endif  // defined(OS_WIN)

  daemon_channel_.reset(new IPC::ChannelProxy(
      channel_handle,
      IPC::Channel::MODE_CLIENT,
      &network_listener_,
      io_task_runner_));
}

void DesktopProcessTest::OnDesktopAttached(
    IPC::PlatformFileForTransit desktop_process) {
#if defined(OS_POSIX)
    DCHECK(desktop_process.auto_close);

    base::ClosePlatformFile(desktop_process.fd);
#endif  // defined(OS_POSIX)
}

void DesktopProcessTest::DisconnectChannels() {
  daemon_channel_.reset();
  network_channel_.reset();
  io_task_runner_ = NULL;
}

void DesktopProcessTest::RunDesktopProcess() {
  base::RunLoop run_loop;
  base::Closure quit_ui_task_runner = base::Bind(
      base::IgnoreResult(&base::SingleThreadTaskRunner::PostTask),
      message_loop_.message_loop_proxy(),
      FROM_HERE, run_loop.QuitClosure());
  scoped_refptr<AutoThreadTaskRunner> ui_task_runner = new AutoThreadTaskRunner(
      message_loop_.message_loop_proxy(), quit_ui_task_runner);

  io_task_runner_ = AutoThread::CreateWithType("IPC thread", ui_task_runner,
                                               MessageLoop::TYPE_IO);

  std::string channel_name = IPC::Channel::GenerateUniqueRandomChannelID();
  daemon_channel_.reset(new IPC::ChannelProxy(
      IPC::ChannelHandle(channel_name),
      IPC::Channel::MODE_SERVER,
      &daemon_listener_,
      io_task_runner_));

  DesktopProcess desktop_process(ui_task_runner, channel_name);
  EXPECT_TRUE(desktop_process.Start());

  ui_task_runner = NULL;
  run_loop.Run();
}

void DesktopProcessTest::RunDeathTest() {
  InSequence s;
  EXPECT_CALL(daemon_listener_, OnChannelConnected(_));
  EXPECT_CALL(daemon_listener_, OnDesktopAttached(_))
      .WillOnce(DoAll(
          Invoke(this, &DesktopProcessTest::OnDesktopAttached),
          InvokeWithoutArgs(this, &DesktopProcessTest::SendCrashRequest)));

  RunDesktopProcess();
}

void DesktopProcessTest::SendCrashRequest() {
  tracked_objects::Location location = FROM_HERE;
  daemon_channel_->Send(new ChromotingDaemonDesktopMsg_Crash(
      location.function_name(), location.file_name(), location.line_number()));
}

// Launches the desktop process and waits when it connects back.
TEST_F(DesktopProcessTest, Basic) {
  InSequence s;
  EXPECT_CALL(daemon_listener_, OnChannelConnected(_));
  EXPECT_CALL(daemon_listener_, OnDesktopAttached(_))
      .WillOnce(DoAll(
          Invoke(this, &DesktopProcessTest::OnDesktopAttached),
          InvokeWithoutArgs(this, &DesktopProcessTest::DisconnectChannels)));

  RunDesktopProcess();
}

// Launches the desktop process and waits when it connects back.
TEST_F(DesktopProcessTest, ConnectNetworkChannel) {
  InSequence s;
  EXPECT_CALL(daemon_listener_, OnChannelConnected(_));
  EXPECT_CALL(daemon_listener_, OnDesktopAttached(_))
      .WillOnce(Invoke(this, &DesktopProcessTest::ConnectNetworkChannel));
  EXPECT_CALL(network_listener_, OnChannelConnected(_))
      .WillOnce(InvokeWithoutArgs(
          this, &DesktopProcessTest::DisconnectChannels));

  RunDesktopProcess();
}

// Run the desktop process and ask it to crash.
TEST_F(DesktopProcessTest, DeathTest) {
  testing::GTEST_FLAG(death_test_style) = "threadsafe";

  EXPECT_DEATH(RunDeathTest(), "");
}

}  // namespace remoting
