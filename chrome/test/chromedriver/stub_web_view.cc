// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/values.h"
#include "chrome/test/chromedriver/status.h"
#include "chrome/test/chromedriver/stub_web_view.h"
#include "chrome/test/chromedriver/ui_events.h"

StubWebView::StubWebView() {}

StubWebView::~StubWebView() {}

std::string StubWebView::GetId() {
  return "";
}

Status StubWebView::Load(const std::string& url) {
  return Status(kOk);
}

Status StubWebView::Reload() {
  return Status(kOk);
}

Status StubWebView::EvaluateScript(const std::string& frame,
                                   const std::string& function,
                                   scoped_ptr<base::Value>* result) {
  return Status(kOk);
}

Status StubWebView::CallFunction(const std::string& frame,
                                 const std::string& function,
                                 const base::ListValue& args,
                                 scoped_ptr<base::Value>* result) {
  return Status(kOk);
}

Status StubWebView::GetFrameByFunction(const std::string& frame,
                                       const std::string& function,
                                       const base::ListValue& args,
                                       std::string* out_frame) {
  return Status(kOk);
}

Status StubWebView::DispatchMouseEvents(const std::list<MouseEvent>& events) {
  return Status(kOk);
}

Status StubWebView::DispatchKeyEvents(const std::list<KeyEvent>& events) {
  return Status(kOk);
}

Status StubWebView::WaitForPendingNavigations(const std::string& frame_id) {
  return Status(kOk);
}

Status StubWebView::GetMainFrame(std::string* frame_id) {
  return Status(kOk);
}
