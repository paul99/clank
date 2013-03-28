// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/video_frame_capturer.h"

#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <dlfcn.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <OpenGL/CGLMacro.h>
#include <OpenGL/OpenGL.h>
#include <set>
#include <stddef.h>

#include "base/logging.h"
#include "base/file_path.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/memory/scoped_ptr.h"
#include "base/scoped_native_library.h"
#include "base/synchronization/waitable_event.h"
#include "base/time.h"
#include "remoting/base/capture_data.h"
#include "remoting/base/util.h"
#include "remoting/host/mac/scoped_pixel_buffer_object.h"
#include "remoting/host/video_frame.h"
#include "remoting/host/video_frame_capturer_helper.h"
#include "remoting/host/video_frame_queue.h"
#include "remoting/proto/control.pb.h"

namespace remoting {

namespace {

// Definitions used to dynamic-link to deprecated OS 10.6 functions.
const char* kApplicationServicesLibraryName =
    "/System/Library/Frameworks/ApplicationServices.framework/"
    "ApplicationServices";
typedef void* (*CGDisplayBaseAddressFunc)(CGDirectDisplayID);
typedef size_t (*CGDisplayBytesPerRowFunc)(CGDirectDisplayID);
typedef size_t (*CGDisplayBitsPerPixelFunc)(CGDirectDisplayID);
const char* kOpenGlLibraryName =
    "/System/Library/Frameworks/OpenGL.framework/OpenGL";
typedef CGLError (*CGLSetFullScreenFunc)(CGLContextObj);

// skia/ext/skia_utils_mac.h only defines CGRectToSkRect().
SkIRect CGRectToSkIRect(const CGRect& rect) {
  SkIRect sk_rect = {
    SkScalarRound(rect.origin.x),
    SkScalarRound(rect.origin.y),
    SkScalarRound(rect.origin.x + rect.size.width),
    SkScalarRound(rect.origin.y + rect.size.height)
  };
  return sk_rect;
}

// The amount of time allowed for displays to reconfigure.
const int64 kDisplayConfigurationEventTimeoutInSeconds = 10;

// A class representing a full-frame pixel buffer.
class VideoFrameMac : public VideoFrame {
 public:
  explicit VideoFrameMac(const SkISize& size);
  virtual ~VideoFrameMac();

  const SkIPoint& dpi() const { return dpi_; }

 private:
  // Allocated pixel buffer.
  scoped_array<uint8> data_;

  // DPI settings for this buffer.
  SkIPoint dpi_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameMac);
};

// A class to perform video frame capturing for mac.
class VideoFrameCapturerMac : public VideoFrameCapturer {
 public:
  VideoFrameCapturerMac();
  virtual ~VideoFrameCapturerMac();

  bool Init();

  // Overridden from VideoFrameCapturer:
  virtual void Start(Delegate* delegate) OVERRIDE;
  virtual void Stop() OVERRIDE;
  virtual media::VideoFrame::Format pixel_format() const OVERRIDE;
  virtual void InvalidateRegion(const SkRegion& invalid_region) OVERRIDE;
  virtual void CaptureFrame() OVERRIDE;
  virtual const SkISize& size_most_recent() const OVERRIDE;

 private:
  void CaptureCursor();

  void GlBlitFast(const VideoFrame& buffer, const SkRegion& region);
  void GlBlitSlow(const VideoFrame& buffer);
  void CgBlitPreLion(const VideoFrame& buffer, const SkRegion& region);
  void CgBlitPostLion(const VideoFrame& buffer, const SkRegion& region);

  // Called when the screen configuration is changed.
  void ScreenConfigurationChanged();

  void ScreenRefresh(CGRectCount count, const CGRect *rect_array);
  void ScreenUpdateMove(CGScreenUpdateMoveDelta delta,
                        size_t count,
                        const CGRect *rect_array);
  void DisplaysReconfigured(CGDirectDisplayID display,
                            CGDisplayChangeSummaryFlags flags);
  static void ScreenRefreshCallback(CGRectCount count,
                                    const CGRect *rect_array,
                                    void *user_parameter);
  static void ScreenUpdateMoveCallback(CGScreenUpdateMoveDelta delta,
                                       size_t count,
                                       const CGRect *rect_array,
                                       void *user_parameter);
  static void DisplaysReconfiguredCallback(CGDirectDisplayID display,
                                           CGDisplayChangeSummaryFlags flags,
                                           void *user_parameter);

