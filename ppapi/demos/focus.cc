// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/var.h"
#include "ppapi/demos/demo_framework.h"
#include "ppapi/demos/focus.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"


void FocusCallback(void* data, int32_t result) {
}

REGISTER_DEMO(DemoFocus);

DemoFocus::DemoFocus(PP_Instance instance) : pp::Instance(instance),
                                             isPrimaryPlugin_(true),
                                             mouseInButtonArea_(false) {
}

DemoFocus::~DemoFocus() {}

bool DemoFocus::Init(uint32_t argc, const char* argn[], const char* argv[]) {
  RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
  SetupHTML();
  paintStyle_ = SkPaint::kStroke_Style;
  return true;
}

void DemoFocus::DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
  if(isPrimaryPlugin_)
    button_.setXYWH(58, 70, 40, 40);
  else
    button_.setXYWH(8, 65, 40, 40);
  width_ = position.width();
  height_ = position.height();
  ReDraw();
}

void DemoFocus::SetupHTML() {
  std::string bodyHTML = "ReplaceUpperBodyHTML:<form name=\"myForm\">"
    "Red Plugin Focused <input type=\"checkbox\" name=\"chkbx\" /></form>";
  PostMessage(bodyHTML);

  std::string header = "AppendHeaderHTML:<style type=\"text/css\">"
    "embed { border-width:2px; border-style:solid; } "
    "#plugin { border-color:red; }"
    "#plugin2 { position:absolute; left:50px; top:100px; border-color:blue; } "
    "</style>";
  PostMessage(header);

  std::string script = "Execute:";
  script.append("var plugin2 = document.getElementById('plugin2');");
  script.append("if (!plugin2) {");
  script.append("var plugin = document.getElementById('plugin');");
  script.append("plugin.width = 600;");
  script.append("plugin.height = 400;");
  script.append("plugin2 = document.createElement('EMBED');");
  script.append("plugin2.type = 'application/x-ppapi-demos';");
  script.append("plugin2.id = 'plugin2';");
  script.append("plugin2.src = 'DemoFocus';");
  script.append("plugin2.width = 200;");
  script.append("plugin2.height = 200;");
  script.append("document.getElementById('container').appendChild(plugin2);");
  script.append("plugin2.postMessage('NOTPRIMARY');");
  script.append("}");
  PostMessage(script);
}

void DemoFocus::HandleMessage(const pp::Var& message) {
  if (message.AsString() == "NOTPRIMARY") {
    isPrimaryPlugin_ = false;
    button_.setXYWH(8, 65, 40, 40);
    ReDraw();
  }
}

void DemoFocus::DidChangeFocus(bool has_focus) {
  std::string str = (isPrimaryPlugin_ ? "Red Plugin " : "Blue Plugin ");
  str.append((has_focus ? "focused" : "unfocused"));
  Debug(str);
  if (isPrimaryPlugin_) {
    std::string msg = "Execute:document.myForm.chkbx.checked=";
    msg.append((has_focus ? "true" : "false"));
    PostMessage(msg);
  }
}

void DemoFocus::ReDraw() {
  pp::ImageData img = pp::ImageData(this,
                                    pp::ImageData::GetNativeImageDataFormat(),
                                    pp::Size(width_, height_), false);
  SkBitmap bitmap;
  bitmap.setConfig(SkBitmap::kARGB_8888_Config, img.size().width(),
                    img.size().height(), img.stride());
  bitmap.setPixels(img.data());
  SkCanvas canvas;
  canvas.setBitmapDevice(bitmap);
  pp::Graphics2D device_context;
  device_context = pp::Graphics2D(this, pp::Size(width_, height_), false);
  if (!BindGraphics(device_context)) {
    return;
  }
  SkPaint paint;
  paint.setColor(isPrimaryPlugin_ ? SK_ColorRED : SK_ColorBLUE);
  paint.setStyle(paintStyle_);
  canvas.drawColor(0);
  canvas.drawIRect(button_, paint);
  device_context.PaintImageData(img, pp::Point(0, 0));
  device_context.Flush(pp::CompletionCallback(&FocusCallback, this));
}

bool DemoFocus::HandleInputEvent(const pp::InputEvent& event) {
  std::string str = "DemoFocus::HandleInputEvent(";
  str.append(isPrimaryPlugin_? "Red" : "Blue");
  str.append("):");
  str.append(IntToString(event.GetType()));
  Debug(str);
  pp::MouseInputEvent mevent = pp::MouseInputEvent(event);
  if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEDOWN) {
    mouseInButtonArea_ = InButtonArea(mevent.GetPosition());
  } else if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEMOVE) {
    if (!InButtonArea(mevent.GetPosition()))
      mouseInButtonArea_ = false;
  } else if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEUP) {
    if (mouseInButtonArea_ && InButtonArea(mevent.GetPosition()))
      ButtonPressed();
  } else {
    Debug("... Returning False");
    return false;
  }
  return mouseInButtonArea_;
}

bool DemoFocus::InButtonArea(pp::Point point) {
  return point.x() >= button_.fLeft && point.x() <= button_.fRight
      && point.y() >= button_.fTop && point.y() <= button_.fBottom;
}

void DemoFocus::ButtonPressed() {
  paintStyle_ = SkPaint::kFill_Style;
  ReDraw();
}

void DemoFocus::Debug(const pp::Var& value) {
  const PPB_Console_Dev* console = reinterpret_cast<const PPB_Console_Dev*>(
      pp::Module::Get()->GetBrowserInterface(PPB_CONSOLE_DEV_INTERFACE));
  if (!console)
    return;
  console->Log(pp_instance(), PP_LOGLEVEL_LOG, value.pp_var());
}

std::string DemoFocus::IntToString(int i) {
  char str[100];
  sprintf(str, "%d", i);
  return std::string(str);
}
