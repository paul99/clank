// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web_scrollbar_layer_impl.h"

#include "cc/scrollbar_layer.h"
#include "web_layer_impl.h"

using cc::ScrollbarLayer;

namespace WebKit {

WebScrollbarLayer* WebScrollbarLayer::create(WebScrollbar* scrollbar, WebScrollbarThemePainter painter, WebScrollbarThemeGeometry* geometry)
{
    return new WebScrollbarLayerImpl(scrollbar, painter, geometry);
}


WebScrollbarLayerImpl::WebScrollbarLayerImpl(WebScrollbar* scrollbar, WebScrollbarThemePainter painter, WebScrollbarThemeGeometry* geometry)
    : m_layer(new WebLayerImpl(ScrollbarLayer::create(make_scoped_ptr(scrollbar), painter, make_scoped_ptr(geometry), 0)))
{
}

WebScrollbarLayerImpl::~WebScrollbarLayerImpl()
{
}

WebLayer* WebScrollbarLayerImpl::layer()
{
    return m_layer.get();
}

void WebScrollbarLayerImpl::setScrollLayer(WebLayer* layer)
{
    int id = layer ? static_cast<WebLayerImpl*>(layer)->layer()->id() : 0;
    static_cast<ScrollbarLayer*>(m_layer->layer())->setScrollLayerId(id);
}



} // namespace WebKit