  void ReleaseBuffers();

  Delegate* delegate_;

  CGLContextObj cgl_context_;
  ScopedPixelBufferObject pixel_buffer_object_;

  // Queue of the frames buffers.
  VideoFrameQueue queue_;

  // Current display configuration.
  std::vector<CGDirectDisplayID> display_ids_;
  SkIRect desktop_bounds_;

  // A thread-safe list of invalid rectangles, and the size of the most
  // recently captured screen.
  VideoFrameCapturerHelper helper_;

  // Image of the last cursor that we sent to the client.
  base::mac::ScopedCFTypeRef<CGImageRef> current_cursor_;

  // Contains an invalid region from the previous capture.
  SkRegion last_invalid_region_;

  // Format of pixels returned in buffer.
  media::VideoFrame::Format pixel_format_;

  // Used to ensure that frame captures do not take place while displays
  // are being reconfigured.
  base::WaitableEvent display_configuration_capture_event_;

  // Records the Ids of attached displays which are being reconfigured.
  // Accessed on the thread on which we are notified of display events.
  std::set<CGDirectDisplayID> reconfiguring_displays_;

  // Power management assertion to prevent the screen from sleeping.
  IOPMAssertionID power_assertion_id_display_;

  // Power management assertion to indicate that the user is active.
  IOPMAssertionID power_assertion_id_user_;

  // Dynamically link to deprecated APIs for Mac OS X 10.6 support.
  base::ScopedNativeLibrary app_services_library_;
  CGDisplayBaseAddressFunc cg_display_base_address_;
  CGDisplayBytesPerRowFunc cg_display_bytes_per_row_;
  CGDisplayBitsPerPixelFunc cg_display_bits_per_pixel_;
  base::ScopedNativeLibrary opengl_library_;
  CGLSetFullScreenFunc cgl_set_full_screen_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameCapturerMac);
};

VideoFrameMac::VideoFrameMac(const SkISize& size) {
  set_bytes_per_row(size.width() * sizeof(uint32_t));
  set_dimensions(size);

  size_t buffer_size = size.width() * size.height() * sizeof(uint32_t);
  data_.reset(new  uint8[buffer_size]);
  set_pixels(data_.get());

  // TODO(wez): Move the ugly DPI code into a helper.
  NSScreen* screen = [NSScreen mainScreen];
  NSDictionary* attr = [screen deviceDescription];
  NSSize resolution = [[attr objectForKey: NSDeviceResolution] sizeValue];
  dpi_.set(resolution.width, resolution.height);
}

VideoFrameMac::~VideoFrameMac() {
}

VideoFrameCapturerMac::VideoFrameCapturerMac()
    : delegate_(NULL),
      cgl_context_(NULL),
      pixel_format_(media::VideoFrame::RGB32),
      display_configuration_capture_event_(false, true),
      power_assertion_id_display_(kIOPMNullAssertionID),
      power_assertion_id_user_(kIOPMNullAssertionID),
      cg_display_base_address_(NULL),
      cg_display_bytes_per_row_(NULL),
      cg_display_bits_per_pixel_(NULL),
      cgl_set_full_screen_(NULL)
{
}

VideoFrameCapturerMac::~VideoFrameCapturerMac() {
  ReleaseBuffers();
  CGUnregisterScreenRefreshCallback(
      VideoFrameCapturerMac::ScreenRefreshCallback, this);
  CGScreenUnregisterMoveCallback(
      VideoFrameCapturerMac::ScreenUpdateMoveCallback, this);
  CGError err = CGDisplayRemoveReconfigurationCallback(
      VideoFrameCapturerMac::DisplaysReconfiguredCallback, this);
  if (err != kCGErrorSuccess) {
    LOG(ERROR) << "CGDisplayRemoveReconfigurationCallback " << err;
  }
}

bool VideoFrameCapturerMac::Init() {
  CGError err = CGRegisterScreenRefreshCallback(
      VideoFrameCapturerMac::ScreenRefreshCallback, this);
  if (err != kCGErrorSuccess) {
    LOG(ERROR) << "CGRegisterScreenRefreshCallback " << err;
    return false;
  }

  err = CGScreenRegisterMoveCallback(
      VideoFrameCapturerMac::ScreenUpdateMoveCallback, this);
  if (err != kCGErrorSuccess) {
    LOG(ERROR) << "CGScreenRegisterMoveCallback " << err;
    return false;
  }
  err = CGDisplayRegisterReconfigurationCallback(
      VideoFrameCapturerMac::DisplaysReconfiguredCallback, this);
  if (err != kCGErrorSuccess) {
    LOG(ERROR) << "CGDisplayRegisterReconfigurationCallback " << err;
    return false;
  }

  ScreenConfigurationChanged();
  return true;
}

