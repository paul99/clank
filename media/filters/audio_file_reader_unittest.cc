// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/md5.h"
#include "base/memory/scoped_ptr.h"
#include "base/sys_byteorder.h"
#include "build/build_config.h"
#include "media/base/audio_bus.h"
#include "media/base/decoder_buffer.h"
#include "media/base/test_data_util.h"
#include "media/filters/audio_file_reader.h"
#include "media/filters/in_memory_url_protocol.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class AudioFileReaderTest : public testing::Test {
 public:
  AudioFileReaderTest() {}
  virtual ~AudioFileReaderTest() {}

  void Initialize(const char* filename) {
    data_ = ReadTestDataFile(filename);
    protocol_.reset(new InMemoryUrlProtocol(
        data_->GetData(), data_->GetDataSize(), false));
    reader_.reset(new AudioFileReader(protocol_.get()));
  }

  // Reads and the entire file provided to Initialize().  If NULL is specified
  // for |audio_hash| MD5 checks are skipped.
  void ReadAndVerify(const char* audio_hash, int expected_frames) {
    scoped_ptr<AudioBus> decoded_audio_data = AudioBus::Create(
        reader_->channels(), reader_->number_of_frames());
    int actual_frames = reader_->Read(decoded_audio_data.get());
    ASSERT_LE(actual_frames, decoded_audio_data->frames());
    ASSERT_EQ(expected_frames, actual_frames);

    // TODO(dalecurtis): Audio decoded in float does not have a consistent hash
    // across platforms.  Fix this: http://crbug.com/168204
    if (!audio_hash)
      return;

    base::MD5Context md5_context;
    base::MD5Init(&md5_context);

    DCHECK_EQ(sizeof(float), sizeof(uint32));
    int channels = decoded_audio_data->channels();
    for (int ch = 0; ch < channels; ++ch) {
      float* channel = decoded_audio_data->channel(ch);
      for (int i = 0; i < actual_frames; ++i) {
        // Convert float to uint32 w/o conversion loss.
        uint32 frame = base::ByteSwapToLE32(bit_cast<uint32>(channel[i]));
        base::MD5Update(&md5_context, base::StringPiece(
            reinterpret_cast<char*>(&frame), sizeof(frame)));
      }
    }

    base::MD5Digest digest;
    base::MD5Final(&digest, &md5_context);
    EXPECT_EQ(audio_hash, base::MD5DigestToBase16(digest));
  }

  void RunTest(const char* fn, const char* hash, int channels, int sample_rate,
               base::TimeDelta duration, int frames, int trimmed_frames) {
    Initialize(fn);
    ASSERT_TRUE(reader_->Open());
    EXPECT_EQ(channels, reader_->channels());
    EXPECT_EQ(sample_rate, reader_->sample_rate());
    EXPECT_EQ(duration.InMicroseconds(), reader_->duration().InMicroseconds());
    EXPECT_EQ(frames, reader_->number_of_frames());
    ReadAndVerify(hash, trimmed_frames);
  }

  void RunTestFailingDemux(const char* fn) {
    Initialize(fn);
    EXPECT_FALSE(reader_->Open());
  }

  void RunTestFailingDecode(const char* fn) {
    Initialize(fn);
    EXPECT_TRUE(reader_->Open());
    scoped_ptr<AudioBus> decoded_audio_data = AudioBus::Create(
        reader_->channels(), reader_->number_of_frames());
    EXPECT_EQ(reader_->Read(decoded_audio_data.get()), 0);
  }

 protected:
  scoped_refptr<DecoderBuffer> data_;
  scoped_ptr<InMemoryUrlProtocol> protocol_;
  scoped_ptr<AudioFileReader> reader_;

  DISALLOW_COPY_AND_ASSIGN(AudioFileReaderTest);
};

TEST_F(AudioFileReaderTest, WithoutOpen) {
  Initialize("bear.ogv");
}

TEST_F(AudioFileReaderTest, InvalidFile) {
  RunTestFailingDemux("ten_byte_file");
}

TEST_F(AudioFileReaderTest, WithVideo) {
  RunTest("bear.ogv", NULL, 2, 44100,
          base::TimeDelta::FromMicroseconds(1011520), 44608, 44608);
}

TEST_F(AudioFileReaderTest, Vorbis) {
  RunTest("sfx.ogg", NULL, 1, 44100,
          base::TimeDelta::FromMicroseconds(350001), 15435, 15435);
}

TEST_F(AudioFileReaderTest, WaveU8) {
  RunTest("sfx_u8.wav", "d7e255a8e634fffdf9f744c5803632f8", 1, 44100,
          base::TimeDelta::FromMicroseconds(288414), 12719, 12719);
}

TEST_F(AudioFileReaderTest, WaveS16LE) {
  RunTest("sfx_s16le.wav", "2a5847207fdcba1c05e52f65ad010f66", 1, 44100,
          base::TimeDelta::FromMicroseconds(288414), 12719, 12719);
}

TEST_F(AudioFileReaderTest, WaveS24LE) {
  RunTest("sfx_s24le.wav", "66296b4ec633290581f9abf3c21cd5e7", 1, 44100,
          base::TimeDelta::FromMicroseconds(288414), 12719, 12719);
}

TEST_F(AudioFileReaderTest, WaveF32LE) {
  RunTest("sfx_f32le.wav", "66296b4ec633290581f9abf3c21cd5e7", 1, 44100,
          base::TimeDelta::FromMicroseconds(288414), 12719, 12719);
}

#if defined(GOOGLE_CHROME_BUILD) || defined(USE_PROPRIETARY_CODECS)
TEST_F(AudioFileReaderTest, MP3) {
  RunTest("sfx.mp3", NULL, 1, 44100,
          base::TimeDelta::FromMicroseconds(313470), 13824, 12719);
}

TEST_F(AudioFileReaderTest, AAC) {
  RunTest("sfx.m4a", NULL, 1, 44100,
          base::TimeDelta::FromMicroseconds(312001), 13759, 13312);
}

TEST_F(AudioFileReaderTest, MidStreamConfigChangesFail) {
  RunTestFailingDecode("midstream_config_change.mp3");
}
#endif

TEST_F(AudioFileReaderTest, VorbisInvalidChannelLayout) {
  RunTestFailingDemux("9ch.ogg");
}

TEST_F(AudioFileReaderTest, WaveValidFourChannelLayout) {
  RunTest("4ch.wav", "d40bb7dbe532b2f1cf2e3558e780caa2", 4, 44100,
          base::TimeDelta::FromMicroseconds(100001), 4410, 4410);
}

}  // namespace media
