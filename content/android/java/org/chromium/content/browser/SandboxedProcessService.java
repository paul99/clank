// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;
import android.view.Surface;

import org.chromium.base.CalledByNative;

class SandboxedProcessService extends Service {
    // Names of items placed in the bind intent or connection bundle.
    public static final String EXTRA_COMMAND_LINE =
        "com.google.android.apps.chrome.extra.sandbox_command_line";
    // Note the FD may only be passed in the connection bundle.
    public static final String EXTRA_IPC_FD = "com.google.android.apps.chrome.extra.sandbox_ipcFd";
    public static final String EXTRA_MINIDUMP_FD =
        "com.google.android.apps.chrome.extra.sandbox_minidumpFd";
    public static final String EXTRA_CHROME_PAK_FD =
        "com.google.android.apps.chrome.extra.sandbox_chromePakFd";
    public static final String EXTRA_LOCALE_PAK_FD =
        "com.google.android.apps.chrome.extra.sandbox_localePakFd";

    private static final String MAIN_THREAD_NAME = "SandboxedProcessMain";
    private static final String TAG = "SandboxedProcessService";
    private ISandboxedProcessCallback mCallback;

    // This is the native "Main" thread for the renderer / utility process.
    private Thread mSandboxMainThread;
    // Parameters received via IPC, only accessed while holding the mSandboxMainThread monitor.
    private String[] mCommandLineParams;
    private ParcelFileDescriptor mIPCFd;
    private ParcelFileDescriptor mMinidumpFd;
    private ParcelFileDescriptor mChromePakFd;
    private ParcelFileDescriptor mLocalePakFd;

    private static Context sContext = null;
    private boolean mLibraryInitialized = false;

    // Binder object used by clients for this service.
    private final ISandboxedProcessService.Stub mBinder = new ISandboxedProcessService.Stub() {
        // NOTE: Implement any ISandboxedProcessService methods here.
        @Override
        public int setupConnection(Bundle args, ISandboxedProcessCallback callback) {
            mCallback = callback;
            synchronized (mSandboxMainThread) {
                // Allow the command line to be set via bind() intent or setupConnection, but
                // the FD can only be transferred here.
                if (mCommandLineParams == null) {
                    mCommandLineParams = args.getStringArray(EXTRA_COMMAND_LINE);
                }
                // We must have received the command line by now
                assert mCommandLineParams != null;
                mIPCFd = args.getParcelable(EXTRA_IPC_FD);
                mChromePakFd = args.getParcelable(EXTRA_CHROME_PAK_FD);
                mLocalePakFd = args.getParcelable(EXTRA_LOCALE_PAK_FD);
                // mMinidumpFd may be null if native crash reporting is disabled.
                if (args.containsKey(EXTRA_MINIDUMP_FD)) {
                    mMinidumpFd = args.getParcelable(EXTRA_MINIDUMP_FD);
                }
                mSandboxMainThread.notifyAll();
            }
            return Process.myPid();
        }

        @Override
        public void setSurface(int type, Surface surface, int primaryID, int secondaryID) {
            // Calling nativeSetSurface will release the surface.
            nativeSetSurface(type, surface, primaryID, secondaryID);
        }
    };

    public static Context getContext() {
        return sContext;
    }

    @Override
    public void onCreate() {
        sContext = this;
        super.onCreate();

        mSandboxMainThread = new Thread(new Runnable() {
            @Override
            public void run()  {
                try {
                    if (!LibraryLoader.loadNow()) return;
                    synchronized (mSandboxMainThread) {
                        while (mCommandLineParams == null) {
                            mSandboxMainThread.wait();
                        }
                    }
                    LibraryLoader.initializeOnMainThread(mCommandLineParams);
                    synchronized (mSandboxMainThread) {
                        mLibraryInitialized = true;
                        mSandboxMainThread.notifyAll();
                        while (mIPCFd == null) {
                            mSandboxMainThread.wait();
                        }
                    }
                    int minidumpFd = (mMinidumpFd == null) ? -1 : mMinidumpFd.detachFd();
                    nativeSandboxedProcessMain(
                            SandboxedProcessService.this, mIPCFd.detachFd(), minidumpFd,
                            mChromePakFd.detachFd(), mLocalePakFd.detachFd());
                } catch (InterruptedException e) {
                    Log.w(TAG, MAIN_THREAD_NAME + " startup failed: " + e);
                }
            }
        }, MAIN_THREAD_NAME);
        mSandboxMainThread.start();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mCommandLineParams == null) {
            // This process was destroyed before it even started. Nothing more to do.
            return;
        }
        synchronized (mSandboxMainThread) {
            try {
                while (!mLibraryInitialized) {
                    // Avoid a potential race in calling through to native code before the library
                    // has loaded.
                    mSandboxMainThread.wait();
                }
            } catch (InterruptedException e) {
            }
        }