void VideoFrameCapturerMac::ReleaseBuffers() {
  if (cgl_context_) {
    pixel_buffer_object_.Release();
    CGLDestroyContext(cgl_context_);
    cgl_context_ = NULL;
  }
  // The buffers might be in use by the encoder, so don't delete them here.
  // Instead, mark them as "needs update"; next time the buffers are used by
  // the capturer, they will be recreated if necessary.
  queue_.SetAllFramesNeedUpdate();
}

void VideoFrameCapturerMac::Start(Delegate* delegate) {
  DCHECK(delegate_ == NULL);

  delegate_ = delegate;

  // Create power management assertions to wake the display and prevent it from
  // going to sleep on user idle.
  // TODO(jamiewalch): Use IOPMAssertionDeclareUserActivity on 10.7.3 and above
  //                   instead of the following two assertions.
  IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                              kIOPMAssertionLevelOn,
                              CFSTR("Chrome Remote Desktop connection active"),
                              &power_assertion_id_display_);
  // This assertion ensures that the display is woken up if it  already asleep
  // (as used by Apple Remote Desktop).
  IOPMAssertionCreateWithName(CFSTR("UserIsActive"),
                              kIOPMAssertionLevelOn,
                              CFSTR("Chrome Remote Desktop connection active"),
                              &power_assertion_id_user_);
}

void VideoFrameCapturerMac::Stop() {
  if (power_assertion_id_display_ != kIOPMNullAssertionID) {
    IOPMAssertionRelease(power_assertion_id_display_);
    power_assertion_id_display_ = kIOPMNullAssertionID;
  }
  if (power_assertion_id_user_ != kIOPMNullAssertionID) {
    IOPMAssertionRelease(power_assertion_id_user_);
    power_assertion_id_user_ = kIOPMNullAssertionID;
  }
}

media::VideoFrame::Format VideoFrameCapturerMac::pixel_format() const {
  return pixel_format_;
}

void VideoFrameCapturerMac::InvalidateRegion(const SkRegion& invalid_region) {
  helper_.InvalidateRegion(invalid_region);
}

void VideoFrameCapturerMac::CaptureFrame() {
  // Only allow captures when the display configuration is not occurring.
  scoped_refptr<CaptureData> data;

  base::Time capture_start_time = base::Time::Now();

  // Wait until the display configuration is stable. If one or more displays
  // are reconfiguring then |display_configuration_capture_event_| will not be
  // set until the reconfiguration completes.
  // TODO(wez): Replace this with an early-exit (See crbug.com/104542).
  CHECK(display_configuration_capture_event_.TimedWait(
      base::TimeDelta::FromSeconds(
          kDisplayConfigurationEventTimeoutInSeconds)));

  SkRegion region;
  helper_.SwapInvalidRegion(&region);

  // If the current buffer is from an older generation then allocate a new one.
  // Note that we can't reallocate other buffers at this point, since the caller
  // may still be reading from them.
  if (queue_.current_frame_needs_update()) {
    scoped_ptr<VideoFrameMac> buffer(new VideoFrameMac(
        SkISize::Make(desktop_bounds_.width(), desktop_bounds_.height())));
    queue_.ReplaceCurrentFrame(buffer.PassAs<VideoFrame>());
  }

  VideoFrame* current_buffer = queue_.current_frame();

  bool flip = false;  // GL capturers need flipping.
  if (base::mac::IsOSLionOrLater()) {
    // Lion requires us to use their new APIs for doing screen capture. These
    // APIS currently crash on 10.6.8 if there is no monitor attached.
    CgBlitPostLion(*current_buffer, region);
  } else if (cgl_context_) {
    flip = true;
    if (pixel_buffer_object_.get() != 0) {
      GlBlitFast(*current_buffer, region);
    } else {
      // See comment in ScopedPixelBufferObject::Init about why the slow
      // path is always used on 10.5.
      GlBlitSlow(*current_buffer);
    }
  } else {
    CgBlitPreLion(*current_buffer, region);
  }

  DataPlanes planes;
  planes.data[0] = current_buffer->pixels();
  planes.strides[0] = current_buffer->bytes_per_row();
  if (flip) {
    planes.strides[0] = -planes.strides[0];
    planes.data[0] += (current_buffer->dimensions().height() - 1) *
        current_buffer->bytes_per_row();
  }

  data = new CaptureData(planes, current_buffer->dimensions(), pixel_format());
  data->set_dpi(static_cast<VideoFrameMac*>(current_buffer)->dpi());
  data->mutable_dirty_region() = region;

  helper_.set_size_most_recent(data->size());

  // Signal that we are done capturing data from the display framebuffer,
  // and accessing display structures.
  display_configuration_capture_event_.Signal();

  // Capture the current cursor shape and notify |delegate_| if it has changed.
  CaptureCursor();

  // Move the capture frame buffer queue on to the next buffer.
  queue_.DoneWithCurrentFrame();

  data->set_capture_time_ms(
      (base::Time::Now() - capture_start_time).InMillisecondsRoundedUp());
  delegate_->OnCaptureCompleted(data);
}

