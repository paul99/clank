// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_VERTEX_ATTRIB_MANAGER_H_
#define GPU_COMMAND_BUFFER_SERVICE_VERTEX_ATTRIB_MANAGER_H_

#include <list>
#include <vector>
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "gpu/command_buffer/service/buffer_manager.h"
#include "gpu/command_buffer/service/gl_utils.h"
#include "gpu/gpu_export.h"

namespace gpu {
namespace gles2 {

class VertexArrayManager;

// Manages vertex attributes.
// This class also acts as the service-side representation of a
// vertex array object and it's contained state.
class GPU_EXPORT VertexAttribManager :
    public base::RefCounted<VertexAttribManager> {
 public:
  typedef scoped_refptr<VertexAttribManager> Ref;

  // Info about Vertex Attributes. This is used to track what the user currently
  // has bound on each Vertex Attribute so that checking can be done at
  // glDrawXXX time.
  class GPU_EXPORT VertexAttribInfo {
   public:
    typedef std::list<VertexAttribInfo*> VertexAttribInfoList;

    VertexAttribInfo();
    ~VertexAttribInfo();

    // Returns true if this VertexAttrib can access index.
    bool CanAccess(GLuint index) const;

    BufferManager::BufferInfo* buffer() const {
      return buffer_;
    }

    GLsizei offset() const {
      return offset_;
    }

    GLuint index() const {
      return index_;
    }

    GLint size() const {
      return size_;
    }

    GLenum type() const {
      return type_;
    }

    GLboolean normalized() const {
      return normalized_;
    }

    GLsizei gl_stride() const {
      return gl_stride_;
    }

    GLuint divisor() const {
      return divisor_;
    }

    bool enabled() const {
      return enabled_;
    }

    // Find the maximum vertex accessed, accounting for instancing.
    GLuint MaxVertexAccessed(GLsizei primcount,
                             GLuint max_vertex_accessed) const {
      return (primcount && divisor_) ? ((primcount - 1) / divisor_) :
                                       max_vertex_accessed;
    }

   private:
    friend class VertexAttribManager;

    void set_enabled(bool enabled) {
      enabled_ = enabled;
    }

    void set_index(GLuint index) {
      index_ = index;
    }

    void SetList(VertexAttribInfoList* new_list) {
      DCHECK(new_list);

      if (list_) {
        list_->erase(it_);
      }

      it_ = new_list->insert(new_list->end(), this);
      list_ = new_list;
    }

    void SetInfo(
        BufferManager::BufferInfo* buffer,
        GLint size,
        GLenum type,
        GLboolean normalized,
        GLsizei gl_stride,
        GLsizei real_stride,
        GLsizei offset) {
      DCHECK_GT(real_stride, 0);
      buffer_ = buffer;
      size_ = size;
      type_ = type;
      normalized_ = normalized;
      gl_stride_ = gl_stride;
      real_stride_ = real_stride;
      offset_ = offset;
    }

    void SetDivisor(GLsizei divisor) {
      divisor_ = divisor;
    }

    void Unbind(BufferManager::BufferInfo* buffer) {
      if (buffer_ == buffer) {
        buffer_ = NULL;
      }
    }

    // The index of this attrib.
    GLuint index_;

    // Whether or not this attribute is enabled.
    bool enabled_;

    // number of components (1, 2, 3, 4)
    GLint size_;

    // GL_BYTE, GL_FLOAT, etc. See glVertexAttribPointer.
    GLenum type_;

    // The offset into the buffer.
    GLsizei offset_;

    GLboolean normalized_;

    // The stride passed to glVertexAttribPointer.
    GLsizei gl_stride_;

    // The stride that will be used to access the buffer. This is the actual
    // stide, NOT the GL bogus stride. In other words there is never a stride
    // of 0.
    GLsizei real_stride_;

    GLsizei divisor_;

    // The buffer bound to this attribute.
    BufferManager::BufferInfo::Ref buffer_;

    // List this info is on.
    VertexAttribInfoList* list_;

    // Iterator for list this info is on. Enabled/Disabled
    VertexAttribInfoList::iterator it_;
  };

  typedef std::list<VertexAttribInfo*> VertexAttribInfoList;

  VertexAttribManager();

  void Initialize(uint32 num_vertex_attribs, bool init_attribs = true);

  bool Enable(GLuint index, bool enable);

  bool HaveFixedAttribs() const {
    return num_fixed_attribs_ != 0;
  }

  const VertexAttribInfoList& GetEnabledVertexAttribInfos() const {
    return enabled_vertex_attribs_;
  }

  VertexAttribInfo* GetVertexAttribInfo(GLuint index) {
    if (index < vertex_attrib_infos_.size()) {
      return &vertex_attrib_infos_[index];
    }
    return NULL;
  }

  void SetAttribInfo(
      GLuint index,
      BufferManager::BufferInfo* buffer,
      GLint size,
      GLenum type,
      GLboolean normalized,
      GLsizei gl_stride,
      GLsizei real_stride,
      GLsizei offset) {
    VertexAttribInfo* info = GetVertexAttribInfo(index);
    if (info) {
      if (info->type() == GL_FIXED) {
        --num_fixed_attribs_;
      }
      if (type == GL_FIXED) {
        ++num_fixed_attribs_;
      }
      info->SetInfo(
          buffer, size, type, normalized, gl_stride, real_stride, offset);
    }
  }

  void SetDivisor(GLuint index, GLuint divisor) {
    VertexAttribInfo* info = GetVertexAttribInfo(index);
    if (info) {
      info->SetDivisor(divisor);
    }
  }

  void SetElementArrayBuffer(BufferManager::BufferInfo* buffer) {
    element_array_buffer_ = buffer;
  }

  BufferManager::BufferInfo* element_array_buffer() const {
    return element_array_buffer_;
  }

  GLuint service_id() const {
    return service_id_;
  }

  void Unbind(BufferManager::BufferInfo* buffer);

  bool IsDeleted() const {
    return deleted_;
  }

  bool IsValid() const {
    return !IsDeleted();
  }

  size_t num_attribs() const {
    return vertex_attrib_infos_.size();
  }

 private:
  friend class VertexArrayManager;
  friend class VertexArrayManagerTest;
  friend class base::RefCounted<VertexAttribManager>;

  // Used when creating from a VertexArrayManager
  VertexAttribManager(VertexArrayManager* manager, GLuint service_id,
      uint32 num_vertex_attribs);

  ~VertexAttribManager();

  void MarkAsDeleted() {
    deleted_ = true;
  }

  // number of attribs using type GL_FIXED.
  int num_fixed_attribs_;

  // Info for each vertex attribute saved so we can check at glDrawXXX time
  // if it is safe to draw.
  std::vector<VertexAttribInfo> vertex_attrib_infos_;

  // The currently bound element array buffer. If this is 0 it is illegal
  // to call glDrawElements.
  BufferManager::BufferInfo::Ref element_array_buffer_;

  // Lists for which vertex attribs are enabled, disabled.
  VertexAttribInfoList enabled_vertex_attribs_;
  VertexAttribInfoList disabled_vertex_attribs_;

  // The VertexArrayManager that owns this VertexAttribManager
  VertexArrayManager* manager_;

  // True if deleted.
  bool deleted_;

  // Service side vertex array object id.
  GLuint service_id_;
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_VERTEX_ATTRIB_MANAGER_H_

