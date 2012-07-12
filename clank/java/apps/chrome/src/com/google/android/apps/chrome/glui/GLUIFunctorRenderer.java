// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.glui;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.opengl.GLES20;
import android.util.Log;
import android.view.View;

import org.chromium.base.CalledByNative;

import com.google.android.apps.chrome.ChromeNotificationCenter;
import com.google.android.apps.chrome.glui.view.GLUIFunctorView;

import java.nio.ByteBuffer;
import java.lang.reflect.*;

/**
 * Wire the rendering of the {@link GLUIFunctorView} to be rendered as part of Android UI.
 * This class has a C++ counterpart using the linked with JNI.
 */
public class GLUIFunctorRenderer {
    /**
     * This interface listens for any draw calls made by the GLUIFunctorRenderer for cases where
     * invalidation of the View is unpredictable.
     */
    public interface Listener {
        /** Alerts the listener that GLUIFunctorRenderer.onDrawFrame() has been called. */
        public void onFrameRendered();
    }

    private static final String TAG = "GLUIFunctorRenderer";
    private static final boolean debuggingOnScreenPerformance = false;
    private static final boolean debuggingPrintPerformance = false;

    // Magic key
    private static final int GL_TEXTURE_EXTERNAL_OES = 0x8D65;
    private static final int BYTES_PER_PIXEL = 4;

    private GLRenderer mRenderer;
    private int mFrameBufferWidth = 0;
    private int mFrameBufferHeight = 0;

    // JNI c++ pointer
    private int mNativeGLUIFunctorRenderer = 0;

    // Reflection cache
    private Class<?> mHardwareCanvasClass;
    private Method mCallDrawGLFunction;
    private Object[] mDrawFunctor;

    // On screen performance meter
    private long mStartDraw;
    private long mEndDraw;
    private long mLastEndDraw;
    private float mDrawMin;
    private float mDrawMax;
    private float mDrawAvg;
    private float mFrameMin;
    private float mFrameMax;
    private float mFrameAvg;
    private final static int mFrameCount = 100;
    private int mCurrentFrame = 0;
    private float[] mDraws = new float[mFrameCount];
    private float[] mFrames = new float[mFrameCount];

    // Temporary frame buffer handles and memory
    private int[] mFBOHandle = new int[1];
    private int[] mTextureHandle = new int[1];
    private int[] mDepthHandle = new int[1];
    private boolean mOffscreenBufferDirty = true;

    private Listener mListener;

