// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/event_executor.h"

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/XInput.h>

#include <set>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "remoting/host/clipboard.h"
#include "remoting/proto/internal.pb.h"
#include "third_party/skia/include/core/SkPoint.h"

namespace remoting {

namespace {

using protocol::ClipboardEvent;
using protocol::KeyEvent;
using protocol::MouseEvent;

// USB to XKB keycode map table.
#define USB_KEYMAP(usb, xkb, win, mac) {usb, xkb}
#include "ui/base/keycodes/usb_keycode_map.h"
#undef USB_KEYMAP

// Pixel-to-wheel-ticks conversion ratio used by GTK.
// From Source/WebKit/chromium/src/gtk/WebInputFactory.cc.
const float kWheelTicksPerPixel = 3.0f / 160.0f;

// A class to generate events on Linux.
class EventExecutorLinux : public EventExecutor {
 public:
  explicit EventExecutorLinux(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  virtual ~EventExecutorLinux();

  bool Init();

  // Clipboard stub interface.
  virtual void InjectClipboardEvent(const ClipboardEvent& event) OVERRIDE;

  // InputStub interface.
  virtual void InjectKeyEvent(const KeyEvent& event) OVERRIDE;
  virtual void InjectMouseEvent(const MouseEvent& event) OVERRIDE;

  // EventExecutor interface.
  virtual void Start(
      scoped_ptr<protocol::ClipboardStub> client_clipboard) OVERRIDE;

 private:
  // The actual implementation resides in EventExecutorWin::Core class.
  class Core : public base::RefCountedThreadSafe<Core>, public EventExecutor {
   public:
    explicit Core(scoped_refptr<base::SingleThreadTaskRunner> task_runner);

    bool Init();

    // Clipboard stub interface.
    virtual void InjectClipboardEvent(const ClipboardEvent& event)
        OVERRIDE;

    // InputStub interface.
    virtual void InjectKeyEvent(const KeyEvent& event) OVERRIDE;
    virtual void InjectMouseEvent(const MouseEvent& event) OVERRIDE;

    // EventExecutor interface.
    virtual void Start(
        scoped_ptr<protocol::ClipboardStub> client_clipboard) OVERRIDE;

    void Stop();

   private:
    friend class base::RefCountedThreadSafe<Core>;
    virtual ~Core();

    void InitClipboard();

    // |mode| is one of the AutoRepeatModeOn, AutoRepeatModeOff,
    // AutoRepeatModeDefault constants defined by the XChangeKeyboardControl()
    // API.
    void SetAutoRepeatForKey(int keycode, int mode);
    void InjectScrollWheelClicks(int button, int count);
    // Compensates for global button mappings and resets the XTest device
    // mapping.
    void InitMouseButtonMap();
    int MouseButtonToX11ButtonNumber(MouseEvent::MouseButton button);
    int HorizontalScrollWheelToX11ButtonNumber(int dx);
    int VerticalScrollWheelToX11ButtonNumber(int dy);

    scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

    std::set<int> pressed_keys_;
    SkIPoint latest_mouse_position_;
    float wheel_ticks_x_;
    float wheel_ticks_y_;

    // X11 graphics context.
    Display* display_;
    Window root_window_;

    int test_event_base_;
    int test_error_base_;

    // Number of buttons we support.
    // Left, Right, Middle, VScroll Up/Down, HScroll Left/Right.
    static const int kNumPointerButtons = 7;

    int pointer_button_map_[kNumPointerButtons];

    scoped_ptr<Clipboard> clipboard_;

    DISALLOW_COPY_AND_ASSIGN(Core);
  };

  scoped_refptr<Core> core_;

  DISALLOW_COPY_AND_ASSIGN(EventExecutorLinux);
};

EventExecutorLinux::EventExecutorLinux(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  core_ = new Core(task_runner);
}
EventExecutorLinux::~EventExecutorLinux() {
  core_->Stop();
}

bool EventExecutorLinux::Init() {
  return core_->Init();
}

void EventExecutorLinux::InjectClipboardEvent(const ClipboardEvent& event) {
  core_->InjectClipboardEvent(event);
}

void EventExecutorLinux::InjectKeyEvent(const KeyEvent& event) {
  core_->InjectKeyEvent(event);
}

void EventExecutorLinux::InjectMouseEvent(const MouseEvent& event) {
  core_->InjectMouseEvent(event);
}

void EventExecutorLinux::Start(
    scoped_ptr<protocol::ClipboardStub> client_clipboard) {
  core_->Start(client_clipboard.Pass());
}

EventExecutorLinux::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(task_runner),
      latest_mouse_position_(SkIPoint::Make(-1, -1)),
      wheel_ticks_x_(0.0f),
      wheel_ticks_y_(0.0f),
      display_(XOpenDisplay(NULL)),
      root_window_(BadValue) {
}

bool EventExecutorLinux::Core::Init() {
  CHECK(display_);

#if defined(REMOTING_HOST_LINUX_CLIPBOARD)
  if (!task_runner_->BelongsToCurrentThread())
    task_runner_->PostTask(FROM_HERE, base::Bind(&Core::InitClipboard, this));
#endif  // REMOTING_HOST_LINUX_CLIPBOARD

  root_window_ = RootWindow(display_, DefaultScreen(display_));
  if (root_window_ == BadValue) {
    LOG(ERROR) << "Unable to get the root window";
    return false;
  }

  // TODO(ajwong): Do we want to check the major/minor version at all for XTest?
  int major = 0;
  int minor = 0;
  if (!XTestQueryExtension(display_, &test_event_base_, &test_error_base_,
                           &major, &minor)) {
    LOG(ERROR) << "Server does not support XTest.";
    return false;
  }
  InitMouseButtonMap();
  return true;
}

void EventExecutorLinux::Core::InjectClipboardEvent(
    const ClipboardEvent& event) {
#if defined(REMOTING_HOST_LINUX_CLIPBOARD)
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::InjectClipboardEvent, this, event));
    return;
  }

