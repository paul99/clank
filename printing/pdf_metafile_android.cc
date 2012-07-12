// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/pdf_metafile_android.h"

namespace printing {

bool PdfMetafile::Init() {
  NOTIMPLEMENTED();
  return false;
}

SkDevice* PdfMetafile::StartPageForVectorCanvas(
    const gfx::Size& page_size, const gfx::Rect& content_origin,
    const float& scale_factor) {
  NOTIMPLEMENTED();
  return NULL;
}

bool PdfMetafile::StartPage(const gfx::Size& page_size,
                       const gfx::Rect& content_origin,
                       const float& scale_factor) {
  NOTIMPLEMENTED();
  return 0;
}

bool PdfMetafile::FinishPage() {
  NOTIMPLEMENTED();
  return false;
}

bool PdfMetafile::FinishDocument() {
  NOTIMPLEMENTED();
  return false;
}

bool PdfMetafile::SaveTo(const FilePath& file_path) const {
  NOTIMPLEMENTED();
  return false;
}

gfx::Rect PdfMetafile::GetPageBounds(unsigned int page_number) const {
  NOTIMPLEMENTED();
  return gfx::Rect();
}

unsigned int PdfMetafile::GetPageCount() const {
  NOTIMPLEMENTED();
  return 0;
}

gfx::NativeDrawingContext PdfMetafile::context() const {
  NOTIMPLEMENTED();
  return 0;
}

} // namespace printing
