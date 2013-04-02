// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/overdraw_metrics.h"

#include "base/debug/trace_event.h"
#include "base/metrics/histogram.h"
#include "cc/layer_tree_host.h"
#include "cc/layer_tree_host_impl.h"
#include "cc/math_util.h"
#include "ui/gfx/quad_f.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/transform.h"

namespace cc {

OverdrawMetrics::OverdrawMetrics(bool recordMetricsForFrame)
    : m_recordMetricsForFrame(recordMetricsForFrame)
    , m_pixelsPainted(0)
    , m_pixelsUploadedOpaque(0)
    , m_pixelsUploadedTranslucent(0)
    , m_tilesCulledForUpload(0)
    , m_contentsTextureUseBytes(0)
    , m_renderSurfaceTextureUseBytes(0)
    , m_pixelsDrawnOpaque(0)
    , m_pixelsDrawnTranslucent(0)
    , m_pixelsCulledForDrawing(0)
{
}

static inline float wedgeProduct(const gfx::PointF& p1, const gfx::PointF& p2)
{
    return p1.x() * p2.y() - p1.y() * p2.x();
}

// Calculates area of an arbitrary convex polygon with up to 8 points.
static inline float polygonArea(const gfx::PointF points[8], int numPoints)
{
    if (numPoints < 3)
        return 0;

    float area = 0;
    for (int i = 0; i < numPoints; ++i)
        area += wedgeProduct(points[i], points[(i+1)%numPoints]);
    return fabs(0.5f * area);
}

// Takes a given quad, maps it by the given transformation, and gives the area of the resulting polygon.
static inline float areaOfMappedQuad(const gfx::Transform& transform, const gfx::QuadF& quad)
{
    gfx::PointF clippedQuad[8];
    int numVerticesInClippedQuad = 0;
    MathUtil::mapClippedQuad(transform, quad, clippedQuad, numVerticesInClippedQuad);
    return polygonArea(clippedQuad, numVerticesInClippedQuad);
}

void OverdrawMetrics::didPaint(const gfx::Rect& paintedRect)
{
    if (!m_recordMetricsForFrame)
        return;

    m_pixelsPainted += static_cast<float>(paintedRect.width()) * paintedRect.height();
}

void OverdrawMetrics::didCullTilesForUpload(int count)
{
    if (m_recordMetricsForFrame)
        m_tilesCulledForUpload += count;
}

void OverdrawMetrics::didUpload(const gfx::Transform& transformToTarget, const gfx::Rect& uploadRect, const gfx::Rect& opaqueRect)
{
    if (!m_recordMetricsForFrame)
        return;

    float uploadArea = areaOfMappedQuad(transformToTarget, gfx::QuadF(uploadRect));
    float uploadOpaqueArea = areaOfMappedQuad(transformToTarget, gfx::QuadF(gfx::IntersectRects(opaqueRect, uploadRect)));

    m_pixelsUploadedOpaque += uploadOpaqueArea;
    m_pixelsUploadedTranslucent += uploadArea - uploadOpaqueArea;
}

void OverdrawMetrics::didUseContentsTextureMemoryBytes(size_t contentsTextureUseBytes)
{
    if (!m_recordMetricsForFrame)
        return;

    m_contentsTextureUseBytes += contentsTextureUseBytes;
}

void OverdrawMetrics::didUseRenderSurfaceTextureMemoryBytes(size_t renderSurfaceUseBytes)
{
    if (!m_recordMetricsForFrame)
        return;

    m_renderSurfaceTextureUseBytes += renderSurfaceUseBytes;
}

void OverdrawMetrics::didCullForDrawing(const gfx::Transform& transformToTarget, const gfx::Rect& beforeCullRect, const gfx::Rect& afterCullRect)
{
    if (!m_recordMetricsForFrame)
        return;

    float beforeCullArea = areaOfMappedQuad(transformToTarget, gfx::QuadF(beforeCullRect));
    float afterCullArea = areaOfMappedQuad(transformToTarget, gfx::QuadF(afterCullRect));

    m_pixelsCulledForDrawing += beforeCullArea - afterCullArea;
}

void OverdrawMetrics::didDraw(const gfx::Transform& transformToTarget, const gfx::Rect& afterCullRect, const gfx::Rect& opaqueRect)
{
    if (!m_recordMetricsForFrame)
        return;

    float afterCullArea = areaOfMappedQuad(transformToTarget, gfx::QuadF(afterCullRect));
    float afterCullOpaqueArea = areaOfMappedQuad(transformToTarget, gfx::QuadF(gfx::IntersectRects(opaqueRect, afterCullRect)));

    m_pixelsDrawnOpaque += afterCullOpaqueArea;
    m_pixelsDrawnTranslucent += afterCullArea - afterCullOpaqueArea;
}

void OverdrawMetrics::recordMetrics(const LayerTreeHost* layerTreeHost) const
{
    if (m_recordMetricsForFrame)
        recordMetricsInternal<LayerTreeHost>(UpdateAndCommit, layerTreeHost);
}

void OverdrawMetrics::recordMetrics(const LayerTreeHostImpl* layerTreeHost) const
{
    if (m_recordMetricsForFrame)
        recordMetricsInternal<LayerTreeHostImpl>(DrawingToScreen, layerTreeHost);
}

template<typename LayerTreeHostType>
void OverdrawMetrics::recordMetricsInternal(MetricsType metricsType, const LayerTreeHostType* layerTreeHost) const
{
    // This gives approximately 10x the percentage of pixels to fill the viewport once.
    float normalization = 1000.f / (layerTreeHost->deviceViewportSize().width() * layerTreeHost->deviceViewportSize().height());
    // This gives approximately 100x the percentage of tiles to fill the viewport once, if all tiles were 256x256.
    float tileNormalization = 10000.f / (layerTreeHost->deviceViewportSize().width() / 256.f * layerTreeHost->deviceViewportSize().height() / 256.f);
    // This gives approximately 10x the percentage of bytes to fill the viewport once, assuming 4 bytes per pixel.
    float byteNormalization = normalization / 4;

    switch (metricsType) {
    case DrawingToScreen: {
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.pixelCountOpaque_Draw",
            static_cast<int>(normalization * m_pixelsDrawnOpaque),
            100, 1000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.pixelCountTranslucent_Draw",
            static_cast<int>(normalization * m_pixelsDrawnTranslucent),
            100, 1000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.pixelCountCulled_Draw",
            static_cast<int>(normalization * m_pixelsCulledForDrawing),
            100, 1000000, 50);

        TRACE_COUNTER_ID1("cc", "DrawPixelsCulled", layerTreeHost, m_pixelsCulledForDrawing);
        TRACE_EVENT2("cc", "OverdrawMetrics", "PixelsDrawnOpaque", m_pixelsDrawnOpaque, "PixelsDrawnTranslucent", m_pixelsDrawnTranslucent);
        break;
    }
    case UpdateAndCommit: {
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.pixelCountPainted",
            static_cast<int>(normalization * m_pixelsPainted),
            100, 1000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.pixelCountOpaque_Upload",
            static_cast<int>(normalization * m_pixelsUploadedOpaque),
            100, 1000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.pixelCountTranslucent_Upload",
            static_cast<int>(normalization * m_pixelsUploadedTranslucent),
            100, 1000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.tileCountCulled_Upload",
            static_cast<int>(tileNormalization * m_tilesCulledForUpload),
            100, 10000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.renderSurfaceTextureBytes_ViewportScaled",
            static_cast<int>(
                byteNormalization * m_renderSurfaceTextureUseBytes),
            10, 1000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.renderSurfaceTextureBytes_Unscaled",
            static_cast<int>(m_renderSurfaceTextureUseBytes / 1000),
            1000, 100000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.contentsTextureBytes_ViewportScaled",
            static_cast<int>(byteNormalization * m_contentsTextureUseBytes),
            10, 1000000, 50);
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "Renderer4.contentsTextureBytes_Unscaled",
            static_cast<int>(m_contentsTextureUseBytes / 1000),
            1000, 100000000, 50);

        {
            TRACE_COUNTER_ID1("cc", "UploadTilesCulled", layerTreeHost, m_tilesCulledForUpload);
            TRACE_EVENT2("cc", "OverdrawMetrics", "PixelsUploadedOpaque", m_pixelsUploadedOpaque, "PixelsUploadedTranslucent", m_pixelsUploadedTranslucent);
        }
        {
            // This must be in a different scope than the TRACE_EVENT2 above.
            TRACE_EVENT1("cc", "OverdrawPaintMetrics", "PixelsPainted", m_pixelsPainted);
        }
        {
            // This must be in a different scope than the TRACE_EVENTs above.
            TRACE_EVENT2("cc", "OverdrawPaintMetrics", "ContentsTextureBytes", m_contentsTextureUseBytes, "RenderSurfaceTextureBytes", m_renderSurfaceTextureUseBytes);
        }
        break;
    }
    }
}

}  // namespace cc
