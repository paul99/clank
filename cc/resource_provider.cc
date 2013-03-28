// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resource_provider.h"

#include <limits.h>

#include "base/debug/alias.h"
#include "base/hash_tables.h"
#include "base/stl_util.h"
#include "base/string_split.h"
#include "base/string_util.h"
#include "cc/gl_renderer.h" // For the GLC() macro.
#include "cc/platform_color.h"
#include "cc/texture_uploader.h"
#include "cc/transferable_resource.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/vector2d.h"

#include <public/WebGraphicsContext3D.h>

using WebKit::WebGraphicsContext3D;

namespace cc {

static GLenum textureToStorageFormat(GLenum textureFormat)
{
    GLenum storageFormat = GL_RGBA8_OES;
    switch (textureFormat) {
    case GL_RGBA:
        break;
    case GL_BGRA_EXT:
        storageFormat = GL_BGRA8_EXT;
        break;
    default:
        NOTREACHED();
        break;
    }

    return storageFormat;
}

static bool isTextureFormatSupportedForStorage(GLenum format)
{
    return (format == GL_RGBA || format == GL_BGRA_EXT);
}

ResourceProvider::Resource::Resource()
    : glId(0)
    , glPixelBufferId(0)
    , glUploadQueryId(0)
    , pixels(0)
    , pixelBuffer(0)
    , lockForReadCount(0)
    , lockedForWrite(false)
    , external(false)
    , exported(false)
    , markedForDeletion(false)
    , pendingSetPixels(false)
    , size()
    , format(0)
    , filter(0)
    , type(static_cast<ResourceType>(0))
{
}

ResourceProvider::Resource::Resource(unsigned textureId, const gfx::Size& size, GLenum format, GLenum filter)
    : glId(textureId)
    , glPixelBufferId(0)
    , glUploadQueryId(0)
    , pixels(0)
    , pixelBuffer(0)
    , lockForReadCount(0)
    , lockedForWrite(false)
    , external(false)
    , exported(false)
    , markedForDeletion(false)
    , pendingSetPixels(false)
    , size(size)
    , format(format)
    , filter(filter)
    , type(GLTexture)
{
}

ResourceProvider::Resource::Resource(uint8_t* pixels, const gfx::Size& size, GLenum format, GLenum filter)
    : glId(0)
    , glPixelBufferId(0)
    , glUploadQueryId(0)
    , pixels(pixels)
    , pixelBuffer(0)
    , lockForReadCount(0)
    , lockedForWrite(false)
    , external(false)
    , exported(false)
    , markedForDeletion(false)
    , pendingSetPixels(false)
    , size(size)
    , format(format)
    , filter(filter)
    , type(Bitmap)
{
}

ResourceProvider::Child::Child()
{
}

ResourceProvider::Child::~Child()
{
}

scoped_ptr<ResourceProvider> ResourceProvider::create(OutputSurface* context)
{
    scoped_ptr<ResourceProvider> resourceProvider(new ResourceProvider(context));
    if (!resourceProvider->initialize())
        return scoped_ptr<ResourceProvider>();
    return resourceProvider.Pass();
}

ResourceProvider::~ResourceProvider()
{
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    if (!context3d || !context3d->makeContextCurrent())
        return;
    m_textureUploader.reset();
    m_textureCopier.reset();
}

WebGraphicsContext3D* ResourceProvider::graphicsContext3D()
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    return m_outputSurface->Context3D();
}

bool ResourceProvider::inUseByConsumer(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    return !!resource->lockForReadCount || resource->exported;
}

ResourceProvider::ResourceId ResourceProvider::createResource(const gfx::Size& size, GLenum format, TextureUsageHint hint)
{
    switch (m_defaultResourceType) {
    case GLTexture:
        return createGLTexture(size, format, GL_TEXTURE_POOL_UNMANAGED_CHROMIUM, hint);
    case Bitmap:
        DCHECK(format == GL_RGBA);
        return createBitmap(size);
    }

    LOG(FATAL) << "Invalid default resource type.";
    return 0;
}

