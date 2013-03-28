// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layer.h"

#include "cc/active_animation.h"
#include "cc/animation_events.h"
#include "cc/layer_animation_controller.h"
#include "cc/layer_impl.h"
#include "cc/layer_tree_host.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "ui/gfx/rect_conversions.h"
#include <public/WebAnimationDelegate.h>
#include <public/WebLayerScrollClient.h>
#include <public/WebSize.h>

using namespace std;

namespace cc {

static int s_nextLayerId = 1;

scoped_refptr<Layer> Layer::create()
{
    return make_scoped_refptr(new Layer());
}

Layer::Layer()
    : m_needsDisplay(false)
    , m_stackingOrderChanged(false)
    , m_layerId(s_nextLayerId++)
    , m_ignoreSetNeedsCommit(false)
    , m_parent(0)
    , m_layerTreeHost(0)
    , m_layerAnimationController(LayerAnimationController::create(this))
    , m_scrollable(false)
    , m_shouldScrollOnMainThread(false)
    , m_haveWheelEventHandlers(false)
    , m_nonFastScrollableRegionChanged(false)
    , m_touchEventHandlerRegionChanged(false)
    , m_anchorPoint(0.5, 0.5)
    , m_backgroundColor(0)
    , m_opacity(1.0)
    , m_anchorPointZ(0)
    , m_isContainerForFixedPositionLayers(false)
    , m_fixedToContainerLayer(false)
    , m_isDrawable(false)
    , m_masksToBounds(false)
    , m_contentsOpaque(false)
    , m_doubleSided(true)
    , m_preserves3D(false)
    , m_useParentBackfaceVisibility(false)
    , m_drawCheckerboardForMissingTiles(false)
    , m_forceRenderSurface(false)
    , m_replicaLayer(0)
    , m_rasterScale(1.0)
    , m_automaticallyComputeRasterScale(false)
    , m_boundsContainPageScale(false)
    , m_layerAnimationDelegate(0)
    , m_layerScrollClient(0)
{
    if (m_layerId < 0) {
        s_nextLayerId = 1;
        m_layerId = s_nextLayerId++;
    }

    addLayerAnimationObserver(m_layerAnimationController.get());
}

Layer::~Layer()
{
    // Our parent should be holding a reference to us so there should be no
    // way for us to be destroyed while we still have a parent.
    DCHECK(!parent());

    // Remove the parent reference from all children.
    removeAllChildren();
}

void Layer::setLayerTreeHost(LayerTreeHost* host)
{
    if (m_layerTreeHost == host)
        return;

    m_layerTreeHost = host;

    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->setLayerTreeHost(host);

    if (m_maskLayer)
        m_maskLayer->setLayerTreeHost(host);
    if (m_replicaLayer)
        m_replicaLayer->setLayerTreeHost(host);

    // If this layer already has active animations, the host needs to be notified.
    if (host && m_layerAnimationController->hasActiveAnimation())
        host->didAddAnimation();
}

void Layer::setNeedsCommit()
{
    if (m_ignoreSetNeedsCommit)
        return;
    if (m_layerTreeHost)
        m_layerTreeHost->setNeedsCommit();
}

void Layer::setNeedsFullTreeSync()
{
    if (m_layerTreeHost)
        m_layerTreeHost->setNeedsFullTreeSync();
}

gfx::Rect Layer::layerRectToContentRect(const gfx::RectF& layerRect) const
{
    gfx::RectF contentRect = gfx::ScaleRect(layerRect, contentsScaleX(), contentsScaleY());
    // Intersect with content rect to avoid the extra pixel because for some
    // values x and y, ceil((x / y) * y) may be x + 1.
    contentRect.Intersect(gfx::Rect(gfx::Point(), contentBounds()));
    return gfx::ToEnclosingRect(contentRect);
}

bool Layer::blocksPendingCommit() const
{
    return false;
}

void Layer::setParent(Layer* layer)
{
    DCHECK(!layer || !layer->hasAncestor(this));
    m_parent = layer;
    setLayerTreeHost(m_parent ? m_parent->layerTreeHost() : 0);

    forceAutomaticRasterScaleToBeRecomputed();
}

bool Layer::hasAncestor(Layer* ancestor) const
{
    for (const Layer* layer = parent(); layer; layer = layer->parent()) {
        if (layer == ancestor)
            return true;
    }
    return false;
}

void Layer::addChild(scoped_refptr<Layer> child)
{
    insertChild(child, numChildren());
}

void Layer::insertChild(scoped_refptr<Layer> child, size_t index)
{
    child->removeFromParent();
    child->setParent(this);
    child->m_stackingOrderChanged = true;

    index = min(index, m_children.size());
    LayerList::iterator iter = m_children.begin();
    m_children.insert(iter + index, child);
    setNeedsFullTreeSync();
}

void Layer::removeFromParent()
{
    if (m_parent)
        m_parent->removeChild(this);
}

void Layer::removeChild(Layer* child)
{
    for (LayerList::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
    {
        if (*iter != child)
            continue;

        child->setParent(0);
        m_children.erase(iter);
        setNeedsFullTreeSync();
        return;
    }
}

void Layer::replaceChild(Layer* reference, scoped_refptr<Layer> newLayer)
{
    DCHECK(reference);
    DCHECK_EQ(reference->parent(), this);

    if (reference == newLayer)
        return;

    int referenceIndex = indexOfChild(reference);
    if (referenceIndex == -1) {
        NOTREACHED();
        return;
    }

    reference->removeFromParent();

    if (newLayer) {
        newLayer->removeFromParent();
        insertChild(newLayer, referenceIndex);
    }
}

int Layer::indexOfChild(const Layer* reference)
{
    for (size_t i = 0; i < m_children.size(); i++) {
        if (m_children[i] == reference)
            return i;
    }
    return -1;
}

void Layer::setBounds(const gfx::Size& size)
{
    if (bounds() == size)
        return;

    bool firstResize = bounds().IsEmpty() && !size.IsEmpty();

    m_bounds = size;

    didUpdateBounds();

    if (firstResize)
        setNeedsDisplay();
    else
        setNeedsCommit();
}

void Layer::didUpdateBounds()
{
    m_drawProperties.content_bounds = bounds();
}

Layer* Layer::rootLayer()
{
    Layer* layer = this;
    while (layer->parent())
        layer = layer->parent();
    return layer;
}

void Layer::removeAllChildren()
{
    while (m_children.size()) {
        Layer* layer = m_children[0].get();
        DCHECK(layer->parent());
        layer->removeFromParent();
    }
}

void Layer::setChildren(const LayerList& children)
{
    if (children == m_children)
        return;

    removeAllChildren();
    size_t listSize = children.size();
    for (size_t i = 0; i < listSize; i++)
        addChild(children[i]);
}

void Layer::setAnchorPoint(const gfx::PointF& anchorPoint)
{
    if (m_anchorPoint == anchorPoint)
        return;
    m_anchorPoint = anchorPoint;
    setNeedsCommit();
}

void Layer::setAnchorPointZ(float anchorPointZ)
{
    if (m_anchorPointZ == anchorPointZ)
        return;
    m_anchorPointZ = anchorPointZ;
    setNeedsCommit();
}

void Layer::setBackgroundColor(SkColor backgroundColor)
{
    if (m_backgroundColor == backgroundColor)
        return;
    m_backgroundColor = backgroundColor;
    setNeedsCommit();
}

void Layer::calculateContentsScale(
    float idealContentsScale,
    float* contentsScaleX,
    float* contentsScaleY,
    gfx::Size* contentBounds)
{
    *contentsScaleX = 1;
    *contentsScaleY = 1;
    *contentBounds = bounds();
}

void Layer::setMasksToBounds(bool masksToBounds)
{
    if (m_masksToBounds == masksToBounds)
        return;
    m_masksToBounds = masksToBounds;
    setNeedsCommit();
}

void Layer::setMaskLayer(Layer* maskLayer)
{
    if (m_maskLayer == maskLayer)
        return;
    if (m_maskLayer)
        m_maskLayer->setLayerTreeHost(0);
    m_maskLayer = maskLayer;
    if (m_maskLayer) {
        m_maskLayer->setLayerTreeHost(m_layerTreeHost);
        m_maskLayer->setIsMask(true);
    }
    setNeedsFullTreeSync();
}

void Layer::setReplicaLayer(Layer* layer)
{
    if (m_replicaLayer == layer)
        return;
    if (m_replicaLayer)
        m_replicaLayer->setLayerTreeHost(0);
    m_replicaLayer = layer;
    if (m_replicaLayer)
        m_replicaLayer->setLayerTreeHost(m_layerTreeHost);
    setNeedsFullTreeSync();
}

void Layer::setFilters(const WebKit::WebFilterOperations& filters)
{
    if (m_filters == filters)
        return;
    DCHECK(!m_filter);
    m_filters = filters;
    setNeedsCommit();
    if (!filters.isEmpty())
        LayerTreeHost::setNeedsFilterContext(true);
}

void Layer::setFilter(const skia::RefPtr<SkImageFilter>& filter)
{
    if (m_filter.get() == filter.get())
        return;
    DCHECK(m_filters.isEmpty());
    m_filter = filter;
    setNeedsCommit();
    if (filter)
        LayerTreeHost::setNeedsFilterContext(true);
}

void Layer::setBackgroundFilters(const WebKit::WebFilterOperations& backgroundFilters)
{
    if (m_backgroundFilters == backgroundFilters)
        return;
    m_backgroundFilters = backgroundFilters;
    setNeedsCommit();
    if (!backgroundFilters.isEmpty())
        LayerTreeHost::setNeedsFilterContext(true);
}

bool Layer::needsDisplay() const
{
    return m_needsDisplay;
}

void Layer::setOpacity(float opacity)
{
    if (m_opacity == opacity)
        return;
    m_opacity = opacity;
    setNeedsCommit();
}

bool Layer::opacityIsAnimating() const
{
    return m_layerAnimationController->isAnimatingProperty(ActiveAnimation::Opacity);
}

void Layer::setContentsOpaque(bool opaque)
{
    if (m_contentsOpaque == opaque)
        return;
    m_contentsOpaque = opaque;
    setNeedsDisplay();
}

void Layer::setPosition(const gfx::PointF& position)
{
    if (m_position == position)
        return;
    m_position = position;
    setNeedsCommit();
}

void Layer::setSublayerTransform(const gfx::Transform& sublayerTransform)
{
    if (m_sublayerTransform == sublayerTransform)
        return;
    m_sublayerTransform = sublayerTransform;
    setNeedsCommit();
}

void Layer::setTransform(const gfx::Transform& transform)
{
    if (m_transform == transform)
        return;
    m_transform = transform;
    setNeedsCommit();
}

bool Layer::transformIsAnimating() const
{
    return m_layerAnimationController->isAnimatingProperty(ActiveAnimation::Transform);
}

void Layer::setScrollOffset(gfx::Vector2d scrollOffset)
{
    if (m_scrollOffset == scrollOffset)
        return;
    m_scrollOffset = scrollOffset;
    if (m_layerScrollClient)
        m_layerScrollClient->didScroll();
    setNeedsFullTreeSync();
}

void Layer::setMaxScrollOffset(gfx::Vector2d maxScrollOffset)
{
    if (m_maxScrollOffset == maxScrollOffset)
        return;
    m_maxScrollOffset = maxScrollOffset;
    setNeedsCommit();
}

void Layer::setScrollable(bool scrollable)
{
    if (m_scrollable == scrollable)
        return;
    m_scrollable = scrollable;
    setNeedsCommit();
}

void Layer::setShouldScrollOnMainThread(bool shouldScrollOnMainThread)
{
    if (m_shouldScrollOnMainThread == shouldScrollOnMainThread)
        return;
    m_shouldScrollOnMainThread = shouldScrollOnMainThread;
    setNeedsCommit();
}

void Layer::setHaveWheelEventHandlers(bool haveWheelEventHandlers)
{
    if (m_haveWheelEventHandlers == haveWheelEventHandlers)
        return;
    m_haveWheelEventHandlers = haveWheelEventHandlers;
    setNeedsCommit();
}

void Layer::setNonFastScrollableRegion(const Region& region)
{
    if (m_nonFastScrollableRegion == region)
        return;
    m_nonFastScrollableRegion = region;
    m_nonFastScrollableRegionChanged = true;
    setNeedsCommit();
}

void Layer::setTouchEventHandlerRegion(const Region& region)
{
    if (m_touchEventHandlerRegion == region)
        return;
    m_touchEventHandlerRegion = region;
    m_touchEventHandlerRegionChanged = true;
}

void Layer::setDrawCheckerboardForMissingTiles(bool checkerboard)
{
    if (m_drawCheckerboardForMissingTiles == checkerboard)
        return;
    m_drawCheckerboardForMissingTiles = checkerboard;
    setNeedsCommit();
}

void Layer::setForceRenderSurface(bool force)
{
    if (m_forceRenderSurface == force)
        return;
    m_forceRenderSurface = force;
    setNeedsCommit();
}

void Layer::setImplTransform(const gfx::Transform& transform)
{
    if (m_implTransform == transform)
        return;
    m_implTransform = transform;
    setNeedsCommit();
}

void Layer::setDoubleSided(bool doubleSided)
{
    if (m_doubleSided == doubleSided)
        return;
    m_doubleSided = doubleSided;
    setNeedsCommit();
}

void Layer::setIsDrawable(bool isDrawable)
{
    if (m_isDrawable == isDrawable)
        return;

    m_isDrawable = isDrawable;
    setNeedsCommit();
}

void Layer::setNeedsDisplayRect(const gfx::RectF& dirtyRect)
{
    m_updateRect.Union(dirtyRect);

    // Simply mark the contents as dirty. For non-root layers, the call to
    // setNeedsCommit will schedule a fresh compositing pass.
    // For the root layer, setNeedsCommit has no effect.
    if (!dirtyRect.IsEmpty())
        m_needsDisplay = true;

    if (drawsContent())
        setNeedsCommit();
}

bool Layer::descendantIsFixedToContainerLayer() const
{
    for (size_t i = 0; i < m_children.size(); ++i) {
        if (m_children[i]->fixedToContainerLayer() || m_children[i]->descendantIsFixedToContainerLayer())
            return true;
    }
    return false;
}

void Layer::setIsContainerForFixedPositionLayers(bool isContainerForFixedPositionLayers)
{
    if (m_isContainerForFixedPositionLayers == isContainerForFixedPositionLayers)
        return;
    m_isContainerForFixedPositionLayers = isContainerForFixedPositionLayers;

    if (m_layerTreeHost && m_layerTreeHost->commitRequested())
        return;

    // Only request a commit if we have a fixed positioned descendant.
    if (descendantIsFixedToContainerLayer())
        setNeedsCommit();
}

void Layer::setFixedToContainerLayer(bool fixedToContainerLayer)
{
    if (m_fixedToContainerLayer == fixedToContainerLayer)
        return;
    m_fixedToContainerLayer = fixedToContainerLayer;
    setNeedsCommit();
}

void Layer::pushPropertiesTo(LayerImpl* layer)
{
    layer->setAnchorPoint(m_anchorPoint);
    layer->setAnchorPointZ(m_anchorPointZ);
    layer->setBackgroundColor(m_backgroundColor);
    layer->setBounds(m_bounds);
    layer->setContentBounds(contentBounds());
    layer->setContentsScale(contentsScaleX(), contentsScaleY());
    layer->setDebugName(m_debugName);
    layer->setDoubleSided(m_doubleSided);
    layer->setDrawCheckerboardForMissingTiles(m_drawCheckerboardForMissingTiles);
    layer->setForceRenderSurface(m_forceRenderSurface);
    layer->setDrawsContent(drawsContent());
    layer->setFilters(filters());
    layer->setFilter(filter());
    layer->setBackgroundFilters(backgroundFilters());
    layer->setMasksToBounds(m_masksToBounds);
    layer->setScrollable(m_scrollable);
    layer->setShouldScrollOnMainThread(m_shouldScrollOnMainThread);
    layer->setHaveWheelEventHandlers(m_haveWheelEventHandlers);
    // Copying a Region is more expensive than most layer properties, since it involves copying two Vectors that may be
    // arbitrarily large depending on page content, so we only push the property if it's changed.
    if (m_nonFastScrollableRegionChanged) {
        layer->setNonFastScrollableRegion(m_nonFastScrollableRegion);
        m_nonFastScrollableRegionChanged = false;
    }
    if (m_touchEventHandlerRegionChanged) {
        layer->setTouchEventHandlerRegion(m_touchEventHandlerRegion);
        m_touchEventHandlerRegionChanged = false;
    }
    layer->setContentsOpaque(m_contentsOpaque);
    if (!opacityIsAnimating())
        layer->setOpacity(m_opacity);
    layer->setPosition(m_position);
    layer->setIsContainerForFixedPositionLayers(m_isContainerForFixedPositionLayers);
    layer->setFixedToContainerLayer(m_fixedToContainerLayer);
    layer->setPreserves3D(preserves3D());
    layer->setUseParentBackfaceVisibility(m_useParentBackfaceVisibility);
    layer->setScrollOffset(m_scrollOffset);
    layer->setMaxScrollOffset(m_maxScrollOffset);
    layer->setSublayerTransform(m_sublayerTransform);
    if (!transformIsAnimating())
        layer->setTransform(m_transform);

    // If the main thread commits multiple times before the impl thread actually draws, then damage tracking
    // will become incorrect if we simply clobber the updateRect here. The LayerImpl's updateRect needs to
    // accumulate (i.e. union) any update changes that have occurred on the main thread.
    m_updateRect.Union(layer->updateRect());
    layer->setUpdateRect(m_updateRect);

    layer->setScrollDelta(layer->scrollDelta() - layer->sentScrollDelta());
    layer->setSentScrollDelta(gfx::Vector2d());

    layer->setStackingOrderChanged(m_stackingOrderChanged);

    if (maskLayer())
        maskLayer()->pushPropertiesTo(layer->maskLayer());
    if (replicaLayer())
        replicaLayer()->pushPropertiesTo(layer->replicaLayer());

    m_layerAnimationController->pushAnimationUpdatesTo(layer->layerAnimationController());

    // Reset any state that should be cleared for the next update.
    m_stackingOrderChanged = false;
    m_updateRect = gfx::RectF();
}

scoped_ptr<LayerImpl> Layer::createLayerImpl(LayerTreeImpl* treeImpl)
{
    return LayerImpl::create(treeImpl, m_layerId);
}

bool Layer::drawsContent() const
{
    return m_isDrawable;
}

bool Layer::needMoreUpdates()
{
    return false;
}

void Layer::setDebugName(const std::string& debugName)
{
    m_debugName = debugName;
    setNeedsCommit();
}

void Layer::setRasterScale(float scale)
{
    if (m_rasterScale == scale)
        return;
    m_rasterScale = scale;

    // When automatically computed, this acts like a draw property.
    if (m_automaticallyComputeRasterScale)
        return;
    setNeedsDisplay();
}

void Layer::setAutomaticallyComputeRasterScale(bool automatic)
{
    if (m_automaticallyComputeRasterScale == automatic)
        return;
    m_automaticallyComputeRasterScale = automatic;

    if (m_automaticallyComputeRasterScale)
        forceAutomaticRasterScaleToBeRecomputed();
    else
        setRasterScale(1);
}

void Layer::forceAutomaticRasterScaleToBeRecomputed()
{
    if (!m_automaticallyComputeRasterScale)
        return;
    m_rasterScale = 0;
    setNeedsDisplay();
}

void Layer::setBoundsContainPageScale(bool boundsContainPageScale)
{
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->setBoundsContainPageScale(boundsContainPageScale);

    if (boundsContainPageScale == m_boundsContainPageScale)
        return;

    m_boundsContainPageScale = boundsContainPageScale;
    setNeedsDisplay();
}

void Layer::createRenderSurface()
{
    DCHECK(!m_drawProperties.render_surface);
    m_drawProperties.render_surface = make_scoped_ptr(new RenderSurface(this));
    m_drawProperties.render_target = this;
}

int Layer::id() const
{
    return m_layerId;
}

float Layer::opacity() const
{
    return m_opacity;
}

void Layer::setOpacityFromAnimation(float opacity)
{
    // This is called due to an ongoing accelerated animation. Since this animation is
    // also being run on the impl thread, there is no need to request a commit to push
    // this value over, so set the value directly rather than calling setOpacity.
    m_opacity = opacity;
}

const gfx::Transform& Layer::transform() const
{
    return m_transform;
}

void Layer::setTransformFromAnimation(const gfx::Transform& transform)
{
    // This is called due to an ongoing accelerated animation. Since this animation is
    // also being run on the impl thread, there is no need to request a commit to push
    // this value over, so set this value directly rather than calling setTransform.
    m_transform = transform;
}

bool Layer::addAnimation(scoped_ptr <ActiveAnimation> animation)
{
    // WebCore currently assumes that accelerated animations will start soon
    // after the animation is added. However we cannot guarantee that if we do
    // not have a layerTreeHost that will setNeedsCommit().
    // Unfortunately, the fix below to guarantee correctness causes performance
    // regressions on Android, since Android has shipped for a long time
    // with all animations accelerated. For this reason, we will live with
    // this bug only on Android until the bug is fixed.
    // http://crbug.com/129683
#if !defined(OS_ANDROID)
    if (!m_layerTreeHost)
        return false;

    if (!m_layerTreeHost->settings().acceleratedAnimationEnabled)
        return false;
#endif

    m_layerAnimationController->addAnimation(animation.Pass());
    if (m_layerTreeHost) {
        m_layerTreeHost->didAddAnimation();
        setNeedsCommit();
    }
    return true;
}

void Layer::pauseAnimation(int animationId, double timeOffset)
{
    m_layerAnimationController->pauseAnimation(animationId, timeOffset);
    setNeedsCommit();
}

void Layer::removeAnimation(int animationId)
{
    m_layerAnimationController->removeAnimation(animationId);
    setNeedsCommit();
}

void Layer::suspendAnimations(double monotonicTime)
{
    m_layerAnimationController->suspendAnimations(monotonicTime);
    setNeedsCommit();
}

void Layer::resumeAnimations(double monotonicTime)
{
    m_layerAnimationController->resumeAnimations(monotonicTime);
    setNeedsCommit();
}

void Layer::setLayerAnimationController(scoped_ptr<LayerAnimationController> layerAnimationController)
{
    if (m_layerAnimationController)
        removeLayerAnimationObserver(m_layerAnimationController.get());

    m_layerAnimationController = layerAnimationController.Pass();
    if (m_layerAnimationController) {
        m_layerAnimationController->setClient(this);
        m_layerAnimationController->setForceSync();
        addLayerAnimationObserver(m_layerAnimationController.get());
    }
    setNeedsCommit();
}

scoped_ptr<LayerAnimationController> Layer::releaseLayerAnimationController()
{
    scoped_ptr<LayerAnimationController> toReturn = m_layerAnimationController.Pass();
    m_layerAnimationController = LayerAnimationController::create(this);
    return toReturn.Pass();
}

bool Layer::hasActiveAnimation() const
{
    return m_layerAnimationController->hasActiveAnimation();
}

void Layer::notifyAnimationStarted(const AnimationEvent& event, double wallClockTime)
{
    FOR_EACH_OBSERVER(LayerAnimationObserver, m_layerAnimationObservers,
                      OnAnimationStarted(event));
    if (m_layerAnimationDelegate)
        m_layerAnimationDelegate->notifyAnimationStarted(wallClockTime);
}

void Layer::notifyAnimationFinished(double wallClockTime)
{
    if (m_layerAnimationDelegate)
        m_layerAnimationDelegate->notifyAnimationFinished(wallClockTime);
}

void Layer::addLayerAnimationObserver(LayerAnimationObserver* animationObserver)
{
    if (!m_layerAnimationObservers.HasObserver(animationObserver))
        m_layerAnimationObservers.AddObserver(animationObserver);
}

void Layer::removeLayerAnimationObserver(LayerAnimationObserver* animationObserver)
{
    m_layerAnimationObservers.RemoveObserver(animationObserver);
}

Region Layer::visibleContentOpaqueRegion() const
{
    if (contentsOpaque())
        return visibleContentRect();
    return Region();
}

ScrollbarLayer* Layer::toScrollbarLayer()
{
    return 0;
}

void sortLayers(std::vector<scoped_refptr<Layer> >::iterator, std::vector<scoped_refptr<Layer> >::iterator, void*)
{
    // Currently we don't use z-order to decide what to paint, so there's no need to actually sort Layers.
}

}  // namespace cc
