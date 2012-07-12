// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_PDF_METAFILE_ANDROID_H_
#define PRINTING_PDF_METAFILE_ANDROID_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "skia/ext/platform_device.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/rect.h"
#include "printing/metafile.h"


class FilePath;

namespace printing {

class PdfMetafile : public Metafile {
 public:
  PdfMetafile() { }
  virtual ~PdfMetafile() { }

  virtual bool Init() OVERRIDE;

  virtual bool InitFromData(const void* src_buffer,
                            uint32 src_buffer_size) OVERRIDE;

  virtual SkDevice* StartPageForVectorCanvas(
      const gfx::Size& page_size, const gfx::Rect& content_origin,
      const float& scale_factor) OVERRIDE;

  virtual bool StartPage(const gfx::Size& page_size,
                         const gfx::Rect& content_origin,
                         const float& scale_factor) OVERRIDE;

  virtual bool FinishPage() OVERRIDE;

  virtual bool FinishDocument() OVERRIDE;

  virtual uint32 GetDataSize() const OVERRIDE;

  virtual bool GetData(void* dst_buffer, uint32 dst_buffer_size) const OVERRIDE;

  virtual bool SaveTo(const FilePath& file_path) const OVERRIDE;

  virtual gfx::Rect GetPageBounds(unsigned int page_number) const OVERRIDE;

  virtual unsigned int GetPageCount() const OVERRIDE;

  virtual gfx::NativeDrawingContext context() const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(PdfMetafile);
};

}  // namespace printing

#endif  // PRINTING_PDF_METAFILE_ANDROID_H_