ResourceProvider::ResourceId ResourceProvider::createManagedResource(const gfx::Size& size, GLenum format, TextureUsageHint hint)
{
    switch (m_defaultResourceType) {
    case GLTexture:
        return createGLTexture(size, format, GL_TEXTURE_POOL_MANAGED_CHROMIUM, hint);
    case Bitmap:
        DCHECK(format == GL_RGBA);
        return createBitmap(size);
    }

    LOG(FATAL) << "Invalid default resource type.";
    return 0;
}

ResourceProvider::ResourceId ResourceProvider::createGLTexture(const gfx::Size& size, GLenum format, GLenum texturePool, TextureUsageHint hint)
{
    DCHECK_LE(size.width(), m_maxTextureSize);
    DCHECK_LE(size.height(), m_maxTextureSize);

    DCHECK(m_threadChecker.CalledOnValidThread());
    unsigned textureId = 0;
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    DCHECK(context3d);
    GLC(context3d, textureId = context3d->createTexture());
    GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, textureId));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_POOL_CHROMIUM, texturePool));

    if (m_useTextureUsageHint && hint == TextureUsageFramebuffer)
        GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_USAGE_ANGLE, GL_FRAMEBUFFER_ATTACHMENT_ANGLE));
    if (m_useTextureStorageExt && isTextureFormatSupportedForStorage(format)) {
        GLenum storageFormat = textureToStorageFormat(format);
        GLC(context3d, context3d->texStorage2DEXT(GL_TEXTURE_2D, 1, storageFormat, size.width(), size.height()));
    } else
        GLC(context3d, context3d->texImage2D(GL_TEXTURE_2D, 0, format, size.width(), size.height(), 0, format, GL_UNSIGNED_BYTE, 0));

    ResourceId id = m_nextId++;
    Resource resource(textureId, size, format, GL_LINEAR);
    m_resources[id] = resource;
    return id;
}

ResourceProvider::ResourceId ResourceProvider::createBitmap(const gfx::Size& size)
{
    DCHECK(m_threadChecker.CalledOnValidThread());

    uint8_t* pixels = new uint8_t[size.width() * size.height() * 4];

    ResourceId id = m_nextId++;
    Resource resource(pixels, size, GL_RGBA, GL_LINEAR);
    m_resources[id] = resource;
    return id;
}

ResourceProvider::ResourceId ResourceProvider::createResourceFromExternalTexture(unsigned textureId)
{
    DCHECK(m_threadChecker.CalledOnValidThread());

    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    DCHECK(context3d);
    GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, textureId));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    ResourceId id = m_nextId++;
    Resource resource(textureId, gfx::Size(), 0, GL_LINEAR);
    resource.external = true;
    m_resources[id] = resource;
    return id;
}

void ResourceProvider::deleteResource(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->lockedForWrite);
    DCHECK(!resource->lockForReadCount);
    DCHECK(!resource->markedForDeletion);

    if (resource->exported) {
        resource->markedForDeletion = true;
        return;
    } else
        deleteResourceInternal(it);
}

void ResourceProvider::deleteResourceInternal(ResourceMap::iterator it)
{
    Resource* resource = &it->second;
    if (resource->glId && !resource->external) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        GLC(context3d, context3d->deleteTexture(resource->glId));
    }
    if (resource->glUploadQueryId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        GLC(context3d, context3d->deleteQueryEXT(resource->glUploadQueryId));
    }
    if (resource->glPixelBufferId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        GLC(context3d, context3d->deleteBuffer(resource->glPixelBufferId));
    }
    if (resource->pixels)
        delete[] resource->pixels;
    if (resource->pixelBuffer)
        delete[] resource->pixelBuffer;

    m_resources.erase(it);
}

ResourceProvider::ResourceType ResourceProvider::resourceType(ResourceId id)
{
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    return resource->type;
}