void VideoFrameCapturerMac::CaptureCursor() {
  NSCursor* cursor = [NSCursor currentSystemCursor];
  if (cursor == nil) {
    return;
  }

  NSImage* nsimage = [cursor image];
  NSPoint hotspot = [cursor hotSpot];
  NSSize size = [nsimage size];
  CGImageRef image = [nsimage CGImageForProposedRect:NULL
                                             context:nil
                                               hints:nil];
  if (image == nil) {
    return;
  }

  if (CGImageGetBitsPerPixel(image) != 32 ||
      CGImageGetBytesPerRow(image) != (size.width * 4) ||
      CGImageGetBitsPerComponent(image) != 8) {
    return;
  }

  // Compare the current cursor with the last one we sent to the client
  // and exit if the cursor is the same.
  if (current_cursor_.get() != NULL) {
    CGImageRef current = current_cursor_.get();
    if (CGImageGetWidth(image) == CGImageGetWidth(current) &&
        CGImageGetHeight(image) == CGImageGetHeight(current) &&
        CGImageGetBitsPerPixel(image) == CGImageGetBitsPerPixel(current) &&
        CGImageGetBytesPerRow(image) == CGImageGetBytesPerRow(current) &&
        CGImageGetBitsPerComponent(image) ==
            CGImageGetBitsPerComponent(current)) {
      CGDataProviderRef provider_new = CGImageGetDataProvider(image);
      base::mac::ScopedCFTypeRef<CFDataRef> data_ref_new(
          CGDataProviderCopyData(provider_new));
      CGDataProviderRef provider_current = CGImageGetDataProvider(current);
      base::mac::ScopedCFTypeRef<CFDataRef> data_ref_current(
          CGDataProviderCopyData(provider_current));

      if (data_ref_new.get() != NULL && data_ref_current.get() != NULL) {
        int data_size = CFDataGetLength(data_ref_new);
        CHECK(data_size == CFDataGetLength(data_ref_current));
        const uint8* data_new = CFDataGetBytePtr(data_ref_new);
        const uint8* data_current = CFDataGetBytePtr(data_ref_current);
        if (memcmp(data_new, data_current, data_size) == 0) {
          return;
        }
      }
    }
  }

  VLOG(3) << "Sending cursor: " << size.width << "x" << size.height;

  CGDataProviderRef provider = CGImageGetDataProvider(image);
  base::mac::ScopedCFTypeRef<CFDataRef> image_data_ref(
      CGDataProviderCopyData(provider));
  if (image_data_ref.get() == NULL) {
    return;
  }
  const uint8* cursor_src_data = CFDataGetBytePtr(image_data_ref);
  int data_size = CFDataGetLength(image_data_ref);

  // Create a CursorShapeInfo proto that describes the cursor and pass it to
  // the client.
  scoped_ptr<protocol::CursorShapeInfo> cursor_proto(
      new protocol::CursorShapeInfo());
  cursor_proto->mutable_data()->resize(data_size);
  uint8* cursor_tgt_data = const_cast<uint8*>(reinterpret_cast<const uint8*>(
      cursor_proto->mutable_data()->data()));

  memcpy(cursor_tgt_data, cursor_src_data, data_size);

  cursor_proto->set_width(size.width);
  cursor_proto->set_height(size.height);
  cursor_proto->set_hotspot_x(hotspot.x);
  cursor_proto->set_hotspot_y(hotspot.y);
  delegate_->OnCursorShapeChanged(cursor_proto.Pass());

  // Record the last cursor image that we sent.
  current_cursor_.reset(CGImageCreateCopy(image));
}