  clipboard_->InjectClipboardEvent(event);
#endif  // REMOTING_HOST_LINUX_CLIPBOARD
}

void EventExecutorLinux::Core::InjectKeyEvent(const KeyEvent& event) {
  // HostEventDispatcher should filter events missing the pressed field.
  DCHECK(event.has_pressed());
  DCHECK(event.has_usb_keycode());

  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&Core::InjectKeyEvent, this, event));
    return;
  }

  int keycode = UsbKeycodeToNativeKeycode(event.usb_keycode());
  VLOG(3) << "Converting USB keycode: " << std::hex << event.usb_keycode()
          << " to keycode: " << keycode << std::dec;

  // Ignore events which can't be mapped.
  if (keycode == kInvalidKeycode)
    return;

  if (event.pressed()) {
    if (pressed_keys_.find(keycode) != pressed_keys_.end()) {
      // Key is already held down, so lift the key up to ensure this repeated
      // press takes effect.
      XTestFakeKeyEvent(display_, keycode, False, CurrentTime);
    } else {
      // Key is not currently held down, so disable auto-repeat for this
      // key to avoid repeated presses in case network congestion delays the
      // key-released event from the client.
      SetAutoRepeatForKey(keycode, AutoRepeatModeOff);
    }
    pressed_keys_.insert(keycode);
  } else {
    pressed_keys_.erase(keycode);

    // Reset the AutoRepeatMode for the key that has been lifted.  In the IT2Me
    // case, this ensures that key-repeating will continue to work normally
    // for the local user of the host machine.  "ModeDefault" is used instead
    // of "ModeOn", since some keys (such as Shift) should not auto-repeat.
    SetAutoRepeatForKey(keycode, AutoRepeatModeDefault);
  }

  XTestFakeKeyEvent(display_, keycode, event.pressed(), CurrentTime);
  XFlush(display_);
}

EventExecutorLinux::Core::~Core() {
  CHECK(pressed_keys_.empty());
}

void EventExecutorLinux::Core::InitClipboard() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  clipboard_ = Clipboard::Create();
}

void EventExecutorLinux::Core::SetAutoRepeatForKey(int keycode, int mode) {
  XKeyboardControl control;
  control.key = keycode;
  control.auto_repeat_mode = mode;
  XChangeKeyboardControl(display_, KBKey | KBAutoRepeatMode, &control);
}

