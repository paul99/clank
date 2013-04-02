// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/basic_desktop_environment.h"

#include "base/logging.h"
#include "media/video/capture/screen/screen_capturer.h"
#include "remoting/host/audio_capturer.h"
#include "remoting/host/event_executor.h"

namespace remoting {

BasicDesktopEnvironment::BasicDesktopEnvironment(bool use_x_damage)
    : use_x_damage_(use_x_damage) {
}

BasicDesktopEnvironment::~BasicDesktopEnvironment() {
  DCHECK(CalledOnValidThread());
}

scoped_ptr<AudioCapturer> BasicDesktopEnvironment::CreateAudioCapturer(
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner) {
  DCHECK(CalledOnValidThread());

  return AudioCapturer::Create();
}

scoped_ptr<EventExecutor> BasicDesktopEnvironment::CreateEventExecutor(
    scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner) {
  DCHECK(CalledOnValidThread());

  return EventExecutor::Create(input_task_runner, ui_task_runner);
}

scoped_ptr<media::ScreenCapturer> BasicDesktopEnvironment::CreateVideoCapturer(
    scoped_refptr<base::SingleThreadTaskRunner> capture_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner) {
  DCHECK(CalledOnValidThread());

#if defined(OS_LINUX)
  return media::ScreenCapturer::CreateWithXDamage(use_x_damage_);
#else  // !defined(OS_LINUX)
  return media::ScreenCapturer::Create();
#endif  // !defined(OS_LINUX)
}

BasicDesktopEnvironmentFactory::BasicDesktopEnvironmentFactory(
    bool use_x_damage)
    : use_x_damage_(use_x_damage) {
}

BasicDesktopEnvironmentFactory::~BasicDesktopEnvironmentFactory() {
}

scoped_ptr<DesktopEnvironment> BasicDesktopEnvironmentFactory::Create(
    const std::string& client_jid,
    const base::Closure& disconnect_callback) {
  return scoped_ptr<DesktopEnvironment>(
      new BasicDesktopEnvironment(use_x_damage_));
}

bool BasicDesktopEnvironmentFactory::SupportsAudioCapture() const {
  return AudioCapturer::IsSupported();
}

}  // namespace remoting