void VideoFrameCapturerMac::GlBlitFast(const VideoFrame& buffer,
                                       const SkRegion& region) {
  const int buffer_height = buffer.dimensions().height();
  const int buffer_width = buffer.dimensions().width();

  // Clip to the size of our current screen.
  SkIRect clip_rect = SkIRect::MakeWH(buffer_width, buffer_height);
  if (queue_.previous_frame()) {
    // We are doing double buffer for the capture data so we just need to copy
    // the invalid region from the previous capture in the current buffer.
    // TODO(hclam): We can reduce the amount of copying here by subtracting
    // |capturer_helper_|s region from |last_invalid_region_|.
    // http://crbug.com/92354

    // Since the image obtained from OpenGL is upside-down, need to do some
    // magic here to copy the correct rectangle.
    const int y_offset = (buffer_height - 1) * buffer.bytes_per_row();
    for(SkRegion::Iterator i(last_invalid_region_); !i.done(); i.next()) {
      SkIRect copy_rect = i.rect();
      if (copy_rect.intersect(clip_rect)) {
        CopyRect(queue_.previous_frame()->pixels() + y_offset,
                 -buffer.bytes_per_row(),
                 buffer.pixels() + y_offset,
                 -buffer.bytes_per_row(),
                 4,  // Bytes for pixel for RGBA.
                 copy_rect);
      }
    }
  }
  last_invalid_region_ = region;

  CGLContextObj CGL_MACRO_CONTEXT = cgl_context_;
  glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pixel_buffer_object_.get());
  glReadPixels(0, 0, buffer_width, buffer_height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
  GLubyte* ptr = static_cast<GLubyte*>(
      glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB));
  if (ptr == NULL) {
    // If the buffer can't be mapped, assume that it's no longer valid and
    // release it.
    pixel_buffer_object_.Release();
  } else {
    // Copy only from the dirty rects. Since the image obtained from OpenGL is
    // upside-down we need to do some magic here to copy the correct rectangle.
    const int y_offset = (buffer_height - 1) * buffer.bytes_per_row();
    for(SkRegion::Iterator i(region); !i.done(); i.next()) {
      SkIRect copy_rect = i.rect();
      if (copy_rect.intersect(clip_rect)) {
        CopyRect(ptr + y_offset,
           -buffer.bytes_per_row(),
           buffer.pixels() + y_offset,
           -buffer.bytes_per_row(),
           4,  // Bytes for pixel for RGBA.
           copy_rect);
      }
    }
  }
  if (!glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB)) {
    // If glUnmapBuffer returns false, then the contents of the data store are
    // undefined. This might be because the screen mode has changed, in which
    // case it will be recreated in ScreenConfigurationChanged, but releasing
    // the object here is the best option. Capturing will fall back on
    // GlBlitSlow until such time as the pixel buffer object is recreated.
    pixel_buffer_object_.Release();
  }
  glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
}

void VideoFrameCapturerMac::GlBlitSlow(const VideoFrame& buffer) {
  CGLContextObj CGL_MACRO_CONTEXT = cgl_context_;
  glReadBuffer(GL_FRONT);
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);  // Force 4-byte alignment.
  glPixelStorei(GL_PACK_ROW_LENGTH, 0);
  glPixelStorei(GL_PACK_SKIP_ROWS, 0);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
  // Read a block of pixels from the frame buffer.
  glReadPixels(0, 0, buffer.dimensions().width(), buffer.dimensions().height(),
               GL_BGRA, GL_UNSIGNED_BYTE, buffer.pixels());
  glPopClientAttrib();
}

