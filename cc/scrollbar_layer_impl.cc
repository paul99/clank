// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/scrollbar_layer_impl.h"

#include "cc/quad_sink.h"
#include "cc/scrollbar_animation_controller.h"
#include "cc/texture_draw_quad.h"
#include "ui/gfx/rect_conversions.h"

using WebKit::WebRect;
using WebKit::WebScrollbar;

namespace cc {

scoped_ptr<ScrollbarLayerImpl> ScrollbarLayerImpl::create(LayerTreeImpl* treeImpl, int id)
{
    return make_scoped_ptr(new ScrollbarLayerImpl(treeImpl, id));
}

ScrollbarLayerImpl::ScrollbarLayerImpl(LayerTreeImpl* treeImpl, int id)
    : ScrollbarLayerImplBase(treeImpl, id)
    , m_scrollbar(this)
    , m_backTrackResourceId(0)
    , m_foreTrackResourceId(0)
    , m_thumbResourceId(0)
    , m_scrollbarOverlayStyle(WebScrollbar::ScrollbarOverlayStyleDefault)
    , m_orientation(WebScrollbar::Horizontal)
    , m_controlSize(WebScrollbar::RegularScrollbar)
    , m_pressedPart(WebScrollbar::NoPart)
    , m_hoveredPart(WebScrollbar::NoPart)
    , m_isScrollableAreaActive(false)
    , m_isScrollViewScrollbar(false)
    , m_enabled(false)
    , m_isCustomScrollbar(false)
    , m_isOverlayScrollbar(false)
{
}

ScrollbarLayerImpl::~ScrollbarLayerImpl()
{
}

void ScrollbarLayerImpl::setScrollbarGeometry(scoped_ptr<ScrollbarGeometryFixedThumb> geometry)
{
    m_geometry = geometry.Pass();
}

void ScrollbarLayerImpl::setScrollbarData(WebScrollbar* scrollbar)
{
    m_scrollbarOverlayStyle = scrollbar->scrollbarOverlayStyle();
    m_orientation = scrollbar->orientation();
    m_controlSize = scrollbar->controlSize();
    m_pressedPart = scrollbar->pressedPart();
    m_hoveredPart = scrollbar->hoveredPart();
    m_isScrollableAreaActive = scrollbar->isScrollableAreaActive();
    m_isScrollViewScrollbar = scrollbar->isScrollViewScrollbar();
    m_enabled = scrollbar->enabled();
    m_isCustomScrollbar = scrollbar->isCustomScrollbar();
    m_isOverlayScrollbar = scrollbar->isOverlay();

    scrollbar->getTickmarks(m_tickmarks);

    m_geometry->update(scrollbar);
}

float ScrollbarLayerImpl::currentPos() const
{
    return m_currentPos;
}

int ScrollbarLayerImpl::totalSize() const
{
    return m_totalSize;
}

int ScrollbarLayerImpl::maximum() const
{
    return m_maximum;
}

WebKit::WebScrollbar::Orientation ScrollbarLayerImpl::orientation() const
{
    return m_orientation;
}

static gfx::RectF toUVRect(const gfx::Rect& r, const gfx::Rect& bounds)
{
    return gfx::ScaleRect(r, 1.0 / bounds.width(), 1.0 / bounds.height());
}

gfx::Rect ScrollbarLayerImpl::scrollbarLayerRectToContentRect(const gfx::Rect& layerRect) const
{
    // Don't intersect with the bounds as in layerRectToContentRect() because
    // layerRect here might be in coordinates of the containing layer.
    gfx::RectF contentRect = gfx::ScaleRect(layerRect, contentsScaleX(), contentsScaleY());
    return gfx::ToEnclosingRect(contentRect);
}

void ScrollbarLayerImpl::appendQuads(QuadSink& quadSink, AppendQuadsData& appendQuadsData)
{
    bool premultipledAlpha = false;
    bool flipped = false;
    gfx::RectF uvRect(0, 0, 1, 1);
    gfx::Rect boundsRect(gfx::Point(), bounds());
    gfx::Rect contentBoundsRect(gfx::Point(), contentBounds());

    SharedQuadState* sharedQuadState = quadSink.useSharedQuadState(createSharedQuadState());
    appendDebugBorderQuad(quadSink, sharedQuadState, appendQuadsData);

    WebRect thumbRect, backTrackRect, foreTrackRect;
    m_geometry->splitTrack(&m_scrollbar, m_geometry->trackRect(&m_scrollbar), backTrackRect, thumbRect, foreTrackRect);
    if (!m_geometry->hasThumb(&m_scrollbar))
        thumbRect = WebRect();

    if (m_thumbResourceId && !thumbRect.isEmpty()) {
        gfx::Rect quadRect(scrollbarLayerRectToContentRect(thumbRect));
        gfx::Rect opaqueRect;
        const float opacity[] = {1.0f, 1.0f, 1.0f, 1.0f};
        scoped_ptr<TextureDrawQuad> quad = TextureDrawQuad::Create();
        quad->SetNew(sharedQuadState, quadRect, opaqueRect, m_thumbResourceId, premultipledAlpha, uvRect, opacity, flipped);
        quadSink.append(quad.PassAs<DrawQuad>(), appendQuadsData);
    }

    if (!m_backTrackResourceId)
        return;

    // We only paint the track in two parts if we were given a texture for the forward track part.
    if (m_foreTrackResourceId && !foreTrackRect.isEmpty()) {
        gfx::Rect quadRect(scrollbarLayerRectToContentRect(foreTrackRect));
        gfx::Rect opaqueRect(contentsOpaque() ? quadRect : gfx::Rect());
        const float opacity[] = {1.0f, 1.0f, 1.0f, 1.0f};
        scoped_ptr<TextureDrawQuad> quad = TextureDrawQuad::Create();
        quad->SetNew(sharedQuadState, quadRect, opaqueRect, m_foreTrackResourceId, premultipledAlpha, toUVRect(foreTrackRect, boundsRect), opacity, flipped);
        quadSink.append(quad.PassAs<DrawQuad>(), appendQuadsData);
    }

    // Order matters here: since the back track texture is being drawn to the entire contents rect, we must append it after the thumb and
    // fore track quads. The back track texture contains (and displays) the buttons.
    if (!contentBoundsRect.IsEmpty()) {
        gfx::Rect quadRect(contentBoundsRect);
        gfx::Rect opaqueRect(contentsOpaque() ? quadRect : gfx::Rect());
        const float opacity[] = {1.0f, 1.0f, 1.0f, 1.0f};
        scoped_ptr<TextureDrawQuad> quad = TextureDrawQuad::Create();
        quad->SetNew(sharedQuadState, quadRect, opaqueRect, m_backTrackResourceId, premultipledAlpha, uvRect, opacity, flipped);
        quadSink.append(quad.PassAs<DrawQuad>(), appendQuadsData);
    }
}

void ScrollbarLayerImpl::didLoseOutputSurface()
{
    m_backTrackResourceId = 0;
    m_foreTrackResourceId = 0;
    m_thumbResourceId = 0;
}

bool ScrollbarLayerImpl::Scrollbar::isOverlay() const
{
    return m_owner->m_isOverlayScrollbar;
}

int ScrollbarLayerImpl::Scrollbar::value() const
{
    return m_owner->m_currentPos;
}

WebKit::WebPoint ScrollbarLayerImpl::Scrollbar::location() const
{
    return WebKit::WebPoint();
}

WebKit::WebSize ScrollbarLayerImpl::Scrollbar::size() const
{
    return WebKit::WebSize(m_owner->bounds().width(), m_owner->bounds().height());
}

bool ScrollbarLayerImpl::Scrollbar::enabled() const
{
    return m_owner->m_enabled;
}

int ScrollbarLayerImpl::Scrollbar::maximum() const
{
    return m_owner->m_maximum;
}

int ScrollbarLayerImpl::Scrollbar::totalSize() const
{
    return m_owner->m_totalSize;
}

bool ScrollbarLayerImpl::Scrollbar::isScrollViewScrollbar() const
{
    return m_owner->m_isScrollViewScrollbar;
}

bool ScrollbarLayerImpl::Scrollbar::isScrollableAreaActive() const
{
    return m_owner->m_isScrollableAreaActive;
}

void ScrollbarLayerImpl::Scrollbar::getTickmarks(WebKit::WebVector<WebRect>& tickmarks) const
{
    tickmarks = m_owner->m_tickmarks;
}

WebScrollbar::ScrollbarControlSize ScrollbarLayerImpl::Scrollbar::controlSize() const
{
    return m_owner->m_controlSize;
}

WebScrollbar::ScrollbarPart ScrollbarLayerImpl::Scrollbar::pressedPart() const
{
    return m_owner->m_pressedPart;
}

WebScrollbar::ScrollbarPart ScrollbarLayerImpl::Scrollbar::hoveredPart() const
{
    return m_owner->m_hoveredPart;
}

WebScrollbar::ScrollbarOverlayStyle ScrollbarLayerImpl::Scrollbar::scrollbarOverlayStyle() const
{
    return m_owner->m_scrollbarOverlayStyle;
}

WebScrollbar::Orientation ScrollbarLayerImpl::Scrollbar::orientation() const
{
    return m_owner->m_orientation;
}

bool ScrollbarLayerImpl::Scrollbar::isCustomScrollbar() const
{
    return m_owner->m_isCustomScrollbar;
}

const char* ScrollbarLayerImpl::layerTypeAsString() const
{
    return "ScrollbarLayer";
}

}  // namespace cc