void ResourceProvider::setPixels(ResourceId id, const uint8_t* image, const gfx::Rect& imageRect, const gfx::Rect& sourceRect, const gfx::Vector2d& destOffset)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->lockedForWrite);
    DCHECK(!resource->lockForReadCount);
    DCHECK(!resource->external);
    DCHECK(!resource->exported);

    if (resource->glId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        DCHECK(m_textureUploader.get());
        context3d->bindTexture(GL_TEXTURE_2D, resource->glId);
        m_textureUploader->upload(image,
                                  imageRect,
                                  sourceRect,
                                  destOffset,
                                  resource->format,
                                  resource->size);
    }

    if (resource->pixels) {
        DCHECK(resource->format == GL_RGBA);
        SkBitmap srcFull;
        srcFull.setConfig(SkBitmap::kARGB_8888_Config, imageRect.width(), imageRect.height());
        srcFull.setPixels(const_cast<uint8_t*>(image));
        SkBitmap srcSubset;
        SkIRect skSourceRect = SkIRect::MakeXYWH(sourceRect.x(), sourceRect.y(), sourceRect.width(), sourceRect.height());
        skSourceRect.offset(-imageRect.x(), -imageRect.y());
        srcFull.extractSubset(&srcSubset, skSourceRect);

        ScopedWriteLockSoftware lock(this, id);
        SkCanvas* dest = lock.skCanvas();
        dest->writePixels(srcSubset, destOffset.x(), destOffset.y());
    }
}

size_t ResourceProvider::numBlockingUploads()
{
    if (!m_textureUploader)
        return 0;

    return m_textureUploader->numBlockingUploads();
}

void ResourceProvider::markPendingUploadsAsNonBlocking()
{
    if (!m_textureUploader)
        return;

    m_textureUploader->markPendingUploadsAsNonBlocking();
}

double ResourceProvider::estimatedUploadsPerSecond()
{
    if (!m_textureUploader)
        return 0.0;

    return m_textureUploader->estimatedTexturesPerSecond();
}

void ResourceProvider::flushUploads()
{
    if (!m_textureUploader)
        return;

    m_textureUploader->flush();
}

void ResourceProvider::flush()
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    if (context3d)
        context3d->flush();
}

bool ResourceProvider::shallowFlushIfSupported()
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    if (!context3d || !m_useShallowFlush)
        return false;

    context3d->shallowFlushCHROMIUM();
    return true;
}

const ResourceProvider::Resource* ResourceProvider::lockForRead(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->lockedForWrite);
    DCHECK(!resource->exported);
    resource->lockForReadCount++;
    return resource;
}

void ResourceProvider::unlockForRead(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(resource->lockForReadCount > 0);
    DCHECK(!resource->exported);
    resource->lockForReadCount--;
}

const ResourceProvider::Resource* ResourceProvider::lockForWrite(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->lockedForWrite);
    DCHECK(!resource->lockForReadCount);
    DCHECK(!resource->exported);
    DCHECK(!resource->external);
    resource->lockedForWrite = true;
    return resource;
}

void ResourceProvider::unlockForWrite(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(resource->lockedForWrite);
    DCHECK(!resource->exported);
    DCHECK(!resource->external);
    resource->lockedForWrite = false;
}

ResourceProvider::ScopedReadLockGL::ScopedReadLockGL(ResourceProvider* resourceProvider, ResourceProvider::ResourceId resourceId)
    : m_resourceProvider(resourceProvider)
    , m_resourceId(resourceId)
    , m_textureId(resourceProvider->lockForRead(resourceId)->glId)
{
    DCHECK(m_textureId);
}

ResourceProvider::ScopedReadLockGL::~ScopedReadLockGL()
{
    m_resourceProvider->unlockForRead(m_resourceId);
}

ResourceProvider::ScopedSamplerGL::ScopedSamplerGL(ResourceProvider* resourceProvider, ResourceProvider::ResourceId resourceId, GLenum target, GLenum filter)
    : ScopedReadLockGL(resourceProvider, resourceId)
{
    resourceProvider->bindForSampling(resourceId, target, filter);
}

