// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef _WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <time.h>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/dev/ppb_console_dev.h"
#include "ppapi/c/dev/ppb_fullscreen_dev.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/var.h"
#include "ppapi/demos/fingerpaint.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/effects/SkGradientShader.h"

void FingerPaintCallback(void* data, int32_t result) {
  static_cast<Fingerpaint*>(data)->Draw();
}

#if defined(_WIN32)
struct timeval {
  long tv_sec;
  long tv_usec;
};

struct timezone {
  long x;
  long y;
};

#define EPOCHFILETIME (116444736000000000i64)

int gettimeofday(struct timeval *tv, struct timezone*) {
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  LARGE_INTEGER li = {ft.dwLowDateTime, ft.dwHighDateTime};
  __int64 t = li.QuadPart - EPOCHFILETIME;
  tv->tv_sec  = static_cast<long>(t / 10000000);
  tv->tv_usec = static_cast<long>(t % 10000000);
  return 0;
}
#endif  // if defined(_WIN32)

REGISTER_DEMO(Fingerpaint);

#define FRAMES_TO_COUNT 20

Fingerpaint::Fingerpaint(PP_Instance instance)
                             : pp::Instance(instance),
                               frames_rendered_(0),
                               mouse_down_(false),
                               drawing_(false) {
  colors_[0] = SK_ColorBLUE;
  colors_[1] = SK_ColorRED;
  colors_[2] = SK_ColorGREEN;
  colors_[3] = SK_ColorCYAN;
  colors_[4] = SK_ColorMAGENTA;
  colors_[5] = SK_ColorYELLOW;
}
Fingerpaint::~Fingerpaint() {}

bool Fingerpaint::Init(uint32_t argc, const char* argn[], const char* argv[]) {
  RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_TOUCH);
  SetupHTML();
  return true;
}

bool Fingerpaint::HandleInputEvent(const pp::InputEvent& event) {
  Log("Received input event");
  switch (event.GetType()) {
    case PP_INPUTEVENT_TYPE_TOUCHSTART:
      ReportMultitouchEvent(pp::TouchInputEvent_Dev(event));
      return true;
    case PP_INPUTEVENT_TYPE_TOUCHMOVE:
      ReportMultitouchEvent(pp::TouchInputEvent_Dev(event));
      return true;
    case PP_INPUTEVENT_TYPE_TOUCHEND:
      ReportMultitouchEvent(pp::TouchInputEvent_Dev(event));
      return true;
    case PP_INPUTEVENT_TYPE_TOUCHCANCEL:
      return true;
    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
      mouse_down_ = true;
      return true;
    case PP_INPUTEVENT_TYPE_MOUSEUP:
      mouse_down_ = false;
      previous_points_.clear();
      return true;
    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
      if (mouse_down_)
        ReportMouseEvent(pp::MouseInputEvent(event));
      return true;
    default:
      break;
  }
  return false;
}

void Fingerpaint::HandleMessage(const pp::Var& message_data) {
  Log("Receiving message!");
  if (message_data.is_string()){
    std::string msg(message_data.AsString());
    if (msg == "Clear")
      Clear();
    else if (msg == "FullScreen")
      FullScreen();
    else
      Log("Unknown Message");
  }
}

void Fingerpaint::SetupHTML() {
  std::string bodyHTML = "ReplaceUpperBodyHTML:"
      "<p><button onclick=\""
      "document.getElementById('plugin').postMessage('Clear');"
      "\">Clear</button>"
      "<button onclick=\""
      "document.getElementById('plugin').postMessage('FullScreen');"
      "\">Enter FullScreen</button>"
      "<div id=\"fps_div\">FPS: </div><div id=\"ts_div\">Time: </div></p>";
  PostMessage(bodyHTML);

  std::string header = "AppendHeaderHTML:<style type=\"text/css\">"
      "embed {"
      "border-width:2px;"
      "border-style:solid;"
      "width:800px;"
      "height:400px; }"
      "</style>";
  PostMessage(header);
}

void Fingerpaint::ReportMouseEvent(const pp::MouseInputEvent& event) {
  pp::Point pt = event.GetPosition();
  ReportCoords(0, pt.x(), pt.y(), 0, 0);
  UpdateTime(event.GetTimeStamp());
}

