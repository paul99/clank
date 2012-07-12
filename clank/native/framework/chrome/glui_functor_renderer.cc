// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/glui_functor_renderer.h"

#include "utils/Functor.h" // Android include

#include "base/android/jni_android.h"
#include "jni/glui_functor_renderer_jni.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;

// This class holds a function pointer and parameters for calling Draw
// into a specific viewport.
// The pointer to the Functor will be put on a framework display list to be
// called when the display list is replayed.
class GLUIFunctorRenderer::DrawFunctor : public android::Functor {
 public:
  DrawFunctor() {
  }
  void set_renderer(GLUIFunctorRenderer* glui_functor_renderer_instance) {
      glui_functor_renderer_instance_ = glui_functor_renderer_instance;
  }
  virtual ~DrawFunctor() {}
  // This operator will be called when Android's UI requestion to render.
  virtual android::status_t operator()(int messageId, void* data) {
    if (glui_functor_renderer_instance_) {
      JNIEnv* env = AttachCurrentThread();
      glui_functor_renderer_instance_->PreDrawCheck(env);
      glui_functor_renderer_instance_->Draw(env);
    }
    // Returns 0 to indicate there is no need to call invalidate.
    return 0;
  }
 private:
  GLUIFunctorRenderer* glui_functor_renderer_instance_;
};

// ----------------------------------------------------------------

// We need a static pointer to the functor because we give it
// away to the Android framework and have no control of its lifetime.
// The only thing we can do is invalidating its effect when
// GLUIFunctorRenderer is deleted.
GLUIFunctorRenderer::DrawFunctor* GLUIFunctorRenderer::draw_functor_ = NULL;

GLUIFunctorRenderer::GLUIFunctorRenderer(JNIEnv* env, jobject obj)
    : java_glui_functor_renderer_(env, obj) {
}

GLUIFunctorRenderer::~GLUIFunctorRenderer() {
}

void GLUIFunctorRenderer::Destroy(JNIEnv* env, jobject obj) {
  if (draw_functor_) {
    draw_functor_->set_renderer(NULL);
  }
  delete this;
}

void GLUIFunctorRenderer::PreDrawCheck(JNIEnv* env) {
  // Check for any preexisting errors by calling into the predraw method
  Java_GLUIFunctorRenderer_onPreDrawCheck(
      env, java_glui_functor_renderer_.get(env).obj());
}

void GLUIFunctorRenderer::Draw(JNIEnv* env) {
  // Issues all the OpenGL call on the Java side.
  Java_GLUIFunctorRenderer_onDrawFrame(
      env, java_glui_functor_renderer_.get(env).obj());
}

jint GLUIFunctorRenderer::GetDrawFunction(JNIEnv* env, jobject obj) {
  if (!draw_functor_) {
    draw_functor_ = new DrawFunctor();
  }
  draw_functor_->set_renderer(this);
  // The Java side defines the funtion pointer as an int.
  return reinterpret_cast<jint>(draw_functor_);
}

// ----------------------------------------------------------------

static int Init(JNIEnv* env, jobject obj) {
  GLUIFunctorRenderer* glui_functor_renderer =
      new GLUIFunctorRenderer(env, obj);
  return reinterpret_cast<jint>(glui_functor_renderer);
}

bool RegisterGLUIFunctorRenderer(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