ResourceProvider::ScopedWriteLockGL::ScopedWriteLockGL(ResourceProvider* resourceProvider, ResourceProvider::ResourceId resourceId)
    : m_resourceProvider(resourceProvider)
    , m_resourceId(resourceId)
    , m_textureId(resourceProvider->lockForWrite(resourceId)->glId)
{
    DCHECK(m_textureId);
}

ResourceProvider::ScopedWriteLockGL::~ScopedWriteLockGL()
{
    m_resourceProvider->unlockForWrite(m_resourceId);
}

void ResourceProvider::populateSkBitmapWithResource(SkBitmap* skBitmap, const Resource* resource)
{
    DCHECK(resource->pixels);
    DCHECK(resource->format == GL_RGBA);
    skBitmap->setConfig(SkBitmap::kARGB_8888_Config, resource->size.width(), resource->size.height());
    skBitmap->setPixels(resource->pixels);
}

ResourceProvider::ScopedReadLockSoftware::ScopedReadLockSoftware(ResourceProvider* resourceProvider, ResourceProvider::ResourceId resourceId)
    : m_resourceProvider(resourceProvider)
    , m_resourceId(resourceId)
{
    ResourceProvider::populateSkBitmapWithResource(&m_skBitmap, resourceProvider->lockForRead(resourceId));
}

ResourceProvider::ScopedReadLockSoftware::~ScopedReadLockSoftware()
{
    m_resourceProvider->unlockForRead(m_resourceId);
}

ResourceProvider::ScopedWriteLockSoftware::ScopedWriteLockSoftware(ResourceProvider* resourceProvider, ResourceProvider::ResourceId resourceId)
    : m_resourceProvider(resourceProvider)
    , m_resourceId(resourceId)
{
    ResourceProvider::populateSkBitmapWithResource(&m_skBitmap, resourceProvider->lockForWrite(resourceId));
    m_skCanvas.reset(new SkCanvas(m_skBitmap));
}

ResourceProvider::ScopedWriteLockSoftware::~ScopedWriteLockSoftware()
{
    m_resourceProvider->unlockForWrite(m_resourceId);
}

ResourceProvider::ResourceProvider(OutputSurface* context)
    : m_outputSurface(context)
    , m_nextId(1)
    , m_nextChild(1)
    , m_defaultResourceType(GLTexture)
    , m_useTextureStorageExt(false)
    , m_useTextureUsageHint(false)
    , m_useShallowFlush(false)
    , m_maxTextureSize(0)
    , m_bestTextureFormat(0)
{
}

bool ResourceProvider::initialize()
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    if (!context3d) {
        m_maxTextureSize = INT_MAX / 2;
        m_bestTextureFormat = GL_RGBA;
        return true;
    }
    if (!context3d->makeContextCurrent())
        return false;

    std::string extensionsString = UTF16ToASCII(context3d->getString(GL_EXTENSIONS));
    std::vector<std::string> extensions;
    base::SplitString(extensionsString, ' ', &extensions);
    bool useMapSub = false;
    bool useBindUniform = false;
    bool useBGRA = false;
    for (size_t i = 0; i < extensions.size(); ++i) {
        if (extensions[i] == "GL_EXT_texture_storage")
            m_useTextureStorageExt = true;
        else if (extensions[i] == "GL_ANGLE_texture_usage")
            m_useTextureUsageHint = true;
        else if (extensions[i] == "GL_CHROMIUM_map_sub")
            useMapSub = true;
        else if (extensions[i] == "GL_CHROMIUM_shallow_flush")
            m_useShallowFlush = true;
        else if (extensions[i] == "GL_CHROMIUM_bind_uniform_location")
            useBindUniform = true;
        else if (extensions[i] == "GL_EXT_texture_format_BGRA8888")
          useBGRA = true;
    }

    m_textureCopier = AcceleratedTextureCopier::create(context3d, useBindUniform);

    m_textureUploader = TextureUploader::create(context3d, useMapSub, m_useShallowFlush);
    GLC(context3d, context3d->getIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize));
    m_bestTextureFormat = PlatformColor::bestTextureFormat(context3d, useBGRA);
    return true;
}

