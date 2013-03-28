// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/video_frame_capturer.h"

#include <ApplicationServices/ApplicationServices.h>

#include <ostream>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "remoting/base/capture_data.h"
#include "remoting/host/host_mock_objects.h"
#include "remoting/proto/control.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;

namespace remoting {

// Verify that the OS is at least Snow Leopard (10.6).
// Chromoting doesn't support 10.5 or earlier.
bool CheckSnowLeopard() {
  long minorVersion, majorVersion;
  Gestalt(gestaltSystemVersionMajor, &majorVersion);
  Gestalt(gestaltSystemVersionMinor, &minorVersion);
  return majorVersion == 10 && minorVersion > 5;
}

class VideoFrameCapturerMacTest : public testing::Test {
 public:
  // Verifies that the whole screen is initially dirty.
  void CaptureDoneCallback1(scoped_refptr<CaptureData> capture_data);

  // Verifies that a rectangle explicitly marked as dirty is propagated
  // correctly.
  void CaptureDoneCallback2(scoped_refptr<CaptureData> capture_data);

 protected:
  virtual void SetUp() OVERRIDE {
    capturer_ = VideoFrameCapturer::Create();
  }

  void AddDirtyRect() {
    SkIRect rect = SkIRect::MakeXYWH(0, 0, 10, 10);
    region_.op(rect, SkRegion::kUnion_Op);
  }

  scoped_ptr<VideoFrameCapturer> capturer_;
  MockVideoFrameCapturerDelegate delegate_;
  SkRegion region_;
};

void VideoFrameCapturerMacTest::CaptureDoneCallback1(
    scoped_refptr<CaptureData> capture_data) {
  CGDirectDisplayID mainDevice = CGMainDisplayID();
  int width = CGDisplayPixelsWide(mainDevice);
  int height = CGDisplayPixelsHigh(mainDevice);
  SkRegion initial_region(SkIRect::MakeXYWH(0, 0, width, height));
  EXPECT_EQ(initial_region, capture_data->dirty_region());
}

void VideoFrameCapturerMacTest::CaptureDoneCallback2(
    scoped_refptr<CaptureData> capture_data) {
  CGDirectDisplayID mainDevice = CGMainDisplayID();
  int width = CGDisplayPixelsWide(mainDevice);
  int height = CGDisplayPixelsHigh(mainDevice);

  EXPECT_EQ(region_, capture_data->dirty_region());
  EXPECT_EQ(width, capture_data->size().width());
  EXPECT_EQ(height, capture_data->size().height());
  const DataPlanes &planes = capture_data->data_planes();
  EXPECT_TRUE(planes.data[0] != NULL);
  EXPECT_TRUE(planes.data[1] == NULL);
  EXPECT_TRUE(planes.data[2] == NULL);
  // Depending on the capture method, the screen may be flipped or not, so
  // the stride may be positive or negative.
  EXPECT_EQ(static_cast<int>(sizeof(uint32_t) * width),
            abs(planes.strides[0]));
  EXPECT_EQ(0, planes.strides[1]);
  EXPECT_EQ(0, planes.strides[2]);
}

TEST_F(VideoFrameCapturerMacTest, Capture) {
  if (!CheckSnowLeopard()) {
    return;
  }

  EXPECT_CALL(delegate_, OnCaptureCompleted(_))
      .Times(2)
      .WillOnce(Invoke(this, &VideoFrameCapturerMacTest::CaptureDoneCallback1))
      .WillOnce(Invoke(this, &VideoFrameCapturerMacTest::CaptureDoneCallback2));
  EXPECT_CALL(delegate_, OnCursorShapeChangedPtr(_))
      .Times(AnyNumber());

  SCOPED_TRACE("");
  capturer_->Start(&delegate_);

  // Check that we get an initial full-screen updated.
  capturer_->CaptureFrame();

  // Check that subsequent dirty rects are propagated correctly.
  AddDirtyRect();
  capturer_->InvalidateRegion(region_);
  capturer_->CaptureFrame();
  capturer_->Stop();
}

}  // namespace remoting

namespace gfx {

std::ostream& operator<<(std::ostream& out, const SkRegion& region) {
  out << "SkRegion(";
  for (SkRegion::Iterator i(region); !i.done(); i.next()) {
    const SkIRect& r = i.rect();
    out << "(" << r.fLeft << ","  << r.fTop << ","
        << r.fRight  << ","  << r.fBottom << ")";
  }
  out << ")";
  return out;
}

}  // namespace gfx
