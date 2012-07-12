// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "printing/printing_context.h"

namespace printing {

PrintingContext::PrintingContext(const std::string& app_locale) {
  NOTIMPLEMENTED();
}

PrintingContext::~PrintingContext() {
}

void
PrintingContext::AskUserForSettings(gfx::NativeView parent_view, int max_pages,
                                    bool has_selection,
                                    const PrintSettingsCallback& cb) {
  NOTIMPLEMENTED();
}

PrintingContext::Result PrintingContext::UseDefaultSettings() {
  NOTIMPLEMENTED();
  return FAILED;
}

PrintingContext::Result
PrintingContext::InitWithSettings(const PrintSettings& settings) {
  NOTIMPLEMENTED();
  return FAILED;
}

PrintingContext::Result
PrintingContext::NewDocument(const string16& document_name) {
  NOTIMPLEMENTED();
  return FAILED;
}


PrintingContext::Result PrintingContext::NewPage() {
  NOTIMPLEMENTED();
  return FAILED;
}

PrintingContext::Result PrintingContext::PageDone() {
  NOTIMPLEMENTED();
  return FAILED;
}

PrintingContext::Result PrintingContext::DocumentDone() {
  NOTIMPLEMENTED();
  return FAILED;
}

void PrintingContext::Cancel() {
  NOTIMPLEMENTED();
}

void PrintingContext::ReleaseContext() {
  NOTIMPLEMENTED();
}

gfx::NativeDrawingContext PrintingContext::context() const {
  NOTIMPLEMENTED();
  return NULL;
}

// static
PrintingContext* PrintingContext::Create(const std::string& app_locale) {
  NOTIMPLEMENTED();
  return NULL;
}

} // namespace printing
