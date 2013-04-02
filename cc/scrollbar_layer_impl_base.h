// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SCROLLBAR_LAYER_IMPL_BASE_H_
#define CC_SCROLLBAR_LAYER_IMPL_BASE_H_

#include "cc/cc_export.h"
#include "cc/layer_impl.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebScrollbar.h"

namespace cc {

class CC_EXPORT ScrollbarLayerImplBase : public LayerImpl {
public:
    virtual ~ScrollbarLayerImplBase() { }

    virtual float currentPos() const = 0;
    virtual int totalSize() const = 0;
    virtual int maximum() const = 0;
    virtual WebKit::WebScrollbar::Orientation orientation() const = 0;

protected:
    ScrollbarLayerImplBase(LayerTreeImpl* treeImpl, int id)
        : LayerImpl(treeImpl, id) { }

};

} // namespace cc
#endif // CC_SCROLLBAR_LAYER_IMPL_BASE_H_