    /**
     * @param renderer The renderer to trigger on onDraw.
     */
    public GLUIFunctorRenderer(GLRenderer renderer) {
        mRenderer = renderer;

        try {
            mHardwareCanvasClass = Class.forName("android.view.HardwareCanvas");
            mCallDrawGLFunction =
                    mHardwareCanvasClass.getDeclaredMethod("callDrawGLFunction", int.class);
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "Constructor: Could not find android.view.HardwareCanvas. " +
                    "The tab switcher will not show.");
        } catch (NoSuchMethodException e) {
            Log.e(TAG, "Constructor: android.view.HardwareCanvas does not have " +
                    "callDrawGLFunction. The tab switcher will not show.");
        }
    }

    /**
     * Initializes the native elements. To be called when the windows is attached.
     */
    public void attach() {
        mNativeGLUIFunctorRenderer = nativeInit();
        mDrawFunctor = new Object[] {
                new Integer(nativeGetDrawFunction(mNativeGLUIFunctorRenderer))
        };
    }

    /**
     * Destroys the native elements. To be called when the windows is detached.
     */
    public void detach() {
        nativeDestroy(mNativeGLUIFunctorRenderer);
    }

    /**
     * Initializes the renderer for a new session. To be called every time it becomes visible.
     * @param width     The Width of the rendering area.
     * @param height    The height of the rendering area.
     */
    public void init(int width, int height) {
        mRenderer.onSurfaceChanged(width, height);

        mDrawMin = 1000;
        mDrawMax = 0;
        mDrawAvg = 0;
        mFrameMin = 1000;
        mFrameMax = 0;
        mFrameAvg = 0;
        mStartDraw = System.nanoTime();
        mEndDraw = System.nanoTime();
        mLastEndDraw = mEndDraw;
    }

    /**
     * Used to alert outside code that a draw has been performed.
     */
    public void setListener(Listener listener) {
        mListener = listener;
    }

    /**
     * Called when the View change size. It may be called even if the size did not change.
     * @param width The new view width.
     * @param height Then new view height.
     */
    public void sizeChanged(int width, int height) {
        mRenderer.onSurfaceChanged(width, height);
    }

    /**
     * Invalidates the offscreen buffer. So the next time the Android framework requests to draw
     * the layout will be re-drawn.
     */
    public void invalidate() {
        mOffscreenBufferDirty = true;
    }

    /**
     * Hooks the native draw function to the UI so when the UI actually issue the GL calls the
     * native function get called.
     * @param view      The current View this renderer is attached to.
     * @param canvas    The canvas to which the renderer renders.
     */
    public void onDraw(View view, Canvas canvas) {
        invalidate();
        if (mHardwareCanvasClass == null || mCallDrawGLFunction == null) return;

        // This is for generating screenshot only
        if (!mHardwareCanvasClass.isInstance(canvas)) {
            Bitmap src = getBitmapOfOffscreenBuffer();
            if (src != null) {
                try {
                    // The capture is unfortunately vertically mirrored.
                    Matrix m = new Matrix();
                    m.preScale(1, -1);
                    Bitmap dst = Bitmap.createBitmap(
                            src, 0, 0, src.getWidth(), src.getHeight(), m, false);
                    canvas.drawBitmap(dst, 0, 0, null);
                    dst.recycle();
                } catch (OutOfMemoryError e) {
                    Log.e(TAG, "onDraw: could not allocate destination bitmap");
                } finally {
                    src.recycle();
                }
            }
            return;
        }

        try {
            Object hardwareCanvas = mHardwareCanvasClass.cast(canvas);
            Object r = mCallDrawGLFunction.invoke(hardwareCanvas, mDrawFunctor);
            if (((r instanceof Boolean) && ((Boolean) r).booleanValue()) ||
                    ((r instanceof Integer) && ((Integer) r).intValue() > 0)) {
                view.invalidate();
            }
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, "onDraw: Could not initialize android.view.HardwareCanvas.", e);
        } catch (LinkageError e) {
            Log.e(TAG, "onDraw: android.view.HardwareCanvas can not be linked.", e);
        } catch (ClassCastException e) {
            Log.e(TAG, "onDraw: Could not cast result to Integer or Booleean", e);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "onDraw: callDrawGLFunction called with illegal arguments.", e);
        } catch (IllegalAccessException e) {
            Log.e(TAG, "onDraw: callDrawGLFunction IllegalAccessException.", e);
        } catch (InvocationTargetException e) {
            Log.e(TAG, "onDraw: callDrawGLFunction InvocationTargetException.", e);
        }
    }

    /**
     * This is just to get a screenshot for the sendfeedback form.
     * @return A bitmap of the offscreen buffer if any.
     */
    private Bitmap getBitmapOfOffscreenBuffer() {
        if (mFBOHandle[0] == 0) return null;

        Bitmap bitmap = null;
        int width = mFrameBufferWidth;
        int height = mFrameBufferHeight;
        int[] frame = new int[1];
        try {
            GLES20.glGenFramebuffers(1, frame, 0);
            GLErrorChecker.throwExceptionOnGLError("glGenFramebuffers");
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, frame[0]);
            GLErrorChecker.throwExceptionOnGLError("glBindFramebuffer");
            GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0,
                    GLES20.GL_TEXTURE_2D, mTextureHandle[0], 0);
            GLErrorChecker.throwExceptionOnGLError("glFramebufferTexture2D");

            ByteBuffer buffer = ByteBuffer.allocate(width * height * BYTES_PER_PIXEL);
            GLES20.glReadPixels(
                    0, 0, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buffer);
            GLErrorChecker.throwExceptionOnGLError("glReadPixels");
            bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            bitmap.copyPixelsFromBuffer(buffer);

            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
            GLErrorChecker.throwExceptionOnGLError("glBindFramebuffer");
            GLES20.glDeleteFramebuffers(1, frame, 0);
            GLErrorChecker.throwExceptionOnGLError("glDeleteFramebuffer");
        } catch (Throwable e) {
            // Catching generic exception here on purpose.
            Log.e(TAG, "Error generating the screenshot", e);
            bitmap = null;
        }
        return bitmap;
    }


    /**
     *  This is called right after the last frame is drawn.
     */
    public void onViewHidden() {
        mRenderer.onViewHidden();
        destroyOffscreenBuffer();
    }

    /**
     * Releases the resources allocated for frame buffer.
     */
    private void destroyOffscreenBuffer() {
        if (mFBOHandle[0] != 0) {
            GLES20.glDeleteFramebuffers(1, mFBOHandle, 0);
            GLES20.glDeleteRenderbuffers(1, mDepthHandle, 0);
            GLES20.glDeleteTextures(1, mTextureHandle, 0);
            mFBOHandle[0] = 0;
        }
    }

    /**
     * Sets up the frame buffer.  This function explicitly checks for out of memory errors and
     * alerts the rest of the application if one is encountered for either CPU or GPU memory.
     * @param glErrorAllowed Whether having a glError is a show-stopper.
     * @return               whether or not it was successful.
     */
    private boolean setupOffscreenBuffer(boolean glErrorAllowed) {
        boolean encounteredError = false;
        boolean ranOutOfMemory = false;

        try {
            // Initialize the frame buffer if needed
            if (mFBOHandle[0] == 0 ||
                    mFrameBufferWidth != mRenderer.getWidth() ||
                    mFrameBufferHeight != mRenderer.getHeight()) {
                mOffscreenBufferDirty = true;
                destroyOffscreenBuffer();

                mFrameBufferWidth = mRenderer.getWidth();
                mFrameBufferHeight = mRenderer.getHeight();

                // FBO
                GLES20.glGenFramebuffers(1, mFBOHandle, 0);
                GLErrorChecker.throwExceptionOnGLError("glGenFramebuffers - mFBOHandle");
                GLES20.glGenRenderbuffers(1, mDepthHandle, 0);
                GLErrorChecker.throwExceptionOnGLError("glGenFramebuffers - mDepthHandle");
                GLES20.glGenTextures(1, mTextureHandle, 0);
                GLErrorChecker.throwExceptionOnGLError("glGenFramebuffers - mTextureHandle");

                // COLOR
                GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureHandle[0]);
                GLErrorChecker.throwExceptionOnGLError("glBindTexture - mTextureHandle");
                GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
                        GLES20.GL_LINEAR);
                GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
                        GLES20.GL_LINEAR);
                GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
                        GLES20.GL_CLAMP_TO_EDGE);
                GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
                        GLES20.GL_CLAMP_TO_EDGE);

                GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA,
                        mFrameBufferWidth, mFrameBufferHeight, 0,
                        GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
                GLErrorChecker.throwExceptionOnGLError("glTexImage2D");

                // DEPTH
                GLES20.glBindRenderbuffer(GLES20.GL_RENDERBUFFER, mDepthHandle[0]);
                GLErrorChecker.throwExceptionOnGLError("glBindRenderbuffer");
                GLES20.glRenderbufferStorage(GLES20.GL_RENDERBUFFER, GLES20.GL_DEPTH_COMPONENT16,
                        mFrameBufferWidth, mFrameBufferHeight);
                GLErrorChecker.throwExceptionOnGLError("glRenderbufferStorage");
            }

            // Bind the current rendering to an off-screen buffer so it can use a depth buffer.
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFBOHandle[0]);
            GLErrorChecker.throwExceptionOnGLError("glBindFramebuffer");
            GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0,
                    GLES20.GL_TEXTURE_2D, mTextureHandle[0], 0);
            GLErrorChecker.throwExceptionOnGLError("glFramebufferTexture2D");
            GLES20.glFramebufferRenderbuffer(GLES20.GL_FRAMEBUFFER, GLES20.GL_DEPTH_ATTACHMENT,
                    GLES20.GL_RENDERBUFFER, mDepthHandle[0]);
            GLErrorChecker.throwExceptionOnGLError("glFramebufferRenderbuffer");

            // Check to see that the framebuffer is completely set up.
            int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
            if (status != GLES20.GL_FRAMEBUFFER_COMPLETE) {
                Log.w(TAG, "Frame buffer setup failed (" + status + ")");
                encounteredError = true;
            }
        } catch (GLErrorChecker.GLException e) {
            if (!glErrorAllowed) Log.w(TAG, "Encountered general GL error: ", e);
            encounteredError = true;
        } catch (GLErrorChecker.GLOutOfMemoryException e) {
            Log.w(TAG, "GL_OUT_OF_MEMORY error thrown while setting up offscreen buffer.");
            encounteredError = true;
            ranOutOfMemory = true;
        } catch (OutOfMemoryError e) {
            Log.w(TAG, "OutOfMemoryError thrown while trying to allocate a color buffer.");
            encounteredError = true;
            ranOutOfMemory = true;
        }

        if (encounteredError) {
            // Invalidate mFrameBufferWidth/Height so that initialization
            // will be attempted again on the next call to this method.
            mFrameBufferWidth = mFrameBufferHeight = -1;

            if (ranOutOfMemory) {
                Log.w(TAG, "Freeing memory for next attempt to set up buffer");

                // Fire a notification to try and clear up memory for the next attempt.
                ChromeNotificationCenter.broadcastImmediateNotification(
                        ChromeNotificationCenter.OUT_OF_MEMORY);

                destroyOffscreenBuffer();
            }

            return false;
        }

        return true;
    }

    private void flattenOffscreenBuffer() {
        mRenderer.flushDeferredDraw();

        // Render the offscreen buffer to the screen
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureHandle[0]);

        mRenderer.useTextureProgram(true);
        mRenderer.drawQuad(0, mRenderer.getHeight(), mRenderer.getWidth(), -mRenderer.getHeight());
    }

    private void printDebugPerformance() {
        mFrames[mCurrentFrame % mFrameCount] = (float) ((mEndDraw - mLastEndDraw) / 1000000.0);
        mDraws[mCurrentFrame % mFrameCount] = (float) ((mEndDraw - mStartDraw) / 1000000.0);
        mCurrentFrame++;
        if (mCurrentFrame % mFrameCount == 0) {
            float minFrame = Float.MAX_VALUE;
            float maxFrame = 0;
            float avgFrame = 0;
            float minDraw = Float.MAX_VALUE;
            float maxDraw = 0;
            float avgDraw = 0;
            for (int i = 0; i < mFrameCount; i++) {
                minFrame = Math.min(minFrame, mFrames[i]);
                maxFrame = Math.max(maxFrame, mFrames[i]);
                avgFrame += mFrames[i];
                minDraw = Math.min(minDraw, mDraws[i]);
                maxDraw = Math.max(maxDraw, mDraws[i]);
                avgDraw += mDraws[i];
            }
            avgFrame /= mFrameCount;
            avgDraw /= mFrameCount;
            minFrame = Math.round(minFrame * 100.0f) / 100.0f;
            maxFrame = Math.round(maxFrame * 100.0f) / 100.0f;
            avgFrame = Math.round(avgFrame * 100.0f) / 100.0f;
            minDraw = Math.round(minDraw * 100.0f) / 100.0f;
            maxDraw = Math.round(maxDraw * 100.0f) / 100.0f;
            avgDraw = Math.round(avgDraw * 100.0f) / 100.0f;
            Log.w(TAG, "--- Draw  " + avgDraw + " ms ( " + minDraw + " - " + maxDraw + " )");
            Log.w(TAG, "### Frame " + avgFrame + " ms ( " + minFrame + " - " + maxFrame + " )");
        }
    }

    private void drawDebugPerformance() {
        float drawTime = (float) ((mEndDraw - mStartDraw) / 1000000.0);
        mDrawMin = Math.min(drawTime, mDrawMin);
        mDrawMax = Math.max(drawTime, mDrawMax);
        mDrawMin = mDrawMin * 0.99f + mDrawMax * 0.01f;
        mDrawMax = mDrawMax * 0.99f + mDrawMin * 0.01f;
        mDrawAvg = mDrawAvg * 0.9f + drawTime * 0.1f;

        final int msCount = 60;
        final float margin = 20;
        final float meterHeight = 15;

        float frameTime = (float) ((mEndDraw - mLastEndDraw) / 1000000.0);
        if (frameTime < 2 * msCount) {
            mFrameMin = Math.min(frameTime, mFrameMin);
            mFrameMax = Math.max(frameTime, mFrameMax);
            mFrameMin = mFrameMin * 0.99f + mFrameMax * 0.01f;
            mFrameMax = mFrameMax * 0.99f + mFrameMin * 0.01f;
            mFrameAvg = mFrameAvg * 0.9f + frameTime * 0.1f;
        }

        float msX = (mRenderer.getWidth() - 2 * margin) / msCount;

        // Ensures the average does not take too long to come back on screen.
        mFrameMin = Math.min(mFrameMin, msCount * 1.2f);
        mFrameMax = Math.min(mFrameMax, msCount * 1.2f);
        mDrawMin = Math.min(mDrawMin, msCount * 1.2f);
        mDrawMax = Math.min(mDrawMax, msCount * 1.2f);

        // Draw the white background
        mRenderer.useColorProgram(1.f, 1.f, 1.f, 0.5f);
        mRenderer.drawQuad(0, 0, mRenderer.getWidth(), margin * 2 + 30);

        // Draw the units
        mRenderer.useColorProgram(0.f, 0.f, 0.f, 0.5f);
        for (int i = 0; i <= msCount; i++) {
            if (i % 10 == 0) {
                mRenderer.drawQuad(margin + i * msX, margin, 3, 30);
            } else if (i % 5 == 0) {
                mRenderer.drawQuad(margin + i * msX, margin, 2, 20);
            } else {
                mRenderer.drawQuad(margin + i * msX, margin, 1, 10);
            }
        }

        // Draw the CPU draw time bar
        mRenderer.useColorProgram(0.f, 1.f, 0.f, 0.5f);
        mRenderer.drawQuad(margin, margin + meterHeight / 3,
                mDrawAvg * msX, meterHeight / 3);
        mRenderer.drawQuad(margin + mDrawMin * msX, margin,
                (mDrawMax - mDrawMin) * msX, meterHeight);

        // Draw full frame time bar
        mRenderer.useColorProgram(1.f, 0.f, 0.f, 0.5f);
        mRenderer.drawQuad(margin, margin + meterHeight + meterHeight / 3,
                mFrameAvg * msX, meterHeight / 3);
        mRenderer.drawQuad(margin + mFrameMin * msX, margin + meterHeight,
                (mFrameMax - mFrameMin) * msX, meterHeight);

        mRenderer.flushDeferredDraw();
    }

    @CalledByNative
    public void onPreDrawCheck() {
        try {
            GLErrorChecker.checkGlOutOfMemoryError("onPreDrawFrame");
        } catch (RuntimeException ex) {
            // This should trigger freeing of open gl resources.  This will also throw an exception
            // in the case where a driver bug mistracks memory allocations and we get a
            // GL_OUT_OF_MEMORY error from benign gl calls.
            ChromeNotificationCenter.broadcastImmediateNotification(
                    ChromeNotificationCenter.OUT_OF_MEMORY);
        }
    }

    /**
     * Draws the frame. This function actually issues the GL calls. This is called by native
     * whenever the Android UI framework needs to.
     */
    @CalledByNative
    public void onDrawFrame() {
        mRenderer.onStartFrame();
        if (GLRenderer.useDepthOptimization()) {
            // Try to set up the offscreen buffer twice in case the first attempt fails due to a
            // memory error, which may free up enough memory for the second attempt to succeed.
            // TODO(jgreenwald,jscholler): Is there anything better we can do here?
            if (!setupOffscreenBuffer(true)) {
                Log.w(TAG, "Failed to set up offscreen buffer once.  Re-trying.");
                if (!setupOffscreenBuffer(false)) {
                    Log.e(TAG, "Failed to set up offscreen buffer twice in a row.  Aborting.");
                    return;
                }
            }
        }

        mStartDraw = System.nanoTime();

        if (mOffscreenBufferDirty || !GLRenderer.useDepthOptimization()) {
            mOffscreenBufferDirty = false;
            mRenderer.onDrawFrame();
        }

        mLastEndDraw = mEndDraw;
        mEndDraw = System.nanoTime();

        if (debuggingOnScreenPerformance) drawDebugPerformance();
        if (debuggingPrintPerformance) printDebugPerformance();

        if (GLRenderer.useDepthOptimization()) flattenOffscreenBuffer();

        mRenderer.onFinishFrame();

        if (mListener != null) {
            mListener.onFrameRendered();
        }
    }

    // JNI
    private native int nativeInit();
    private native void nativeDestroy(int nativeGLUIFunctorRenderer);
    private native int nativeGetDrawFunction(int nativeGLUIFunctorRenderer);
}