void Fingerpaint::ReportMultitouchEvent(const pp::TouchInputEvent_Dev& event) {
  UpdateTime(event.GetTimeStamp());
  uint32_t num_touches = event.GetTouchCount(PP_TOUCHLIST_TYPE_CHANGEDTOUCHES);
  for (uint32_t i = 0; i < num_touches; i++) {
    pp::TouchPoint_Dev pt =
        event.GetTouchByIndex(PP_TOUCHLIST_TYPE_CHANGEDTOUCHES, i);
    std::string debugmsg = "";
    switch (event.GetType()) {
      case PP_INPUTEVENT_TYPE_TOUCHSTART:
        debugmsg += "Pressed";
        ReportCoords(pt.id(), pt.position().x(), pt.position().y(),
                         pt.radii().x(), pt.pressure());
        break;
      case PP_INPUTEVENT_TYPE_TOUCHEND:
        debugmsg += "Released";
        previous_points_[pt.id()].x = -1;
        previous_points_[pt.id()].y = -1;
        break;
      case PP_INPUTEVENT_TYPE_TOUCHMOVE:
        debugmsg += "Moved";
        ReportCoords(pt.id(), pt.position().x(), pt.position().y(),
                     pt.radii().x(), pt.pressure());
        break;
      case PP_INPUTEVENT_TYPE_TOUCHCANCEL:
        debugmsg += "Cancelled";
        break;
      default:
        break;
    }

    debugmsg += ": " + ToString(pt.position().x()) + ", ";
    debugmsg += ToString(pt.position().y()) + ", " + ToString(pt.pressure());
    debugmsg += ", " + ToString(pt.radii().x()) + ")";
    Log(debugmsg);
  }
}

void Fingerpaint::ReportCoords(int id, double x, double y, double size,
                                  double pressure) {
  double radius = size / RADIUS_ADJ_ * SCALE_RADIUS_;
  if (radius == 0) // Android sometimes gives us 0-sized touch points. If this
    radius = 25;   // happens, arbitrarily set a size so that something gets
                   // drawn.

  SkColor color = SK_ColorBLACK;
  if (id <= NUM_COLORS_)
    color = colors_[id];
  double color_scaler = (pressure < 1 ? pressure : 1);
  int r = ((double) SkColorGetR(color)) * color_scaler;
  int g = ((double) SkColorGetG(color)) * color_scaler;
  int b = ((double) SkColorGetB(color)) * color_scaler;
  color = SkColorSetRGB(r, g, b);

  circle newCir = {x, y, radius, color};

  double prev_x = previous_points_[id].x;
  double prev_y = previous_points_[id].y;
  if (0 < prev_x && prev_x < width_ &&
      0 < prev_y && prev_y < height_)
    DrawLine(id, previous_points_[id], newCir);
  else
    DrawLine(id, newCir, newCir);
  previous_points_[id] = newCir;
}

void Fingerpaint::DidChangeView(const pp::Rect& position,
                                const pp::Rect& clip) {
  if (position.size().width() == width_ &&
      position.size().height() == height_ &&
      position.x() == pos_x_ && position.y() == pos_y_) {
    return;
  }
  width_ = position.size().width();
  height_ = position.size().height();
  pos_x_ = position.x();
  pos_y_ = position.y();
  previous_points_.clear();
  device_context_ = pp::Graphics2D(this, pp::Size(width_, height_), false);
  if (!BindGraphics(device_context_))
    return;

  DrawInit();
}

void Fingerpaint::Draw() {
  device_context_.PaintImageData(img_, pp::Point(0, 0));
  int status =
      device_context_.Flush(pp::CompletionCallback(&FingerPaintCallback, this));
  if (status == PP_OK)
    drawing_ = true;
  if (++frames_rendered_ >= FRAMES_TO_COUNT)
    UpdateFps();
}

