// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_SRC_GLUI_FUNCTOR_RENDERER_H_
#define CLANK_NATIVE_FRAMEWORK_SRC_GLUI_FUNCTOR_RENDERER_H_

#include "base/memory/scoped_ptr.h"
#include "content/browser/android/jni_helper.h"

// C++ side of a Java GLUIFunctorRenderer.
// The GLUIFunctorRenderer offers a function pointer to the java side for drawing
// the OpenGL based UI (Tabswitcher, tabs animations, etc...).
class GLUIFunctorRenderer {
 public:
  GLUIFunctorRenderer(JNIEnv* env, jobject obj);
  ~GLUIFunctorRenderer();

  void Destroy(JNIEnv* env, jobject obj);
  jint GetDrawFunction(JNIEnv* env, jobject obj);

 private:
  class DrawFunctor;

  // Called indirectly via a functor.
  void Draw(JNIEnv* env);
  void PreDrawCheck(JNIEnv* env);

  static DrawFunctor* draw_functor_;

  JavaObjectWeakGlobalRef java_glui_functor_renderer_;
};

bool RegisterGLUIFunctorRenderer(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_SRC_GLUI_FUNCTOR_RENDERER_H_