void VideoFrameCapturerMac::CgBlitPreLion(const VideoFrame& buffer,
                                          const SkRegion& region) {
  const int buffer_height = buffer.dimensions().height();

  // Copy the entire contents of the previous capture buffer, to capture over.
  // TODO(wez): Get rid of this as per crbug.com/145064, or implement
  // crbug.com/92354.
  if (queue_.previous_frame()) {
    memcpy(buffer.pixels(),
           queue_.previous_frame()->pixels(),
           buffer.bytes_per_row() * buffer_height);
  }

  for (unsigned int d = 0; d < display_ids_.size(); ++d) {
    // Use deprecated APIs to determine the display buffer layout.
    DCHECK(cg_display_base_address_ && cg_display_bytes_per_row_ &&
        cg_display_bits_per_pixel_);
    uint8* display_base_address =
      reinterpret_cast<uint8*>((*cg_display_base_address_)(display_ids_[d]));
    CHECK(display_base_address);
    int src_bytes_per_row = (*cg_display_bytes_per_row_)(display_ids_[d]);
    int src_bytes_per_pixel =
        (*cg_display_bits_per_pixel_)(display_ids_[d]) / 8;

    // Determine the position of the display in the buffer.
    SkIRect display_bounds = CGRectToSkIRect(CGDisplayBounds(display_ids_[d]));
    display_bounds.offset(-desktop_bounds_.left(), -desktop_bounds_.top());

    // Determine which parts of the blit region, if any, lay within the monitor.
    SkRegion copy_region;
    if (!copy_region.op(region, display_bounds, SkRegion::kIntersect_Op))
      continue;

    // Translate the region to be copied into display-relative coordinates.
    copy_region.translate(-display_bounds.left(), -display_bounds.top());

    // Calculate where in the output buffer the display's origin is.
    uint8* out_ptr = buffer.pixels() +
         (display_bounds.left() * src_bytes_per_pixel) +
         (display_bounds.top() * buffer.bytes_per_row());

    // Copy the dirty region from the display buffer into our desktop buffer.
    for(SkRegion::Iterator i(copy_region); !i.done(); i.next()) {
      CopyRect(display_base_address,
               src_bytes_per_row,
               out_ptr,
               buffer.bytes_per_row(),
               src_bytes_per_pixel,
               i.rect());
    }
  }
}

void VideoFrameCapturerMac::CgBlitPostLion(const VideoFrame& buffer,
                                           const SkRegion& region) {
  const int buffer_height = buffer.dimensions().height();

  // Copy the entire contents of the previous capture buffer, to capture over.
  // TODO(wez): Get rid of this as per crbug.com/145064, or implement
  // crbug.com/92354.
  if (queue_.previous_frame()) {
    memcpy(buffer.pixels(),
           queue_.previous_frame()->pixels(),
           buffer.bytes_per_row() * buffer_height);
  }

  for (unsigned int d = 0; d < display_ids_.size(); ++d) {
    // Determine the position of the display in the buffer.
    SkIRect display_bounds = CGRectToSkIRect(CGDisplayBounds(display_ids_[d]));
    display_bounds.offset(-desktop_bounds_.left(), -desktop_bounds_.top());

    // Determine which parts of the blit region, if any, lay within the monitor.
    SkRegion copy_region;
    if (!copy_region.op(region, display_bounds, SkRegion::kIntersect_Op))
      continue;

    // Translate the region to be copied into display-relative coordinates.
    copy_region.translate(-display_bounds.left(), -display_bounds.top());

    // Create an image containing a snapshot of the display.
    base::mac::ScopedCFTypeRef<CGImageRef> image(
        CGDisplayCreateImage(display_ids_[d]));
    if (image.get() == NULL)
      continue;

    // Request access to the raw pixel data via the image's DataProvider.
    CGDataProviderRef provider = CGImageGetDataProvider(image);
    base::mac::ScopedCFTypeRef<CFDataRef> data(
        CGDataProviderCopyData(provider));
    if (data.get() == NULL)
      continue;

    const uint8* display_base_address = CFDataGetBytePtr(data);
    int src_bytes_per_row = CGImageGetBytesPerRow(image);
    int src_bytes_per_pixel = CGImageGetBitsPerPixel(image) / 8;

    // Calculate where in the output buffer the display's origin is.
    uint8* out_ptr = buffer.pixels() +
        (display_bounds.left() * src_bytes_per_pixel) +
        (display_bounds.top() * buffer.bytes_per_row());

    // Copy the dirty region from the display buffer into our desktop buffer.
    for(SkRegion::Iterator i(copy_region); !i.done(); i.next()) {
      CopyRect(display_base_address,
               src_bytes_per_row,
               out_ptr,
               buffer.bytes_per_row(),
               src_bytes_per_pixel,
               i.rect());
    }
  }
}

const SkISize& VideoFrameCapturerMac::size_most_recent() const {
  return helper_.size_most_recent();
}