void EventExecutorLinux::Core::InjectScrollWheelClicks(int button, int count) {
  if (button < 0) {
    LOG(WARNING) << "Ignoring unmapped scroll wheel button";
    return;
  }
  for (int i = 0; i < count; i++) {
    // Generate a button-down and a button-up to simulate a wheel click.
    XTestFakeButtonEvent(display_, button, true, CurrentTime);
    XTestFakeButtonEvent(display_, button, false, CurrentTime);
  }
}

void EventExecutorLinux::Core::InjectMouseEvent(const MouseEvent& event) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&Core::InjectMouseEvent, this, event));
    return;
  }

  if (event.has_x() && event.has_y()) {
    // Injecting a motion event immediately before a button release results in
    // a MotionNotify even if the mouse position hasn't changed, which confuses
    // apps which assume MotionNotify implies movement. See crbug.com/138075.
    bool inject_motion = true;
    SkIPoint new_mouse_position(SkIPoint::Make(event.x(), event.y()));
    if (event.has_button() && event.has_button_down() && !event.button_down()) {
      if (new_mouse_position == latest_mouse_position_)
        inject_motion = false;
    }

    latest_mouse_position_ = new_mouse_position;

    if (inject_motion) {
      VLOG(3) << "Moving mouse to " << event.x()
              << "," << event.y();
      XTestFakeMotionEvent(display_, DefaultScreen(display_),
                           event.x(), event.y(),
                           CurrentTime);
    }
  }

  if (event.has_button() && event.has_button_down()) {
    int button_number = MouseButtonToX11ButtonNumber(event.button());

    if (button_number < 0) {
      LOG(WARNING) << "Ignoring unknown button type: " << event.button();
      return;
    }

    VLOG(3) << "Button " << event.button()
            << " received, sending "
            << (event.button_down() ? "down " : "up ")
            << button_number;
    XTestFakeButtonEvent(display_, button_number, event.button_down(),
                         CurrentTime);
  }

  int ticks_y = 0;
  if (event.has_wheel_delta_y()) {
    wheel_ticks_y_ += event.wheel_delta_y() * kWheelTicksPerPixel;
    ticks_y = static_cast<int>(wheel_ticks_y_);
    wheel_ticks_y_ -= ticks_y;
  } else if (event.has_wheel_offset_y()) {
    ticks_y = event.wheel_offset_y();
  }
  if (ticks_y != 0) {
    InjectScrollWheelClicks(VerticalScrollWheelToX11ButtonNumber(ticks_y),
                            abs(ticks_y));
  }

  int ticks_x = 0;
  if (event.has_wheel_delta_x()) {
    wheel_ticks_x_ += event.wheel_delta_x() * kWheelTicksPerPixel;
    ticks_x = static_cast<int>(wheel_ticks_x_);
    wheel_ticks_x_ -= ticks_x;
  } else if (event.has_wheel_offset_x()) {
    ticks_x = event.wheel_offset_x();
  }
  if (ticks_x != 0) {
    InjectScrollWheelClicks(HorizontalScrollWheelToX11ButtonNumber(ticks_x),
                            abs(ticks_x));
  }

  XFlush(display_);
}