        // This is not synchronized with the main thread in any way, but this is analogous
        // to how desktop chrome terminates processes using SIGTERM. The mSandboxMainThread
        // may run briefly before this is executed, but will eventually get a channel error
        // and similarly commit suicide via SuicideOnChannelErrorFilter().
        // TODO(tedbo): Why doesn't the activity manager SIGTERM/SIGKILL this service process?
        nativeSandboxedProcessExit();
    }

    @Override
    public IBinder onBind(Intent intent) {
        // We call stopSelf() to request that this service be stopped as soon as the client
        // unbinds. Otherwise the system may keep it around and available for a reconnect. The
        // sandboxed processes do not currently support reconnect; they must be initialized from
        // scratch every time.
        stopSelf();

        synchronized (mSandboxMainThread) {
            mCommandLineParams = intent.getStringArrayExtra(EXTRA_COMMAND_LINE);
            mSandboxMainThread.notifyAll();
        }

        return mBinder;
    }

    /**
     * Called from native code to share a surface texture with another child process.
     * Through using the callback object the browser is used as a proxy to route the
     * call to the correct process.
     *
     * @param pid Process handle of the sandboxed process to share the SurfaceTexture with.
     * @param type The type of process that the SurfaceTexture is for.
     * @param surfaceObject The Surface or SurfaceTexture to share with the other sandboxed process.
     * @param primaryID Used to route the call to the correct client instance.
     * @param secondaryID Used to route the call to the correct client instance.
     */
    @SuppressWarnings("unused")
    @CalledByNative
    private void establishSurfaceTexturePeer(int pid, int type, Object surfaceObject, int primaryID, int secondaryID) {
        if (mCallback == null) {
            Log.e(TAG, "No callback interface has been provided.");
            return;
        }

        Surface surface = null;
        boolean needRelease = false;
        if (surfaceObject instanceof Surface) {
            surface = (Surface)surfaceObject;
        } else if (surfaceObject instanceof SurfaceTexture) {
            surface = new Surface((SurfaceTexture)surfaceObject);
            needRelease = true;
        } else {
            Log.e(TAG, "Not a valid surfaceObject: " + surfaceObject);
            return;
        }
        try {
            mCallback.establishSurfacePeer(pid, type, surface, primaryID, secondaryID);
        } catch (RemoteException e) {
            Log.e(TAG, "Unable to call establishSurfaceTexturePeer: " + e);
            return;
        } finally {
            if (needRelease) {
                surface.release();
            }
        }
    }

    /**
     * The main entry point for a sandboxed process. This should be called from a new thread since
     * it will not return until the sandboxed process exits. See sandboxed_process_service.{h,cc}

     *
     * @param ipcFd File descriptor to use for ipc.
     * @param minidumpFd File descriptor where minidumps can be written.
     */
    private static native void nativeSandboxedProcessMain(
            SandboxedProcessService service, int ipcFd, int minidumpFd, int chromePakFd,
            int localePakFd);

    /**
     * Force the sandboxed process to exit.
     */
    private static native void nativeSandboxedProcessExit();

    /**
     * Sets up the Surface iBinder for a producer identified by the IDs.
     *
     * @param type The install type for the Surface
     * @param surface The parceled Surface to set.
     * @param primaryID Used to identify the correct target instance.
     * @param secondaryID Used to identify the correct target instance.
     */
    private static native void nativeSetSurface(int type, Surface surface,
            int primaryID, int secondaryID);
}