void Fingerpaint::DrawInit() {
  img_ = pp::ImageData(this, pp::ImageData::GetNativeImageDataFormat(),
                      pp::Size(width_, height_), false);
  bitmap_.setConfig(SkBitmap::kARGB_8888_Config, img_.size().width(),
                    img_.size().height(), img_.stride());
  bitmap_.setPixels(img_.data());
  canvas_.setBitmapDevice(bitmap_);
  if (!drawing_)
    Draw();
}

void Fingerpaint::DrawLine(int id, circle a, circle b) {
  int gradientSize = 2;
  SkPoint gradientPoints[gradientSize];
  gradientPoints[0].set(a.x, a.y);
  gradientPoints[1].set(b.x, b.y);
  SkColor gradientColors[gradientSize];
  gradientColors[0] = a.color;
  gradientColors[1] = b.color;
  SkShader* shader = SkGradientShader::CreateLinear(gradientPoints,
                                                    gradientColors,
                                                    NULL,
                                                    gradientSize,
                                                    SkShader::kClamp_TileMode);

  SkPaint paint;
  paint.setColor(b.color);
  paint.setFlags(SkPaint::kAntiAlias_Flag);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setStrokeWidth(SkIntToScalar(0));
  paint.setShader(shader);

  double d_x = b.x - a.x;
  double d_y = b.y - a.y;
  double dist = sqrt(pow(d_x, 2) + pow(d_y, 2));
  double u_x = d_y / dist;
  double u_y = -d_x / dist;
  double a_x = u_x * a.radius;
  double a_y = u_y * a.radius;
  double b_x = u_x * b.radius;
  double b_y = u_y * b.radius;

  SkPath path;
  path.setFillType(SkPath::kWinding_FillType);
  path.moveTo(b.x - b_x, b.y - b_y);
  path.lineTo(b.x + b_x, b.y + b_y);
  path.lineTo(a.x + a_x, a.y + a_y);
  path.lineTo(a.x - a_x, a.y - a_y);
  path.lineTo(b.x - b_x, b.y - b_y);
  canvas_.drawCircle(b.x, b.y, b.radius, paint);
  canvas_.drawPath(path, paint);
  shader->unref();
}

void Fingerpaint::Clear() {
  canvas_.drawColor(SK_ColorWHITE);
}

void Fingerpaint::FullScreen() {
    const PPB_Fullscreen_Dev* fullScreen =
        reinterpret_cast<const PPB_Fullscreen_Dev*>(
        pp::Module::Get()->GetBrowserInterface(PPB_FULLSCREEN_DEV_INTERFACE));
    if (!fullScreen)
      return;
    fullScreen->SetFullscreen(pp_instance(), PP_TRUE);
}

void Fingerpaint::UpdateFps() {
  struct timeval tv;
  struct timezone tz = {0, 0};
  gettimeofday(&tv, &tz);

  double current_time = tv.tv_sec + tv.tv_usec / 1000000.0;
  double elapsed_time = current_time - time_at_last_count_;
  double avg_time_per_frame = elapsed_time / FRAMES_TO_COUNT;
  int fps = (int) 1 / avg_time_per_frame;

  std::string msg = "Execute:document.getElementById('fps_div').innerHTML='";
  msg += "FPS: " + ToString(fps) + "';";
  PostMessage(msg);
  frames_rendered_ = 0;
  time_at_last_count_ = current_time;

}

void Fingerpaint::UpdateTime(double time) {
  std::string ts_str = "Execute:document.getElementById('ts_div').innerHTML = ";
  ts_str += "'Time (ms): " + ToString((int) (time * 1000.0)) + "';";
  PostMessage(ts_str);
}

void Fingerpaint::Log(const pp::Var& value) {
  const PPB_Console_Dev* console = reinterpret_cast<const PPB_Console_Dev*>(
      pp::Module::Get()->GetBrowserInterface(PPB_CONSOLE_DEV_INTERFACE));
  if (!console)
    return;
  console->Log(pp_instance(), PP_LOGLEVEL_LOG, value.pp_var());
}

std::string Fingerpaint::ToString(double i) {
  char str[100];
  sprintf(str, "%f", i);
  return std::string(str);
}

std::string Fingerpaint::ToString(int i) {
  char str[100];
  sprintf(str, "%d", i);
  return std::string(str);
}
