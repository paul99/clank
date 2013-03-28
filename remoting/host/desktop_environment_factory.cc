// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/desktop_environment_factory.h"

#include "base/single_thread_task_runner.h"
#include "remoting/host/audio_capturer.h"
#include "remoting/host/chromoting_host_context.h"
#include "remoting/host/client_session.h"
#include "remoting/host/desktop_environment.h"
#include "remoting/host/event_executor.h"
#include "remoting/host/video_frame_capturer.h"

namespace remoting {

DesktopEnvironmentFactory::DesktopEnvironmentFactory(
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner)
    : input_task_runner_(input_task_runner),
      ui_task_runner_(ui_task_runner) {
}

DesktopEnvironmentFactory::~DesktopEnvironmentFactory() {
}

scoped_ptr<DesktopEnvironment> DesktopEnvironmentFactory::Create(
    ClientSession* client) {
  scoped_ptr<DesktopEnvironment> environment(new DesktopEnvironment(
      AudioCapturer::Create(),
      EventExecutor::Create(input_task_runner_, ui_task_runner_),
      VideoFrameCapturer::Create()));
  return environment.Pass();
}

bool DesktopEnvironmentFactory::SupportsAudioCapture() const {
  return AudioCapturer::IsSupported();
}

}  // namespace remoting
