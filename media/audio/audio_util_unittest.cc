// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "media/audio/audio_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// Number of samples in each audio array.
static const size_t kNumberOfSamples = 4;

namespace media {

TEST(AudioUtilTest, AdjustVolume_u8) {
  // Test AdjustVolume() on 8 bit samples.
  uint8 samples_u8[kNumberOfSamples] = { 4, 0x40, 0x80, 0xff };
  uint8 expected_u8[kNumberOfSamples] = { (4 - 128) / 2 + 128,
                                          (0x40 - 128) / 2 + 128,
                                          (0x80 - 128) / 2 + 128,
                                          (0xff - 128) / 2 + 128 };
  bool result_u8 = media::AdjustVolume(samples_u8, sizeof(samples_u8),
                                       1,  // channels.
                                       sizeof(samples_u8[0]),
                                       0.5f);
  EXPECT_TRUE(result_u8);
  int expected_test = memcmp(samples_u8, expected_u8, sizeof(expected_u8));
  EXPECT_EQ(0, expected_test);
}

TEST(AudioUtilTest, AdjustVolume_s16) {
  // Test AdjustVolume() on 16 bit samples.
  int16 samples_s16[kNumberOfSamples] = { -4, 0x40, -32768, 123 };
  int16 expected_s16[kNumberOfSamples] = { -1, 0x10, -8192, 30 };
  bool result_s16 = media::AdjustVolume(samples_s16, sizeof(samples_s16),
                                        2,  // channels.
                                        sizeof(samples_s16[0]),
                                        0.25f);
  EXPECT_TRUE(result_s16);
  int expected_test = memcmp(samples_s16, expected_s16, sizeof(expected_s16));
  EXPECT_EQ(0, expected_test);
}

TEST(AudioUtilTest, AdjustVolume_s16_zero) {
  // Test AdjustVolume() on 16 bit samples.
  int16 samples_s16[kNumberOfSamples] = { -4, 0x40, -32768, 123 };
  int16 expected_s16[kNumberOfSamples] = { 0, 0, 0, 0 };
  bool result_s16 = media::AdjustVolume(samples_s16, sizeof(samples_s16),
                                        2,  // channels.
                                        sizeof(samples_s16[0]),
                                        0.0f);
  EXPECT_TRUE(result_s16);
  int expected_test = memcmp(samples_s16, expected_s16, sizeof(expected_s16));
  EXPECT_EQ(0, expected_test);
}

TEST(AudioUtilTest, AdjustVolume_s16_one) {
  // Test AdjustVolume() on 16 bit samples.
  int16 samples_s16[kNumberOfSamples] = { -4, 0x40, -32768, 123 };
  int16 expected_s16[kNumberOfSamples] = { -4, 0x40, -32768, 123 };
  bool result_s16 = media::AdjustVolume(samples_s16, sizeof(samples_s16),
                                        2,  // channels.
                                        sizeof(samples_s16[0]),
                                        1.0f);
  EXPECT_TRUE(result_s16);
  int expected_test = memcmp(samples_s16, expected_s16, sizeof(expected_s16));
  EXPECT_EQ(0, expected_test);
}

TEST(AudioUtilTest, AdjustVolume_s32) {
  // Test AdjustVolume() on 32 bit samples.
  int32 samples_s32[kNumberOfSamples] = { -4, 0x40, -32768, 123 };
  int32 expected_s32[kNumberOfSamples] = { -1, 0x10, -8192, 30 };
  bool result_s32 = media::AdjustVolume(samples_s32, sizeof(samples_s32),
                                        4,  // channels.
                                        sizeof(samples_s32[0]),
                                        0.25f);
  EXPECT_TRUE(result_s32);
  int expected_test = memcmp(samples_s32, expected_s32, sizeof(expected_s32));
  EXPECT_EQ(0, expected_test);
}

}  // namespace media
