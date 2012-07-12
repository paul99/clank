// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_DEMOS_FINGERPAINT_H_
#define PPAPI_DEMOS_FINGERPAINT_H_

#include <map>
#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/dev/touch_event_dev.h"
#include "ppapi/demos/demo_framework.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"

#define SCALE_RADIUS_ 50
#define NUM_COLORS_ 6
#define PI 3.1415926
#define RADIUS_ADJ_ 1024

class Fingerpaint : public pp::Instance {
 public:
  explicit Fingerpaint(PP_Instance instance);
  virtual ~Fingerpaint();

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);
  virtual bool HandleInputEvent(const pp::InputEvent& event);
  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip);
  virtual void HandleMessage(const pp::Var& message_data);
  void Draw();
  void SetupHTML();

 private:

  struct circle {
    double x;
    double y;
    double radius;
    SkColor color;
  };

  void ReportMouseEvent(const pp::MouseInputEvent& event);

  void ReportMultitouchEvent(const pp::TouchInputEvent_Dev& event);

  void ReportCoords(int id, double x, double y, double size, double pressure);

  // Graphics Functions

  void DrawInit();
  void DrawLine(int id, circle a, circle b);
  void Clear();
  void FullScreen();
  void UpdateFps();
  void UpdateTime(double time);
  void Log(const pp::Var& value);
  std::string ToString(int i);
  std::string ToString(double i);

  double time_at_last_count_;
  int frames_rendered_;
  int width_;
  int height_;
  int pos_x_;
  int pos_y_;
  bool mouse_down_;
  bool drawing_;
  std::map<int, circle> previous_points_;
  pp::Graphics2D device_context_;
  pp::ImageData img_;
  SkCanvas canvas_;
  SkBitmap bitmap_;
  SkColor colors_[NUM_COLORS_];
};

#endif // PPAPI_DEMOS_FINGERPAINT_H_
