// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.graphics.SurfaceTexture;
import android.view.TextureView;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * This class is a shell around a set of functions only available on some Android version.
 * These functions allows to swap the consumer on a {@link SurfaceTexture}, and also allow to
 * detach and attach a {@link SurfaceTexture} on the {@link TextureView}.
 */
public class SurfaceTextureSwapConsumer {
    private static final String TAG = SurfaceTextureSwapConsumer.class.getSimpleName();
    private static Boolean sIsEnabled;

    private static Boolean sIsSupported;
    private static Method sDetachFromGLContextMethod;
    private static Method sAttachToGLContextMethod;
    private static Method sSetSurfaceTextureMethod;
    private static Method sSetDefaultBufferSizeMethod;

    /**
    * Set if {@link SurfaceTextureSwapConsumer} is enabled at all.
    *
    * <b>This function is only effective if called before any other function in the
    * {@link SurfaceTextureSwapConsumer} class. And only the first call to this function is taken
    * into account.</b> This constrains are to prevent from ending up in a bad state.
    *
    * @param enabled Whether the {@link SurfaceTextureSwapConsumer} feature is enabled.
    * @return whether the function succeed to set the enabled state.
    */
    static public boolean setEnabled(boolean enabled) {
        if (sIsEnabled == null) {
            sIsEnabled = new Boolean(enabled);
            return true;
        }
        return false;
    }

    /**
     * @return Whether the current Android version supports the swap of {@link SurfaceTexture} on
     *         a {@link TextureView}.
     */
    static public boolean isSupported() {
        if (sIsSupported == null) {
            sIsSupported = new Boolean(false);
            if (sIsEnabled == null || sIsEnabled.booleanValue()) {
                setEnabled(true);
                try {
                    sDetachFromGLContextMethod = SurfaceTexture.class.getDeclaredMethod(
                            "detachFromGLContext");
                    sAttachToGLContextMethod = SurfaceTexture.class.getDeclaredMethod(
                            "attachToGLContext", int.class);
                    sSetSurfaceTextureMethod = TextureView.class.getDeclaredMethod(
                            "setSurfaceTexture", SurfaceTexture.class);
                    sSetDefaultBufferSizeMethod = SurfaceTexture.class.getDeclaredMethod(
                            "setDefaultBufferSize", int.class, int.class);
                    sIsSupported = true;
                } catch (SecurityException e) {
                    setUnsupported(null);
                } catch (NoSuchMethodException e) {
                    setUnsupported(null);
                }
            }
        }
        return sIsSupported.booleanValue();
    }

    private static void setUnsupported(Exception e) {
        sIsSupported = false;
        if (e != null) {
            Log.e(TAG, "SurfaceTextureSwapConsumer is detected to be supported but it is not.", e);
        }
    }

    /**
     * Detaches a {@link SurfaceTexture} from the current openGL context.
     * @see SurfaceTexture#detachFromGLContext()
     * @param surfaceTexture The {@link SurfaceTexture} to detach from the openGL context on the
     *                       thread the function is called.
     */
    public static boolean detachFromGLContext(SurfaceTexture surfaceTexture)
            throws RuntimeException {
        if (!isSupported()) return false;
        try {
            sDetachFromGLContextMethod.invoke(surfaceTexture);
            return true;
        } catch (IllegalArgumentException e) {
            setUnsupported(e);
        } catch (IllegalAccessException e) {
            setUnsupported(e);
        } catch (InvocationTargetException e) {
            if (e.getTargetException() instanceof RuntimeException) {
                throw ((RuntimeException) e.getTargetException());
            } else {
                setUnsupported(e);
            }
        }
        return false;
    }

    /**
     * Attaches a {@link SurfaceTexture} to the current openGL context.
     * @see SurfaceTexture#attachToGLContext(int)
     * @param surfaceTexture The {@link SurfaceTexture} to attach to the current opneGL context.
     * @param textureName The name of the texture targetted by the {@link SurfaceTexture}.
     */
    public static boolean attachToGLContext(SurfaceTexture surfaceTexture, int textureName)
        throws RuntimeException {
        if (!isSupported()) return false;
        try {
            sAttachToGLContextMethod.invoke(surfaceTexture, textureName);
            return true;
        } catch (IllegalArgumentException e) {
            setUnsupported(e);
        } catch (IllegalAccessException e) {
            setUnsupported(e);
        } catch (InvocationTargetException e) {
            if (e.getTargetException() instanceof RuntimeException) {
                throw ((RuntimeException) e.getTargetException());
            } else {
                setUnsupported(e);
            }
        }
        return false;
    }

    /**
     * Sets a {@link SurfaceTexture} to a {@link TextureView}.
     * @see TextureView#SetSurfaceTexture(SurfaceTexture)
     * @param textureView    The {@link TextureView} on which to set the {@link SurfaceTexture}.
     * @param surfaceTexture The {@link SurfaceTexture} to set on the {@link TextureView}.
     */
    public static boolean setSurfaceTexture(
            TextureView textureView, SurfaceTexture surfaceTexture) throws NullPointerException {
        if (!isSupported()) return false;
        try {
            sSetSurfaceTextureMethod.invoke(textureView, surfaceTexture);
            return true;
        } catch (IllegalArgumentException e) {
            setUnsupported(e);
        } catch (IllegalAccessException e) {
            setUnsupported(e);
        } catch (InvocationTargetException e) {
            if (e.getTargetException() instanceof NullPointerException) {
                throw ((NullPointerException) e.getTargetException());
            } else {
                setUnsupported(e);
            }
        }
        return false;
    }

    /**
     * Sets the default size of the buffers for this {@link SurfaceTexture}.
     * @see SurfaceTexture#setDefaultBufferSize(int, int)
     * @param surfaceTexture The {@link SurfaceTexture} on which to call
     *                       {@link SurfaceTexture#setDefaultBufferSize(int, int)}.
     * @param width          The new default buffer width.
     * @param height         The new default buffer height.
     */
    public static void setDefaultBufferSize(SurfaceTexture surfaceTexture, int width, int height) {
        if (!isSupported()) return;
        try {
            sSetDefaultBufferSizeMethod.invoke(surfaceTexture, width, height);
        } catch (IllegalArgumentException e) {
            setUnsupported(e);
        } catch (IllegalAccessException e) {
            setUnsupported(e);
        } catch (InvocationTargetException e) {
            setUnsupported(e);
        }
    }
}
