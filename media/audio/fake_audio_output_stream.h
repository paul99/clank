// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_FAKE_AUDIO_OUTPUT_STREAM_H_
#define MEDIA_AUDIO_FAKE_AUDIO_OUTOUT_STREAM_H_

#include "base/cancelable_callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_parameters.h"

namespace media {

class AudioManagerBase;

// A fake implementation of AudioOutputStream.  Used for testing and when a real
// audio output device is unavailable or refusing output (e.g. remote desktop).
class MEDIA_EXPORT FakeAudioOutputStream : public AudioOutputStream {
 public:
  static AudioOutputStream* MakeFakeStream(AudioManagerBase* manager,
                                           const AudioParameters& params);

  // AudioOutputStream implementation.
  virtual bool Open() OVERRIDE;
  virtual void Start(AudioSourceCallback* callback) OVERRIDE;
  virtual void Stop() OVERRIDE;
  virtual void SetVolume(double volume) OVERRIDE;
  virtual void GetVolume(double* volume) OVERRIDE;
  virtual void Close() OVERRIDE;

 private:
  FakeAudioOutputStream(AudioManagerBase* manager,
                        const AudioParameters& params);
  virtual ~FakeAudioOutputStream();

  // Task that regularly calls |callback_->OnMoreData()| according to the
  // playback rate as determined by the audio parameters given during
  // construction.  Runs on AudioManager's message loop.
  void OnMoreDataTask();

  AudioManagerBase* audio_manager_;
  AudioSourceCallback* callback_;
  scoped_ptr<AudioBus> audio_bus_;
  base::TimeDelta buffer_duration_;
  base::Time next_read_time_;

  // Used to post delayed tasks to the AudioThread that we can cancel.
  base::CancelableClosure on_more_data_cb_;

  DISALLOW_COPY_AND_ASSIGN(FakeAudioOutputStream);
};

}  // namespace media

#endif  // MEDIA_AUDIO_FAKE_AUDIO_OUTPUT_STREAM_H_