int ResourceProvider::createChild()
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    Child childInfo;
    int child = m_nextChild++;
    m_children[child] = childInfo;
    return child;
}

void ResourceProvider::destroyChild(int child_id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ChildMap::iterator it = m_children.find(child_id);
    DCHECK(it != m_children.end());
    Child& child = it->second;
    for (ResourceIdMap::iterator child_it = child.childToParentMap.begin(); child_it != child.childToParentMap.end(); ++child_it)
        deleteResource(child_it->second);
    m_children.erase(it);
}

const ResourceProvider::ResourceIdMap& ResourceProvider::getChildToParentMap(int child) const
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ChildMap::const_iterator it = m_children.find(child);
    DCHECK(it != m_children.end());
    return it->second.childToParentMap;
}

void ResourceProvider::prepareSendToParent(const ResourceIdArray& resources, TransferableResourceList* list)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    list->sync_point = 0;
    list->resources.clear();
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    if (!context3d || !context3d->makeContextCurrent()) {
        // FIXME: Implement this path for software compositing.
        return;
    }
    for (ResourceIdArray::const_iterator it = resources.begin(); it != resources.end(); ++it) {
        TransferableResource resource;
        if (transferResource(context3d, *it, &resource)) {
            m_resources.find(*it)->second.exported = true;
            list->resources.push_back(resource);
        }
    }
    if (list->resources.size())
        list->sync_point = context3d->insertSyncPoint();
}

void ResourceProvider::prepareSendToChild(int child, const ResourceIdArray& resources, TransferableResourceList* list)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    list->sync_point = 0;
    list->resources.clear();
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    if (!context3d || !context3d->makeContextCurrent()) {
        // FIXME: Implement this path for software compositing.
        return;
    }
    Child& childInfo = m_children.find(child)->second;
    for (ResourceIdArray::const_iterator it = resources.begin(); it != resources.end(); ++it) {
        TransferableResource resource;
        if (!transferResource(context3d, *it, &resource))
            NOTREACHED();
        DCHECK(childInfo.parentToChildMap.find(*it) != childInfo.parentToChildMap.end());
        resource.id = childInfo.parentToChildMap[*it];
        childInfo.parentToChildMap.erase(*it);
        childInfo.childToParentMap.erase(resource.id);
        list->resources.push_back(resource);
        deleteResource(*it);
    }
    if (list->resources.size())
        list->sync_point = context3d->insertSyncPoint();
}

void ResourceProvider::receiveFromChild(int child, const TransferableResourceList& resources)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    if (!context3d || !context3d->makeContextCurrent()) {
        // FIXME: Implement this path for software compositing.
        return;
    }
    if (resources.sync_point) {
        // NOTE: If the parent is a browser and the child a renderer, the parent
        // is not supposed to have its context wait, because that could induce
        // deadlocks and/or security issues. The caller is responsible for
        // waiting asynchronously, and resetting sync_point before calling this.
        // However if the parent is a renderer (e.g. browser tag), it may be ok
        // (and is simpler) to wait.
        GLC(context3d, context3d->waitSyncPoint(resources.sync_point));
    }
    Child& childInfo = m_children.find(child)->second;
    for (TransferableResourceArray::const_iterator it = resources.resources.begin(); it != resources.resources.end(); ++it) {
        unsigned textureId;
        GLC(context3d, textureId = context3d->createTexture());
        GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, textureId));
        GLC(context3d, context3d->consumeTextureCHROMIUM(GL_TEXTURE_2D, it->mailbox.name));
        ResourceId id = m_nextId++;
        Resource resource(textureId, it->size, it->format, it->filter);
        resource.mailbox.setName(it->mailbox.name);
        m_resources[id] = resource;
        childInfo.parentToChildMap[id] = it->id;
        childInfo.childToParentMap[it->id] = id;
    }
}

