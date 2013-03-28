// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/delegated_renderer_layer_impl.h"

#include "cc/append_quads_data.h"
#include "cc/math_util.h"
#include "cc/quad_sink.h"
#include "cc/render_pass_draw_quad.h"
#include "cc/render_pass_sink.h"

namespace cc {

DelegatedRendererLayerImpl::DelegatedRendererLayerImpl(LayerTreeImpl* treeImpl, int id)
    : LayerImpl(treeImpl, id)
{
}

DelegatedRendererLayerImpl::~DelegatedRendererLayerImpl()
{
    clearRenderPasses();
}

bool DelegatedRendererLayerImpl::hasDelegatedContent() const
{
    return !m_renderPassesInDrawOrder.isEmpty();
}

bool DelegatedRendererLayerImpl::hasContributingDelegatedRenderPasses() const
{
    // The root RenderPass for the layer is merged with its target
    // RenderPass in each frame. So we only have extra RenderPasses
    // to merge when we have a non-root RenderPass present.
    return m_renderPassesInDrawOrder.size() > 1;
}

void DelegatedRendererLayerImpl::setRenderPasses(ScopedPtrVector<RenderPass>& renderPassesInDrawOrder)
{
    gfx::RectF oldRootDamage;
    if (!m_renderPassesInDrawOrder.isEmpty())
        oldRootDamage = m_renderPassesInDrawOrder.last()->damage_rect;

    clearRenderPasses();

    for (size_t i = 0; i < renderPassesInDrawOrder.size(); ++i) {
        m_renderPassesIndexById.insert(std::pair<RenderPass::Id, int>(renderPassesInDrawOrder[i]->id, i));
        m_renderPassesInDrawOrder.append(renderPassesInDrawOrder.take(i));
    }
    renderPassesInDrawOrder.clear();

    if (!m_renderPassesInDrawOrder.isEmpty())
        m_renderPassesInDrawOrder.last()->damage_rect.Union(oldRootDamage);
}

void DelegatedRendererLayerImpl::clearRenderPasses()
{
    // FIXME: Release the resources back to the nested compositor.
    m_renderPassesIndexById.clear();
    m_renderPassesInDrawOrder.clear();
}

void DelegatedRendererLayerImpl::didLoseOutputSurface()
{
    clearRenderPasses();
}

static inline int indexToId(int index) { return index + 1; }
static inline int idToIndex(int id) { return id - 1; }

RenderPass::Id DelegatedRendererLayerImpl::firstContributingRenderPassId() const
{
    return RenderPass::Id(id(), indexToId(0));
}

RenderPass::Id DelegatedRendererLayerImpl::nextContributingRenderPassId(RenderPass::Id previous) const
{
    return RenderPass::Id(previous.layer_id, previous.index + 1);
}

RenderPass::Id DelegatedRendererLayerImpl::convertDelegatedRenderPassId(RenderPass::Id delegatedRenderPassId) const
{
    base::hash_map<RenderPass::Id, int>::const_iterator it = m_renderPassesIndexById.find(delegatedRenderPassId);
    DCHECK(it != m_renderPassesIndexById.end());
    unsigned delegatedRenderPassIndex = it->second;
    return RenderPass::Id(id(), indexToId(delegatedRenderPassIndex));
}

void DelegatedRendererLayerImpl::appendContributingRenderPasses(RenderPassSink& renderPassSink)
{
    DCHECK(hasContributingDelegatedRenderPasses());

    for (size_t i = 0; i < m_renderPassesInDrawOrder.size() - 1; ++i) {
        RenderPass::Id outputRenderPassId = convertDelegatedRenderPassId(m_renderPassesInDrawOrder[i]->id);

        // Don't clash with the RenderPass we generate if we own a RenderSurfaceImpl.
        DCHECK(outputRenderPassId.index > 0);

        renderPassSink.appendRenderPass(m_renderPassesInDrawOrder[i]->Copy(outputRenderPassId));
    }
}

void DelegatedRendererLayerImpl::appendQuads(QuadSink& quadSink, AppendQuadsData& appendQuadsData)
{
    if (m_renderPassesInDrawOrder.isEmpty())
        return;

    RenderPass::Id targetRenderPassId = appendQuadsData.renderPassId;

    // If the index of the renderPassId is 0, then it is a renderPass generated for a layer
    // in this compositor, not the delegated renderer. Then we want to merge our root renderPass with
    // the target renderPass. Otherwise, it is some renderPass which we added from the delegated
    // renderer.
    bool shouldMergeRootRenderPassWithTarget = !targetRenderPassId.index;
    if (shouldMergeRootRenderPassWithTarget) {
        // Verify that the renderPass we are appending to is created our renderTarget.
        DCHECK(targetRenderPassId.layer_id == renderTarget()->id());

        const RenderPass* rootDelegatedRenderPass = m_renderPassesInDrawOrder.last();
        appendRenderPassQuads(quadSink, appendQuadsData, rootDelegatedRenderPass);
    } else {
        // Verify that the renderPass we are appending to was created by us.
        DCHECK(targetRenderPassId.layer_id == id());

        int renderPassIndex = idToIndex(targetRenderPassId.index);
        const RenderPass* delegatedRenderPass = m_renderPassesInDrawOrder[renderPassIndex];
        appendRenderPassQuads(quadSink, appendQuadsData, delegatedRenderPass);
    }
}

void DelegatedRendererLayerImpl::appendRenderPassQuads(QuadSink& quadSink, AppendQuadsData& appendQuadsData, const RenderPass* delegatedRenderPass) const
{
    const SharedQuadState* currentSharedQuadState = 0;
    SharedQuadState* copiedSharedQuadState = 0;
    for (size_t i = 0; i < delegatedRenderPass->quad_list.size(); ++i) {
        const DrawQuad* quad = delegatedRenderPass->quad_list[i];

        if (quad->shared_quad_state != currentSharedQuadState) {
            currentSharedQuadState = quad->shared_quad_state;
            copiedSharedQuadState = quadSink.useSharedQuadState(currentSharedQuadState->Copy());
            bool targetIsFromDelegatedRendererLayer = appendQuadsData.renderPassId.layer_id == id();
            if (!targetIsFromDelegatedRendererLayer) {
              // Should be the root render pass.
              DCHECK(delegatedRenderPass == m_renderPassesInDrawOrder.last());
              // This layer must be drawing to a renderTarget other than itself.
              DCHECK(renderTarget() != this);

              copiedSharedQuadState->content_to_target_transform = copiedSharedQuadState->content_to_target_transform * drawTransform();
              copiedSharedQuadState->clipped_rect_in_target = MathUtil::mapClippedRect(drawTransform(), copiedSharedQuadState->clipped_rect_in_target);
              copiedSharedQuadState->opacity *= drawOpacity();
            }
        }
        DCHECK(copiedSharedQuadState);

        scoped_ptr<DrawQuad> copyQuad;
        if (quad->material != DrawQuad::RENDER_PASS)
            copyQuad = quad->Copy(copiedSharedQuadState);
        else {
            RenderPass::Id contributingDelegatedRenderPassId = RenderPassDrawQuad::MaterialCast(quad)->render_pass_id;
            RenderPass::Id contributingRenderPassId = convertDelegatedRenderPassId(contributingDelegatedRenderPassId);
            DCHECK(contributingRenderPassId != appendQuadsData.renderPassId);

            copyQuad = RenderPassDrawQuad::MaterialCast(quad)->Copy(copiedSharedQuadState, contributingRenderPassId).PassAs<DrawQuad>();
        }
        DCHECK(copyQuad.get());

        quadSink.append(copyQuad.Pass(), appendQuadsData);
    }
}

const char* DelegatedRendererLayerImpl::layerTypeAsString() const
{
    return "DelegatedRendererLayer";
}

}  // namespace cc
