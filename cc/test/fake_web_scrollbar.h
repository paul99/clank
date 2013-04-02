// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_FAKE_WEB_SCROLLBAR_H_
#define CC_TEST_FAKE_WEB_SCROLLBAR_H_

#include "base/memory/scoped_ptr.h"
#include <public/WebScrollbar.h>

namespace cc {

class FakeWebScrollbar : public WebKit::WebScrollbar {
 public:
  static scoped_ptr<FakeWebScrollbar> create() {
    return make_scoped_ptr(new FakeWebScrollbar());
  }

  void setOverlay(bool isOverlay);

  // WebScrollbar implementation
  virtual bool isOverlay() const OVERRIDE;
  virtual int value() const OVERRIDE;
  virtual WebKit::WebPoint location() const OVERRIDE;
  virtual WebKit::WebSize size() const OVERRIDE;
  virtual bool enabled() const OVERRIDE;
  virtual int maximum() const OVERRIDE;
  virtual int totalSize() const OVERRIDE;
  virtual bool isScrollViewScrollbar() const OVERRIDE;
  virtual bool isScrollableAreaActive() const OVERRIDE;
  virtual void getTickmarks(WebKit::WebVector<WebKit::WebRect>&) const OVERRIDE {}
  virtual ScrollbarControlSize controlSize() const OVERRIDE;
  virtual ScrollbarPart pressedPart() const OVERRIDE;
  virtual ScrollbarPart hoveredPart() const OVERRIDE;
  virtual ScrollbarOverlayStyle scrollbarOverlayStyle() const OVERRIDE;
  virtual bool isCustomScrollbar() const OVERRIDE;
  virtual Orientation orientation() const OVERRIDE;

 private:
  FakeWebScrollbar();

  bool m_isOverlay;
};

}  // namespace cc

#endif  // CC_TEST_FAKE_WEB_SCROLLBAR_H_