void ResourceProvider::receiveFromParent(const TransferableResourceList& resources)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    if (!context3d || !context3d->makeContextCurrent()) {
        // FIXME: Implement this path for software compositing.
        return;
    }
    if (resources.sync_point)
        GLC(context3d, context3d->waitSyncPoint(resources.sync_point));
    for (TransferableResourceArray::const_iterator it = resources.resources.begin(); it != resources.resources.end(); ++it) {
        ResourceMap::iterator mapIterator = m_resources.find(it->id);
        DCHECK(mapIterator != m_resources.end());
        Resource* resource = &mapIterator->second;
        DCHECK(resource->exported);
        resource->exported = false;
        resource->mailbox.setName(it->mailbox.name);
        GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, resource->glId));
        GLC(context3d, context3d->consumeTextureCHROMIUM(GL_TEXTURE_2D, it->mailbox.name));
        if (resource->markedForDeletion)
            deleteResourceInternal(mapIterator);
    }
}

bool ResourceProvider::transferResource(WebGraphicsContext3D* context, ResourceId id, TransferableResource* resource)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* source = &it->second;
    DCHECK(!source->lockedForWrite);
    DCHECK(!source->lockForReadCount);
    DCHECK(!source->external);
    if (source->exported)
        return false;
    resource->id = id;
    resource->format = source->format;
    resource->filter = source->filter;
    resource->size = source->size;

    if (source->mailbox.isZero()) {
      GLbyte name[GL_MAILBOX_SIZE_CHROMIUM];
      GLC(context3d, context3d->genMailboxCHROMIUM(name));
      source->mailbox.setName(name);
    }

    resource->mailbox = source->mailbox;
    GLC(context, context->bindTexture(GL_TEXTURE_2D, source->glId));
    GLC(context, context->produceTextureCHROMIUM(GL_TEXTURE_2D, resource->mailbox.name));
    return true;
}

void ResourceProvider::acquirePixelBuffer(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->external);
    DCHECK(!resource->exported);

    if (resource->glId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        if (!resource->glPixelBufferId)
            resource->glPixelBufferId = context3d->createBuffer();
        context3d->bindBuffer(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
            resource->glPixelBufferId);
        context3d->bufferData(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
            resource->size.width() * resource->size.height() * 4,
            NULL,
            GL_DYNAMIC_DRAW);
        context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
    }

    if (resource->pixels) {
        if (resource->pixelBuffer)
            return;

        resource->pixelBuffer = new uint8_t[
            resource->size.width() * resource->size.height() * 4];
    }
}

void ResourceProvider::releasePixelBuffer(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->external);
    DCHECK(!resource->exported);

    if (resource->glId) {
        DCHECK(resource->glPixelBufferId);
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        context3d->bindBuffer(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
            resource->glPixelBufferId);
        context3d->bufferData(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
            0,
            NULL,
            GL_DYNAMIC_DRAW);
        context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
    }

    if (resource->pixels) {
        if (!resource->pixelBuffer)
            return;
        delete[] resource->pixelBuffer;
        resource->pixelBuffer = 0;
    }
}

uint8_t* ResourceProvider::mapPixelBuffer(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->external);
    DCHECK(!resource->exported);

    if (resource->glId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        DCHECK(resource->glPixelBufferId);
        context3d->bindBuffer(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
            resource->glPixelBufferId);
        uint8_t* image = static_cast<uint8_t*>(
            context3d->mapBufferCHROMIUM(
                GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, GL_WRITE_ONLY));
        context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
        DCHECK(image);
        return image;
    }

    if (resource->pixels)
      return resource->pixelBuffer;

    return NULL;
}

void ResourceProvider::unmapPixelBuffer(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->external);
    DCHECK(!resource->exported);

    if (resource->glId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        DCHECK(resource->glPixelBufferId);
        context3d->bindBuffer(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
            resource->glPixelBufferId);
        context3d->unmapBufferCHROMIUM(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM);
        context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
    }
}

