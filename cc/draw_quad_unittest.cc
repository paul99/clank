// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/draw_quad.h"

#include "cc/checkerboard_draw_quad.h"
#include "cc/debug_border_draw_quad.h"
#include "cc/io_surface_draw_quad.h"
#include "cc/math_util.h"
#include "cc/render_pass_draw_quad.h"
#include "cc/solid_color_draw_quad.h"
#include "cc/stream_video_draw_quad.h"
#include "cc/test/geometry_test_utils.h"
#include "cc/texture_draw_quad.h"
#include "cc/tile_draw_quad.h"
#include "cc/yuv_video_draw_quad.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/transform.h"

namespace cc {
namespace {

TEST(DrawQuadTest, copySharedQuadState)
{
    gfx::Transform quadTransform = MathUtil::createGfxTransform(1, 0.5, 0, 1, 0.5, 0);
    gfx::Rect visibleContentRect(10, 12, 14, 16);
    gfx::Rect clippedRectInTarget(19, 21, 23, 25);
    gfx::Rect clipRect = clippedRectInTarget;
    bool isClipped = true;
    float opacity = 0.25;

    scoped_ptr<SharedQuadState> state(SharedQuadState::Create());
    state->SetAll(quadTransform, visibleContentRect, clippedRectInTarget, clipRect, isClipped, opacity);

    scoped_ptr<SharedQuadState> copy(state->Copy());
    EXPECT_EQ(quadTransform, copy->content_to_target_transform);
    EXPECT_RECT_EQ(visibleContentRect, copy->visible_content_rect);
    EXPECT_RECT_EQ(clippedRectInTarget, copy->clipped_rect_in_target);
    EXPECT_EQ(opacity, copy->opacity);
    EXPECT_RECT_EQ(clipRect, copy->clip_rect);
    EXPECT_EQ(isClipped, copy->is_clipped);
}

scoped_ptr<SharedQuadState> createSharedQuadState()
{
    gfx::Transform quadTransform = MathUtil::createGfxTransform(1, 0.5, 0, 1, 0.5, 0);
    gfx::Rect visibleContentRect(10, 12, 14, 16);
    gfx::Rect clippedRectInTarget(19, 21, 23, 25);
    gfx::Rect clipRect = clippedRectInTarget;
    bool isClipped = false;
    float opacity = 1;

    scoped_ptr<SharedQuadState> state(SharedQuadState::Create());
    state->SetAll(quadTransform, visibleContentRect, clippedRectInTarget, clipRect, isClipped, opacity);
    return state.Pass();
}

void compareDrawQuad(DrawQuad* quad, DrawQuad* copy, SharedQuadState* copySharedState)
{
    EXPECT_EQ(quad->material, copy->material);
    EXPECT_RECT_EQ(quad->rect, copy->rect);
    EXPECT_RECT_EQ(quad->visible_rect, copy->visible_rect);
    EXPECT_RECT_EQ(quad->opaque_rect, copy->opaque_rect);
    EXPECT_EQ(quad->needs_blending, copy->needs_blending);
    EXPECT_EQ(copySharedState, copy->shared_quad_state);
}

#define CREATE_SHARED_STATE() \
    scoped_ptr<SharedQuadState> sharedState(createSharedQuadState()); \
    scoped_ptr<SharedQuadState> copySharedState(sharedState->Copy()); \

#define QUAD_DATA \
    gfx::Rect quadRect(30, 40, 50, 60); \
    gfx::Rect quadVisibleRect(40, 50, 30, 20); \
    gfx::Rect quadOpaqueRect(60, 55, 10, 10); \
    bool needsBlending = true;

#define SETUP_AND_COPY_QUAD_NEW(Type, quad) \
    scoped_ptr<DrawQuad> copyNew(quadNew->Copy(copySharedState.get())); \
    compareDrawQuad(quadNew.get(), copyNew.get(), copySharedState.get()); \
    const Type* copyQuad = Type::MaterialCast(copyNew.get());

#define SETUP_AND_COPY_QUAD_ALL(Type, quad) \
    scoped_ptr<DrawQuad> copyAll(quadAll->Copy(copySharedState.get())); \
    compareDrawQuad(quadAll.get(), copyAll.get(), copySharedState.get()); \
    copyQuad = Type::MaterialCast(copyAll.get());

#define SETUP_AND_COPY_QUAD_NEW_1(Type, quad, a) \
    scoped_ptr<DrawQuad> copyNew(quadNew->Copy(copySharedState.get(), a));  \
    compareDrawQuad(quadNew.get(), copyNew.get(), copySharedState.get()); \
    const Type* copyQuad = Type::MaterialCast(copyNew.get());

#define SETUP_AND_COPY_QUAD_ALL_1(Type, quad, a) \
    scoped_ptr<DrawQuad> copyAll(quadAll->Copy(copySharedState.get(), a)); \
    compareDrawQuad(quadAll.get(), copyAll.get(), copySharedState.get()); \
    copyQuad = Type::MaterialCast(copyAll.get());

#define CREATE_QUAD_1_NEW(Type, a) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_1_ALL(Type, a) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

#define CREATE_QUAD_2_NEW(Type, a, b) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_2_ALL(Type, a, b) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

#define CREATE_QUAD_3_NEW(Type, a, b, c) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b, c); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_3_ALL(Type, a, b, c) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b, c); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

#define CREATE_QUAD_4_NEW(Type, a, b, c, d) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b, c, d); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_4_ALL(Type, a, b, c, d) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b, c, d); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

#define CREATE_QUAD_5_NEW(Type, a, b, c, d, e) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b, c, d, e); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_5_ALL(Type, a, b, c, d, e) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b, c, d, e); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

#define CREATE_QUAD_5_NEW_1(Type, a, b, c, d, e, copyA) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b, c, d, e); } \
    SETUP_AND_COPY_QUAD_NEW_1(Type, quadNew, copyA);

#define CREATE_QUAD_5_ALL_1(Type, a, b, c, d, e, copyA) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b, c, d, e); } \
    SETUP_AND_COPY_QUAD_ALL_1(Type, quadAll, copyA);

#define CREATE_QUAD_6_NEW(Type, a, b, c, d, e, f) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b, c, d, e, f); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_6_ALL(Type, a, b, c, d, e, f) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b, c, d, e, f); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

#define CREATE_QUAD_7_NEW(Type, a, b, c, d, e, f, g) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b, c, d, e, f, g); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_7_ALL(Type, a, b, c, d, e, f, g) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b, c, d, e, f, g); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

#define CREATE_QUAD_8_NEW(Type, a, b, c, d, e, f, g, h) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b, c, d, e, f, g, h); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_8_ALL(Type, a, b, c, d, e, f, g, h) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b, c, d, e, f, g, h); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

#define CREATE_QUAD_9_NEW(Type, a, b, c, d, e, f, g, h, i) \
    scoped_ptr<Type> quadNew(Type::Create()); \
    { QUAD_DATA \
      quadNew->SetNew(sharedState.get(), quadRect, a, b, c, d, e, f, g, h, i); } \
    SETUP_AND_COPY_QUAD_NEW(Type, quadNew);

#define CREATE_QUAD_9_ALL(Type, a, b, c, d, e, f, g, h, i) \
    scoped_ptr<Type> quadAll(Type::Create()); \
    { QUAD_DATA \
      quadAll->SetAll(sharedState.get(), quadRect, quadOpaqueRect, quadVisibleRect, needsBlending, a, b, c, d, e, f, g, h, i); } \
    SETUP_AND_COPY_QUAD_ALL(Type, quadAll);

TEST(DrawQuadTest, copyCheckerboardDrawQuad)
{
    SkColor color = 0xfabb0011;
    CREATE_SHARED_STATE();

    CREATE_QUAD_1_NEW(CheckerboardDrawQuad, color);
    EXPECT_EQ(DrawQuad::CHECKERBOARD, copyQuad->material);
    EXPECT_EQ(color, copyQuad->color);

    CREATE_QUAD_1_ALL(CheckerboardDrawQuad, color);
    EXPECT_EQ(DrawQuad::CHECKERBOARD, copyQuad->material);
    EXPECT_EQ(color, copyQuad->color);
}

TEST(DrawQuadTest, copyDebugBorderDrawQuad)
{
    SkColor color = 0xfabb0011;
    int width = 99;
    CREATE_SHARED_STATE();

    CREATE_QUAD_2_NEW(DebugBorderDrawQuad, color, width);
    EXPECT_EQ(DrawQuad::DEBUG_BORDER, copyQuad->material);
    EXPECT_EQ(color, copyQuad->color);
    EXPECT_EQ(width, copyQuad->width);

    CREATE_QUAD_2_ALL(DebugBorderDrawQuad, color, width);
    EXPECT_EQ(DrawQuad::DEBUG_BORDER, copyQuad->material);
    EXPECT_EQ(color, copyQuad->color);
    EXPECT_EQ(width, copyQuad->width);
}

TEST(DrawQuadTest, copyIOSurfaceDrawQuad)
{
    gfx::Rect opaqueRect(3, 7, 10, 12);
    gfx::Size size(58, 95);
    unsigned textureId = 72;
    IOSurfaceDrawQuad::Orientation orientation = IOSurfaceDrawQuad::UNFLIPPED;
    CREATE_SHARED_STATE();

    CREATE_QUAD_4_NEW(IOSurfaceDrawQuad, opaqueRect, size, textureId, orientation);
    EXPECT_EQ(DrawQuad::IO_SURFACE_CONTENT, copyQuad->material);
    EXPECT_RECT_EQ(opaqueRect, copyQuad->opaque_rect);
    EXPECT_EQ(size, copyQuad->io_surface_size);
    EXPECT_EQ(textureId, copyQuad->io_surface_texture_id);
    EXPECT_EQ(orientation, copyQuad->orientation);

    CREATE_QUAD_3_ALL(IOSurfaceDrawQuad, size, textureId, orientation);
    EXPECT_EQ(DrawQuad::IO_SURFACE_CONTENT, copyQuad->material);
    EXPECT_EQ(size, copyQuad->io_surface_size);
    EXPECT_EQ(textureId, copyQuad->io_surface_texture_id);
    EXPECT_EQ(orientation, copyQuad->orientation);
}

TEST(DrawQuadTest, copyRenderPassDrawQuad)
{
    RenderPass::Id renderPassId(22, 64);
    bool isReplica = true;
    ResourceProvider::ResourceId maskResourceId = 78;
    gfx::Rect contentsChangedSinceLastFrame(42, 11, 74, 24);
    gfx::RectF maskUVRect(-45, -21, 33, 19);

    RenderPass::Id copiedRenderPassId(235, 11);
    CREATE_SHARED_STATE();

    CREATE_QUAD_5_NEW_1(RenderPassDrawQuad, renderPassId, isReplica, maskResourceId, contentsChangedSinceLastFrame, maskUVRect, copiedRenderPassId);
    EXPECT_EQ(DrawQuad::RENDER_PASS, copyQuad->material);
    EXPECT_EQ(copiedRenderPassId, copyQuad->render_pass_id);
    EXPECT_EQ(isReplica, copyQuad->is_replica);
    EXPECT_EQ(maskResourceId, copyQuad->mask_resource_id);
    EXPECT_RECT_EQ(contentsChangedSinceLastFrame, copyQuad->contents_changed_since_last_frame);
    EXPECT_EQ(maskUVRect.ToString(), copyQuad->mask_uv_rect.ToString());

    CREATE_QUAD_5_ALL_1(RenderPassDrawQuad, renderPassId, isReplica, maskResourceId, contentsChangedSinceLastFrame, maskUVRect, copiedRenderPassId);
    EXPECT_EQ(DrawQuad::RENDER_PASS, copyQuad->material);
    EXPECT_EQ(copiedRenderPassId, copyQuad->render_pass_id);
    EXPECT_EQ(isReplica, copyQuad->is_replica);
    EXPECT_EQ(maskResourceId, copyQuad->mask_resource_id);
    EXPECT_RECT_EQ(contentsChangedSinceLastFrame, copyQuad->contents_changed_since_last_frame);
    EXPECT_EQ(maskUVRect.ToString(), copyQuad->mask_uv_rect.ToString());
}

TEST(DrawQuadTest, copySolidColorDrawQuad)
{
    SkColor color = 0x49494949;
    CREATE_SHARED_STATE();

    CREATE_QUAD_1_NEW(SolidColorDrawQuad, color);
    EXPECT_EQ(DrawQuad::SOLID_COLOR, copyQuad->material);
    EXPECT_EQ(color, copyQuad->color);

    CREATE_QUAD_1_ALL(SolidColorDrawQuad, color);
    EXPECT_EQ(DrawQuad::SOLID_COLOR, copyQuad->material);
    EXPECT_EQ(color, copyQuad->color);
}

TEST(DrawQuadTest, copyStreamVideoDrawQuad)
{
    gfx::Rect opaqueRect(3, 7, 10, 12);
    unsigned textureId = 64;
    gfx::Transform matrix = MathUtil::createGfxTransform(0.5, 1, 0.25, 0.75, 0, 1);
    CREATE_SHARED_STATE();

    CREATE_QUAD_3_NEW(StreamVideoDrawQuad, opaqueRect, textureId, matrix);
    EXPECT_EQ(DrawQuad::STREAM_VIDEO_CONTENT, copyQuad->material);
    EXPECT_RECT_EQ(opaqueRect, copyQuad->opaque_rect);
    EXPECT_EQ(textureId, copyQuad->texture_id);
    EXPECT_EQ(matrix, copyQuad->matrix);

    CREATE_QUAD_2_ALL(StreamVideoDrawQuad, textureId, matrix);
    EXPECT_EQ(DrawQuad::STREAM_VIDEO_CONTENT, copyQuad->material);
    EXPECT_EQ(textureId, copyQuad->texture_id);
    EXPECT_EQ(matrix, copyQuad->matrix);
}

TEST(DrawQuadTest, copyTextureDrawQuad)
{
    gfx::Rect opaqueRect(3, 7, 10, 12);
    unsigned resourceId = 82;
    bool premultipliedAlpha = true;
    gfx::RectF uvRect(0.5, 224, -51, 36);
    const float vertex_opacity[] = {1.0f, 1.0f, 1.0f, 1.0f};
    bool flipped = true;
    CREATE_SHARED_STATE();

    CREATE_QUAD_6_NEW(TextureDrawQuad, opaqueRect, resourceId, premultipliedAlpha, uvRect, vertex_opacity, flipped);
    EXPECT_EQ(DrawQuad::TEXTURE_CONTENT, copyQuad->material);
    EXPECT_RECT_EQ(opaqueRect, copyQuad->opaque_rect);
    EXPECT_EQ(resourceId, copyQuad->resource_id);
    EXPECT_EQ(premultipliedAlpha, copyQuad->premultiplied_alpha);
    EXPECT_FLOAT_RECT_EQ(uvRect, copyQuad->uv_rect);
    EXPECT_FLOAT_ARRAY_EQ(vertex_opacity, copyQuad->vertex_opacity, 4);
    EXPECT_EQ(flipped, copyQuad->flipped);

    CREATE_QUAD_5_ALL(TextureDrawQuad, resourceId, premultipliedAlpha, uvRect, vertex_opacity, flipped);
    EXPECT_EQ(DrawQuad::TEXTURE_CONTENT, copyQuad->material);
    EXPECT_EQ(resourceId, copyQuad->resource_id);
    EXPECT_EQ(premultipliedAlpha, copyQuad->premultiplied_alpha);
    EXPECT_FLOAT_RECT_EQ(uvRect, copyQuad->uv_rect);
    EXPECT_FLOAT_ARRAY_EQ(vertex_opacity, copyQuad->vertex_opacity, 4);
    EXPECT_EQ(flipped, copyQuad->flipped);
}

TEST(DrawQuadTest, copyTileDrawQuad)
{
    gfx::Rect opaqueRect(33, 44, 22, 33);
    unsigned resourceId = 104;
    gfx::RectF texCoordRect(31, 12, 54, 20);
    gfx::Size textureSize(85, 32);
    bool swizzleContents = true;
    bool leftEdgeAA = true;
    bool topEdgeAA = true;
    bool rightEdgeAA = false;
    bool bottomEdgeAA = true;
    CREATE_SHARED_STATE();

    CREATE_QUAD_9_NEW(TileDrawQuad, opaqueRect, resourceId, texCoordRect, textureSize, swizzleContents, leftEdgeAA, topEdgeAA, rightEdgeAA, bottomEdgeAA);
    EXPECT_EQ(DrawQuad::TILED_CONTENT, copyQuad->material);
    EXPECT_RECT_EQ(opaqueRect, copyQuad->opaque_rect);
    EXPECT_EQ(resourceId, copyQuad->resource_id);
    EXPECT_EQ(texCoordRect, copyQuad->tex_coord_rect);
    EXPECT_EQ(textureSize, copyQuad->texture_size);
    EXPECT_EQ(swizzleContents, copyQuad->swizzle_contents);
    EXPECT_EQ(leftEdgeAA, copyQuad->left_edge_aa);
    EXPECT_EQ(topEdgeAA, copyQuad->top_edge_aa);
    EXPECT_EQ(rightEdgeAA, copyQuad->right_edge_aa);
    EXPECT_EQ(bottomEdgeAA, copyQuad->bottom_edge_aa);

    CREATE_QUAD_8_ALL(TileDrawQuad, resourceId, texCoordRect, textureSize, swizzleContents, leftEdgeAA, topEdgeAA, rightEdgeAA, bottomEdgeAA);
    EXPECT_EQ(DrawQuad::TILED_CONTENT, copyQuad->material);
    EXPECT_EQ(resourceId, copyQuad->resource_id);
    EXPECT_EQ(texCoordRect, copyQuad->tex_coord_rect);
    EXPECT_EQ(textureSize, copyQuad->texture_size);
    EXPECT_EQ(swizzleContents, copyQuad->swizzle_contents);
    EXPECT_EQ(leftEdgeAA, copyQuad->left_edge_aa);
    EXPECT_EQ(topEdgeAA, copyQuad->top_edge_aa);
    EXPECT_EQ(rightEdgeAA, copyQuad->right_edge_aa);
    EXPECT_EQ(bottomEdgeAA, copyQuad->bottom_edge_aa);
}

TEST(DrawQuadTest, copyYUVVideoDrawQuad)
{
    gfx::Rect opaqueRect(3, 7, 10, 12);
    gfx::SizeF texScale(0.75, 0.5);
    VideoLayerImpl::FramePlane yPlane;
    yPlane.resourceId = 45;
    yPlane.size = gfx::Size(34, 23);
    yPlane.format = 8;
    VideoLayerImpl::FramePlane uPlane;
    uPlane.resourceId = 532;
    uPlane.size = gfx::Size(134, 16);
    uPlane.format = 2;
    VideoLayerImpl::FramePlane vPlane;
    vPlane.resourceId = 4;
    vPlane.size = gfx::Size(456, 486);
    vPlane.format = 46;
    CREATE_SHARED_STATE();

    CREATE_QUAD_5_NEW(YUVVideoDrawQuad, opaqueRect, texScale, yPlane, uPlane, vPlane);
    EXPECT_EQ(DrawQuad::YUV_VIDEO_CONTENT, copyQuad->material);
    EXPECT_RECT_EQ(opaqueRect, copyQuad->opaque_rect);
    EXPECT_EQ(texScale, copyQuad->tex_scale);
    EXPECT_EQ(yPlane.resourceId, copyQuad->y_plane.resourceId);
    EXPECT_EQ(yPlane.size, copyQuad->y_plane.size);
    EXPECT_EQ(yPlane.format, copyQuad->y_plane.format);
    EXPECT_EQ(uPlane.resourceId, copyQuad->u_plane.resourceId);
    EXPECT_EQ(uPlane.size, copyQuad->u_plane.size);
    EXPECT_EQ(uPlane.format, copyQuad->u_plane.format);
    EXPECT_EQ(vPlane.resourceId, copyQuad->v_plane.resourceId);
    EXPECT_EQ(vPlane.size, copyQuad->v_plane.size);
    EXPECT_EQ(vPlane.format, copyQuad->v_plane.format);

    CREATE_QUAD_4_ALL(YUVVideoDrawQuad, texScale, yPlane, uPlane, vPlane);
    EXPECT_EQ(DrawQuad::YUV_VIDEO_CONTENT, copyQuad->material);
    EXPECT_EQ(texScale, copyQuad->tex_scale);
    EXPECT_EQ(yPlane.resourceId, copyQuad->y_plane.resourceId);
    EXPECT_EQ(yPlane.size, copyQuad->y_plane.size);
    EXPECT_EQ(yPlane.format, copyQuad->y_plane.format);
    EXPECT_EQ(uPlane.resourceId, copyQuad->u_plane.resourceId);
    EXPECT_EQ(uPlane.size, copyQuad->u_plane.size);
    EXPECT_EQ(uPlane.format, copyQuad->u_plane.format);
    EXPECT_EQ(vPlane.resourceId, copyQuad->v_plane.resourceId);
    EXPECT_EQ(vPlane.size, copyQuad->v_plane.size);
    EXPECT_EQ(vPlane.format, copyQuad->v_plane.format);
}

}  // namespace
}  // namespace cc