void EventExecutorLinux::Core::InitMouseButtonMap() {
  // TODO(rmsousa): Run this on global/device mapping change events.

  // Do not touch global pointer mapping, since this may affect the local user.
  // Instead, try to work around it by reversing the mapping.
  // Note that if a user has a global mapping that completely disables a button
  // (by assigning 0 to it), we won't be able to inject it.
  int num_buttons = XGetPointerMapping(display_, NULL, 0);
  scoped_array<unsigned char> pointer_mapping(new unsigned char[num_buttons]);
  num_buttons = XGetPointerMapping(display_, pointer_mapping.get(),
                                   num_buttons);
  for (int i = 0; i < kNumPointerButtons; i++) {
    pointer_button_map_[i] = -1;
  }
  for (int i = 0; i < num_buttons; i++) {
    // Reverse the mapping.
    if (pointer_mapping[i] > 0 && pointer_mapping[i] <= kNumPointerButtons) {
      pointer_button_map_[pointer_mapping[i] - 1] = i + 1;
    }
  }
  for (int i = 0; i < kNumPointerButtons; i++) {
    if (pointer_button_map_[i] == -1)
      LOG(ERROR) << "Global pointer mapping does not support button " << i + 1;
  }

  int opcode, event, error;
  if (!XQueryExtension(display_, "XInputExtension", &opcode, &event, &error)) {
    // If XInput is not available, we're done. But it would be very unusual to
    // have a server that supports XTest but not XInput, so log it as an error.
    LOG(ERROR) << "X Input extension not available: " << error;
    return;
  }

  // Make sure the XTEST XInput pointer device mapping is trivial. It should be
  // safe to reset this mapping, as it won't affect the user's local devices.
  // In fact, the reason why we do this is because an old gnome-settings-daemon
  // may have mistakenly applied left-handed preferences to the XTEST device.
  XID device_id = 0;
  bool device_found = false;
  int num_devices;
  XDeviceInfo* devices;
  devices = XListInputDevices(display_, &num_devices);
  for (int i = 0; i < num_devices; i++) {
    XDeviceInfo* device_info = &devices[i];
    if (device_info->use == IsXExtensionPointer &&
        strcmp(device_info->name, "Virtual core XTEST pointer") == 0) {
      device_id = device_info->id;
      device_found = true;
      break;
    }
  }
  XFreeDeviceList(devices);

  if (!device_found) {
    LOG(INFO) << "Cannot find XTest device.";
    return;
  }

  XDevice* device = XOpenDevice(display_, device_id);
  if (!device) {
    LOG(ERROR) << "Cannot open XTest device.";
    return;
  }

  int num_device_buttons = XGetDeviceButtonMapping(display_, device, NULL, 0);
  scoped_array<unsigned char> button_mapping(new unsigned char[num_buttons]);
  for (int i = 0; i < num_device_buttons; i++) {
    button_mapping[i] = i + 1;
  }
  error = XSetDeviceButtonMapping(display_, device, button_mapping.get(),
                                  num_device_buttons);
  if (error != Success)
    LOG(ERROR) << "Failed to set XTest device button mapping: " << error;

  XCloseDevice(display_, device);
}

int EventExecutorLinux::Core::MouseButtonToX11ButtonNumber(
    MouseEvent::MouseButton button) {
  switch (button) {
    case MouseEvent::BUTTON_LEFT:
      return pointer_button_map_[0];

    case MouseEvent::BUTTON_RIGHT:
      return pointer_button_map_[2];

    case MouseEvent::BUTTON_MIDDLE:
      return pointer_button_map_[1];

    case MouseEvent::BUTTON_UNDEFINED:
    default:
      return -1;
  }
}

int EventExecutorLinux::Core::HorizontalScrollWheelToX11ButtonNumber(int dx) {
  return (dx > 0 ? pointer_button_map_[5] : pointer_button_map_[6]);
}


int EventExecutorLinux::Core::VerticalScrollWheelToX11ButtonNumber(int dy) {
  // Positive y-values are wheel scroll-up events (button 4), negative y-values
  // are wheel scroll-down events (button 5).
  return (dy > 0 ? pointer_button_map_[3] : pointer_button_map_[4]);
}

void EventExecutorLinux::Core::Start(
    scoped_ptr<protocol::ClipboardStub> client_clipboard) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&Core::Start, this, base::Passed(&client_clipboard)));
    return;
  }

  InitMouseButtonMap();
#if defined(REMOTING_HOST_LINUX_CLIPBOARD)
  clipboard_->Start(client_clipboard.Pass());
#endif  // REMOTING_HOST_LINUX_CLIPBOARD
}

void EventExecutorLinux::Core::Stop() {
#if defined(REMOTING_HOST_LINUX_CLIPBOARD)
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(FROM_HERE, base::Bind(&Core::Stop, this));
    return;
  }

  clipboard_->Stop();
#endif  // REMOTING_HOST_LINUX_CLIPBOARD
}

}  // namespace

scoped_ptr<EventExecutor> EventExecutor::Create(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner) {
  scoped_ptr<EventExecutorLinux> executor(
      new EventExecutorLinux(main_task_runner));
  if (!executor->Init())
    return scoped_ptr<EventExecutor>(NULL);
  return executor.PassAs<EventExecutor>();
}

}  // namespace remoting