void ResourceProvider::setPixelsFromBuffer(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->lockedForWrite);
    DCHECK(!resource->lockForReadCount);
    DCHECK(!resource->external);
    DCHECK(!resource->exported);

    if (resource->glId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        DCHECK(resource->glPixelBufferId);
        context3d->bindTexture(GL_TEXTURE_2D, resource->glId);
        context3d->bindBuffer(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
            resource->glPixelBufferId);
        context3d->texSubImage2D(GL_TEXTURE_2D,
                                 0, /* level */
                                 0, /* x */
                                 0, /* y */
                                 resource->size.width(),
                                 resource->size.height(),
                                 resource->format,
                                 GL_UNSIGNED_BYTE,
                                 NULL);
        context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
    }

    if (resource->pixels) {
        DCHECK(resource->pixelBuffer);
        DCHECK(resource->format == GL_RGBA);
        SkBitmap src;
        src.setConfig(SkBitmap::kARGB_8888_Config,
                      resource->size.width(),
                      resource->size.height());
        src.setPixels(resource->pixelBuffer);

        ScopedWriteLockSoftware lock(this, id);
        SkCanvas* dest = lock.skCanvas();
        dest->writePixels(src, 0, 0);
    }
}

void ResourceProvider::bindForSampling(ResourceProvider::ResourceId resourceId, GLenum target, GLenum filter)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
    ResourceMap::iterator it = m_resources.find(resourceId);
    DCHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(resource->lockForReadCount);
    DCHECK(!resource->lockedForWrite);

    GLC(context3d, context3d->bindTexture(target, resource->glId));
    if (filter != resource->filter) {
        GLC(context3d, context3d->texParameteri(target, GL_TEXTURE_MIN_FILTER, filter));
        GLC(context3d, context3d->texParameteri(target, GL_TEXTURE_MAG_FILTER, filter));
        resource->filter = filter;
    }
}

void ResourceProvider::beginSetPixels(ResourceId id)
{
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(!resource->pendingSetPixels);

    lockForWrite(id);

    if (resource->glId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        DCHECK(resource->glPixelBufferId);
        context3d->bindTexture(GL_TEXTURE_2D, resource->glId);
        context3d->bindBuffer(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
            resource->glPixelBufferId);
        if (!resource->glUploadQueryId)
            resource->glUploadQueryId = context3d->createQueryEXT();
        context3d->beginQueryEXT(
            GL_ASYNC_PIXEL_TRANSFERS_COMPLETED_CHROMIUM,
            resource->glUploadQueryId);
        context3d->asyncTexSubImage2DCHROMIUM(GL_TEXTURE_2D,
                                              0, /* level */
                                              0, /* x */
                                              0, /* y */
                                              resource->size.width(),
                                              resource->size.height(),
                                              resource->format,
                                              GL_UNSIGNED_BYTE,
                                              NULL);
        context3d->endQueryEXT(GL_ASYNC_PIXEL_TRANSFERS_COMPLETED_CHROMIUM);
        context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
    }

    if (resource->pixels)
      setPixelsFromBuffer(id);

    resource->pendingSetPixels = true;
}

bool ResourceProvider::didSetPixelsComplete(ResourceId id) {
    DCHECK(m_threadChecker.CalledOnValidThread());
    ResourceMap::iterator it = m_resources.find(id);
    CHECK(it != m_resources.end());
    Resource* resource = &it->second;
    DCHECK(resource->lockedForWrite);
    DCHECK(resource->pendingSetPixels);

    if (resource->glId) {
        WebGraphicsContext3D* context3d = m_outputSurface->Context3D();
        DCHECK(context3d);
        DCHECK(resource->glUploadQueryId);
        unsigned complete = 1;
        context3d->getQueryObjectuivEXT(
            resource->glUploadQueryId,
            GL_QUERY_RESULT_AVAILABLE_EXT,
            &complete);
        if (!complete)
            return false;
    }

    resource->pendingSetPixels = false;
    unlockForWrite(id);

    return true;
}

}  // namespace cc
