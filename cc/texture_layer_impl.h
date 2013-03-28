// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEXTURE_LAYER_IMPL_H_
#define CC_TEXTURE_LAYER_IMPL_H_

#include "cc/cc_export.h"
#include "cc/layer_impl.h"

namespace cc {

class CC_EXPORT TextureLayerImpl : public LayerImpl {
public:
    static scoped_ptr<TextureLayerImpl> create(LayerTreeImpl* treeImpl, int id)
    {
        return make_scoped_ptr(new TextureLayerImpl(treeImpl, id));
    }
    virtual ~TextureLayerImpl();

    virtual void willDraw(ResourceProvider*) OVERRIDE;
    virtual void appendQuads(QuadSink&, AppendQuadsData&) OVERRIDE;
    virtual void didDraw(ResourceProvider*) OVERRIDE;

    virtual void didLoseOutputSurface() OVERRIDE;

    virtual void dumpLayerProperties(std::string*, int indent) const OVERRIDE;

    unsigned textureId() const { return m_textureId; }
    void setTextureId(unsigned id) { m_textureId = id; }
    void setPremultipliedAlpha(bool premultipliedAlpha) { m_premultipliedAlpha = premultipliedAlpha; }
    void setFlipped(bool flipped) { m_flipped = flipped; }
    void setUVRect(const gfx::RectF& rect) { m_uvRect = rect; }

    // 1--2
    // |  |
    // 0--3
    void setVertexOpacity(const float vertexOpacity[4]);

private:
    TextureLayerImpl(LayerTreeImpl* treeImpl, int id);

    virtual const char* layerTypeAsString() const OVERRIDE;

    unsigned m_textureId;
    ResourceProvider::ResourceId m_externalTextureResource;
    bool m_premultipliedAlpha;
    bool m_flipped;
    gfx::RectF m_uvRect;
    float m_vertexOpacity[4];
};

}

#endif  // CC_TEXTURE_LAYER_IMPL_H_