void VideoFrameCapturerMac::ScreenConfigurationChanged() {
  // Release existing buffers, which will be of the wrong size.
  ReleaseBuffers();

  // Clear the dirty region, in case the display is down-sizing.
  helper_.ClearInvalidRegion();

  // Fetch the list if active displays and calculate their bounds.
  CGDisplayCount display_count;
  CGError error = CGGetActiveDisplayList(0, NULL, &display_count);
  CHECK_EQ(error, CGDisplayNoErr);

  display_ids_.resize(display_count);
  error = CGGetActiveDisplayList(display_count, &display_ids_[0],
                                 &display_count);
  CHECK_EQ(error, CGDisplayNoErr);
  CHECK_EQ(display_count, display_ids_.size());

  desktop_bounds_ = SkIRect::MakeEmpty();
  for (unsigned int d = 0; d < display_count; ++d) {
    CGRect display_bounds = CGDisplayBounds(display_ids_[d]);
    desktop_bounds_.join(CGRectToSkIRect(display_bounds));
  }

  // Re-mark the entire desktop as dirty.
  helper_.InvalidateScreen(SkISize::Make(desktop_bounds_.width(),
                                         desktop_bounds_.height()));

  // Make sure the frame buffers will be reallocated.
  queue_.SetAllFramesNeedUpdate();

  // CgBlitPostLion uses CGDisplayCreateImage() to snapshot each display's
  // contents. Although the API exists in OS 10.6, it crashes the caller if
  // the machine has no monitor connected, so we fall back to depcreated APIs
  // when running on 10.6.
  if (base::mac::IsOSLionOrLater()) {
    LOG(INFO) << "Using CgBlitPostLion.";
    // No need for any OpenGL support on Lion
    return;
  }

  // Dynamically link to the deprecated pre-Lion capture APIs.
  std::string app_services_library_error;
  FilePath app_services_path(kApplicationServicesLibraryName);
  app_services_library_.Reset(
      base::LoadNativeLibrary(app_services_path, &app_services_library_error));
  CHECK(app_services_library_.is_valid()) << app_services_library_error;

  std::string opengl_library_error;
  FilePath opengl_path(kOpenGlLibraryName);
  opengl_library_.Reset(
      base::LoadNativeLibrary(opengl_path, &opengl_library_error));
  CHECK(opengl_library_.is_valid()) << opengl_library_error;

  cg_display_base_address_ = reinterpret_cast<CGDisplayBaseAddressFunc>(
      app_services_library_.GetFunctionPointer("CGDisplayBaseAddress"));
  cg_display_bytes_per_row_ = reinterpret_cast<CGDisplayBytesPerRowFunc>(
      app_services_library_.GetFunctionPointer("CGDisplayBytesPerRow"));
  cg_display_bits_per_pixel_ = reinterpret_cast<CGDisplayBitsPerPixelFunc>(
      app_services_library_.GetFunctionPointer("CGDisplayBitsPerPixel"));
  cgl_set_full_screen_ = reinterpret_cast<CGLSetFullScreenFunc>(
      opengl_library_.GetFunctionPointer("CGLSetFullScreen"));
  CHECK(cg_display_base_address_ && cg_display_bytes_per_row_ &&
        cg_display_bits_per_pixel_ && cgl_set_full_screen_);

  if (display_ids_.size() > 1) {
    LOG(INFO) << "Using CgBlitPreLion (Multi-monitor).";
    return;
  }

  CGDirectDisplayID mainDevice = CGMainDisplayID();
  if (!CGDisplayUsesOpenGLAcceleration(mainDevice)) {
    LOG(INFO) << "Using CgBlitPreLion (OpenGL unavailable).";
    return;
  }

  LOG(INFO) << "Using GlBlit";

  CGLPixelFormatAttribute attributes[] = {
    kCGLPFAFullScreen,
    kCGLPFADisplayMask,
    (CGLPixelFormatAttribute)CGDisplayIDToOpenGLDisplayMask(mainDevice),
    (CGLPixelFormatAttribute)0
  };
  CGLPixelFormatObj pixel_format = NULL;
  GLint matching_pixel_format_count = 0;
  CGLError err = CGLChoosePixelFormat(attributes,
                                      &pixel_format,
                                      &matching_pixel_format_count);
  DCHECK_EQ(err, kCGLNoError);
  err = CGLCreateContext(pixel_format, NULL, &cgl_context_);
  DCHECK_EQ(err, kCGLNoError);
  CGLDestroyPixelFormat(pixel_format);
  (*cgl_set_full_screen_)(cgl_context_);
  CGLSetCurrentContext(cgl_context_);

  size_t buffer_size = desktop_bounds_.width() * desktop_bounds_.height() *
                       sizeof(uint32_t);
  pixel_buffer_object_.Init(cgl_context_, buffer_size);
}

