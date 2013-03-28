// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/scrollbar_geometry_fixed_thumb.h"

#include <cmath>
#include <public/WebRect.h>
#include <public/WebScrollbar.h>

using WebKit::WebRect;
using WebKit::WebScrollbar;
using WebKit::WebScrollbarThemeGeometry;

namespace cc {

scoped_ptr<ScrollbarGeometryFixedThumb> ScrollbarGeometryFixedThumb::create(scoped_ptr<WebScrollbarThemeGeometry> geometry)
{
    return make_scoped_ptr(new ScrollbarGeometryFixedThumb(geometry.Pass()));
}

ScrollbarGeometryFixedThumb::~ScrollbarGeometryFixedThumb()
{
}

void ScrollbarGeometryFixedThumb::update(WebScrollbar* scrollbar)
{
    int length = ScrollbarGeometryStub::thumbLength(scrollbar);

    if (scrollbar->orientation() == WebScrollbar::Horizontal)
        m_thumbSize = gfx::Size(length, scrollbar->size().height);
    else
        m_thumbSize = gfx::Size(scrollbar->size().width, length);
}

WebScrollbarThemeGeometry* ScrollbarGeometryFixedThumb::clone() const
{
    ScrollbarGeometryFixedThumb* geometry = new ScrollbarGeometryFixedThumb(make_scoped_ptr(ScrollbarGeometryStub::clone()));
    geometry->m_thumbSize = m_thumbSize;
    return geometry;
}

int ScrollbarGeometryFixedThumb::thumbLength(WebScrollbar* scrollbar)
{
    if (scrollbar->orientation() == WebScrollbar::Horizontal)
        return m_thumbSize.width();
    return m_thumbSize.height();
}

int ScrollbarGeometryFixedThumb::thumbPosition(WebScrollbar* scrollbar)
{
    if (scrollbar->enabled()) {
        float size = scrollbar->maximum();
        if (!size)
            return 1;
        int value = std::min(std::max(0, scrollbar->value()), scrollbar->maximum());
        float pos = (trackLength(scrollbar) - thumbLength(scrollbar)) * value / size;
        return static_cast<int>(floorf((pos < 1 && pos > 0) ? 1 : pos));
    }
    return 0;
}
void ScrollbarGeometryFixedThumb::splitTrack(WebScrollbar* scrollbar, const WebRect& unconstrainedTrackRect, WebRect& beforeThumbRect, WebRect& thumbRect, WebRect& afterThumbRect)
{
    // This is a reimplementation of ScrollbarThemeComposite::splitTrack.
    // Because the WebScrollbarThemeGeometry functions call down to native
    // ScrollbarThemeComposite code which uses ScrollbarThemeComposite virtual
    // helpers, there's no way to override a helper like thumbLength from
    // the WebScrollbarThemeGeometry level. So, these three functions
    // (splitTrack, thumbPosition, thumbLength) are copied here so that the
    // WebScrollbarThemeGeometry helper functions are used instead and
    // a fixed size thumbLength can be used.

    WebRect trackRect = constrainTrackRectToTrackPieces(scrollbar, unconstrainedTrackRect);
    int thickness = scrollbar->orientation() == WebScrollbar::Horizontal ? scrollbar->size().height : scrollbar->size().width;
    int thumbPos = thumbPosition(scrollbar);
    if (scrollbar->orientation() == WebScrollbar::Horizontal) {
        thumbRect = WebRect(trackRect.x + thumbPos, trackRect.y + (trackRect.height - thickness) / 2, thumbLength(scrollbar), thickness);
        beforeThumbRect = WebRect(trackRect.x, trackRect.y, thumbPos + thumbRect.width / 2, trackRect.height);
        afterThumbRect = WebRect(trackRect.x + beforeThumbRect.width, trackRect.y, trackRect.x + trackRect.width - beforeThumbRect.x - beforeThumbRect.width, trackRect.height);
    } else {
        thumbRect = WebRect(trackRect.x + (trackRect.width - thickness) / 2, trackRect.y + thumbPos, thickness, thumbLength(scrollbar));
        beforeThumbRect = WebRect(trackRect.x, trackRect.y, trackRect.width, thumbPos + thumbRect.height / 2);
        afterThumbRect = WebRect(trackRect.x, trackRect.y + beforeThumbRect.height, trackRect.width, trackRect.y + trackRect.height - beforeThumbRect.y - beforeThumbRect.height);
    }
}

ScrollbarGeometryFixedThumb::ScrollbarGeometryFixedThumb(scoped_ptr<WebScrollbarThemeGeometry> geometry)
    : ScrollbarGeometryStub(geometry.Pass())
{
}

}  // namespace cc
