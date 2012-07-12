// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_DEMOS_FOCUS_H_
#define PPAPI_DEMOS_FOCUS_H_

#include "ppapi/c/dev/ppb_console_dev.h"
#include "ppapi/cpp/input_event.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkRect.h"

class DemoFocus : public pp::Instance {
 public:
  explicit DemoFocus(PP_Instance instance);
  virtual ~DemoFocus();

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);
  virtual void DidChangeFocus(bool has_focus);
  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip);
  virtual bool HandleInputEvent(const pp::InputEvent& event);
  virtual void HandleMessage(const pp::Var& message);

 private:
  void SetupHTML();
  void Debug(const pp::Var& value);
  void ReDraw();
  void ButtonPressed();
  bool InButtonArea(pp::Point point);
  std::string IntToString(int i);

  bool isPrimaryPlugin_;
  SkIRect button_;
  SkPaint::Style paintStyle_;
  bool mouseInButtonArea_;
  int32_t width_;
  int32_t height_;
};

#endif // PPAPI_DEMOS_FOCUS_H_