void VideoFrameCapturerMac::ScreenRefresh(CGRectCount count,
                                          const CGRect* rect_array) {
  if (desktop_bounds_.isEmpty()) {
    return;
  }
  SkIRect skirect_array[count];
  for (CGRectCount i = 0; i < count; ++i) {
    skirect_array[i] = CGRectToSkIRect(rect_array[i]);
    skirect_array[i].offset(-desktop_bounds_.left(), -desktop_bounds_.top());
  }
  SkRegion region;
  region.setRects(skirect_array, count);
  InvalidateRegion(region);
}

void VideoFrameCapturerMac::ScreenUpdateMove(CGScreenUpdateMoveDelta delta,
                                             size_t count,
                                             const CGRect* rect_array) {
  SkIRect skirect_array[count];
  for (CGRectCount i = 0; i < count; ++i) {
    CGRect rect = rect_array[i];
    rect = CGRectOffset(rect, delta.dX, delta.dY);
    skirect_array[i] = CGRectToSkIRect(rect);
    skirect_array[i].offset(-desktop_bounds_.left(), -desktop_bounds_.top());
  }
  SkRegion region;
  region.setRects(skirect_array, count);
  InvalidateRegion(region);
}

void VideoFrameCapturerMac::DisplaysReconfigured(
    CGDirectDisplayID display,
    CGDisplayChangeSummaryFlags flags) {
  if (flags & kCGDisplayBeginConfigurationFlag) {
    if (reconfiguring_displays_.empty()) {
      // If this is the first display to start reconfiguring then wait on
      // |display_configuration_capture_event_| to block the capture thread
      // from accessing display memory until the reconfiguration completes.
      CHECK(display_configuration_capture_event_.TimedWait(
                base::TimeDelta::FromSeconds(
                    kDisplayConfigurationEventTimeoutInSeconds)));
    }

    reconfiguring_displays_.insert(display);
  } else {
    reconfiguring_displays_.erase(display);

    if (reconfiguring_displays_.empty()) {
      // If no other displays are reconfiguring then refresh capturer data
      // structures and un-block the capturer thread.
      ScreenConfigurationChanged();
      display_configuration_capture_event_.Signal();
    }
  }
}

void VideoFrameCapturerMac::ScreenRefreshCallback(CGRectCount count,
                                                  const CGRect* rect_array,
                                                  void* user_parameter) {
  VideoFrameCapturerMac* capturer = reinterpret_cast<VideoFrameCapturerMac*>(
      user_parameter);
  if (capturer->desktop_bounds_.isEmpty()) {
    capturer->ScreenConfigurationChanged();
  }
  capturer->ScreenRefresh(count, rect_array);
}

void VideoFrameCapturerMac::ScreenUpdateMoveCallback(
    CGScreenUpdateMoveDelta delta,
    size_t count,
    const CGRect* rect_array,
    void* user_parameter) {
  VideoFrameCapturerMac* capturer = reinterpret_cast<VideoFrameCapturerMac*>(
      user_parameter);
  capturer->ScreenUpdateMove(delta, count, rect_array);
}

void VideoFrameCapturerMac::DisplaysReconfiguredCallback(
    CGDirectDisplayID display,
    CGDisplayChangeSummaryFlags flags,
    void* user_parameter) {
  VideoFrameCapturerMac* capturer = reinterpret_cast<VideoFrameCapturerMac*>(
      user_parameter);
  capturer->DisplaysReconfigured(display, flags);
}

}  // namespace

// static
scoped_ptr<VideoFrameCapturer> VideoFrameCapturer::Create() {
  scoped_ptr<VideoFrameCapturerMac> capturer(new VideoFrameCapturerMac());
  if (!capturer->Init())
    capturer.reset();
  return capturer.PassAs<VideoFrameCapturer>();
}

// static
scoped_ptr<VideoFrameCapturer> VideoFrameCapturer::CreateWithFactory(
    SharedBufferFactory* shared_buffer_factory) {
  NOTIMPLEMENTED();
  return scoped_ptr<VideoFrameCapturer>();
}

}  // namespace remoting
