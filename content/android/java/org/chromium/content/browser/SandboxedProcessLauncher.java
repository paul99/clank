// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.os.RemoteException;
import android.util.Log;
import android.view.Surface;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.chromium.base.CalledByNative;

/**
 * This class provides the method to start/stop SandboxedProcess called by
 * native.
 */
class SandboxedProcessLauncher {
    private static String TAG = "SandboxedProcessLauncher";

    // TODO(joth): The following code is a temporary hack to allow for more than one sandboxed
    // process. We currently register multiple SandboxedProcessService subclasses.
    // (If we retain this, we could consider using an array of weak references and avoid
    // the need for the explicit freeConnection() method).
    private static final int NUM_REGISTERED_SERVICES = 5;
    private static SandboxedProcessConnection[] mConnections =
            new SandboxedProcessConnection[NUM_REGISTERED_SERVICES];

    private static SandboxedProcessConnection allocateConnection(Context context) {
        SandboxedProcessConnection.DeathCallback deathCallback =
            new SandboxedProcessConnection.DeathCallback() {
                @Override
                public void onSandboxedProcessDied(int pid) {
                    stop(pid);
                }
            };
        synchronized (mConnections) {
            for (int i = 0; i < NUM_REGISTERED_SERVICES; ++i) {
                if (mConnections[i] == null) {
                    mConnections[i] = new SandboxedProcessConnection(context, i, deathCallback);
                    return mConnections[i];
                }
            }
        }
        Log.w(TAG, "Ran out of sandboxed services.");
        return null;
    }

    private static SandboxedProcessConnection allocateBoundConnection(Context context,
            String[] commandLine) {
        SandboxedProcessConnection connection = allocateConnection(context);
        if (connection != null) {
            connection.bind(commandLine);
        }
        return connection;
    }

    private static void freeConnection(SandboxedProcessConnection connection) {
        if (connection == null) {
            return;
        }
        int slot = connection.getServiceNumber();
        synchronized (mConnections) {
            if (mConnections[slot] != connection) {
                int occupier = mConnections[slot] == null ?
                        -1 : mConnections[slot].getServiceNumber();
                Log.e(TAG, "Unable to find connection to free in slot: " + slot +
                        " already occupied by service: " + occupier);
                assert false;
            } else {
                mConnections[slot] = null;
            }
        }
    }
    //  TODO(joth): ----------------- END HACK --------------------

    // Represents an invalid process handle; same as base/process.h kNullProcessHandle.
    private static final int NULL_PROCESS_HANDLE = 0;

    // Map from pid to SandboxedService connection.
    private static Map<Integer, SandboxedProcessConnection> mServiceMap =
            new ConcurrentHashMap<Integer, SandboxedProcessConnection>();

    // A pre-allocated and pre-bound connection ready for connection setup, or null.
    static SandboxedProcessConnection mSpareConnection = null;

    /**
     * Returns the sandboxed process service interface for the given pid. This may be called on
     * any thread, but the caller must assume that the service can disconnect at any time. All
     * service calls should catch and handle android.os.RemoteException.
     *
     * @param pid The pid (process handle) of the service obtained from {@link #start}.
     * @return The ISandboxedProcessService or null if the service no longer exists.
     */
    public static ISandboxedProcessService getSandboxedService(int pid) {
        SandboxedProcessConnection connection = mServiceMap.get(pid);
        if (connection != null) {
            return connection.getService();
        }
        return null;
    }

    /**
     * Should be called early in startup so the work needed to spawn the sandboxed process can
     * be done in parallel to other startup work.
     * @param context the application context used for the connection.
     */
    public static synchronized void warmUp(Context context) {
        if (mSpareConnection == null) {
            mSpareConnection = allocateBoundConnection(context, null);
        }
    }

    /**
     * Spawns and connects to a sandboxed process. May be called on any thread. It will not
     * block, but will instead callback to {@link #nativeOnSandboxedProcessStarted} when the
     * connection is established. Note this callback will not necessarily be from the same thread
     * (currently it always comes from the main thread).
     *
     * @param context Context used to obtain the application context.
     * @param commandLine The sandboxed process command line argv.
     * @param ipcFd File descriptor used to set up IPC.
     * @param clientContext Arbitrary parameter used by the client to distinguish this connection.
     * @return Connection object which maybe used with subsequent call to {@link #cancelStart}
     */
    @CalledByNative
    static SandboxedProcessConnection start(
            Context context,
            final String[] commandLine,
            int ipcFd,
            int crashFd,
            final int clientContext) {
        assert clientContext != 0;
        SandboxedProcessConnection allocatedConnection;
        synchronized (SandboxedProcessLauncher.class) {
            allocatedConnection = mSpareConnection;
            mSpareConnection = null;
        }
        if (allocatedConnection == null) {
            allocatedConnection = allocateBoundConnection(context, commandLine);
            if (allocatedConnection == null) {
                return null;
            }
        }
        final SandboxedProcessConnection connection = allocatedConnection;
        Log.d(TAG, "Setting up connection to process: slot=" + connection.getServiceNumber());
        // Note: This runnable will be executed when the sandboxed connection is setup.
        final Runnable onConnect = new Runnable() {
            @Override
            public void run() {
                final int pid = connection.getPid();
                Log.d(TAG, "on connect callback, pid=" + pid + " context=" + clientContext);
                if (pid != NULL_PROCESS_HANDLE) {
                    mServiceMap.put(pid, connection);
                } else {
                    freeConnection(connection);
                }
                nativeOnSandboxedProcessStarted(clientContext, pid);
            }
        };
        connection.setupConnection(commandLine, ipcFd, crashFd, createCallback(), onConnect);
        return connection;
    }

    /**
     * Cancels a pending connection to a sandboxed process. This may be called from any thread.
     *
     * @param connection the object that was returned from the corresponding call to {@link #start}.
     */
    @CalledByNative
    static void cancelStart(SandboxedProcessConnection connection) {
        assert connection != null;
        assert !mServiceMap.containsValue(connection);
        connection.unbind();
        freeConnection(connection);
    }

    /**
     * Terminates a sandboxed process. This may be called from any thread.
     *
     * @param pid The pid (process handle) of the service connection obtained from {@link #start}.
     */
    @CalledByNative
    static void stop(int pid) {
        Log.d(TAG, "stopping sandboxed connection: pid=" + pid);

        SandboxedProcessConnection connection = mServiceMap.remove(pid);
        if (connection == null) {
            Log.w(TAG, "Tried to stop non-existent connection to pid: " + pid);
            return;
        }
        connection.unbind();
        freeConnection(connection);
    }

    /**
     * Bind a sandboxed process as a high priority process so that it has the same
     * priority as the main process. This can be used for the foreground renderer
     * process to distinguish it from the the background renderer process.
     *
     * @param pid The process handle of the service connection obtained from {@link #start}.
     */
    static void bindAsHighPriority(int pid) {
        SandboxedProcessConnection connection = mServiceMap.get(pid);
        if (connection == null) {
            Log.w(TAG, "Tried to bind a non-existent connection to pid: " + pid);
            return;
        }
        connection.bindHighPriority();
    }

    /**
     * Unbind a high priority process which is bound by {@link #bindAsHighPriority}.
     *
     * @param pid The process handle of the service obtained from {@link #start}.
     */
    static void unbindAsHighPriority(int pid) {
        SandboxedProcessConnection connection = mServiceMap.get(pid);
        if (connection == null) {
            Log.w(TAG, "Tried to unbind non-existent connection to pid: " + pid);
            return;
        }
        connection.unbindHighPriority(false);
    }

    static void establishSurfacePeer(
            int pid, int type, Surface surface, int primaryID, int secondaryID) {
        Log.d(TAG, "establishSurfaceTexturePeer: pid = " + pid + ", " +
              "type = " + type + ", " +
              "primaryID = " + primaryID + ", " +
              "secondaryID = " + secondaryID);
        ISandboxedProcessService service = SandboxedProcessLauncher.getSandboxedService(pid);
        if (service == null) {
            Log.e(TAG, "Unable to get SandboxedProcessService from pid.");
            return;
        }
        try {
            service.setSurface(type, surface, primaryID, secondaryID);
        } catch (RemoteException e) {
            Log.e(TAG, "Unable to call setSurface: " + e);
        }
    }

    /**
     * This implementation is used to receive callbacks from the remote service.
     */
    private static ISandboxedProcessCallback createCallback() {
        return new ISandboxedProcessCallback.Stub() {
            /**
             * This is called by the remote service regularly to tell us about
             * new values.  Note that IPC calls are dispatched through a thread
             * pool running in each process, so the code executing here will
             * NOT be running in our main thread -- so, to update the UI, we need
             * to use a Handler.
             */
            public void establishSurfacePeer(
                    int pid, int type, Surface surface, int primaryID, int secondaryID) {
                SandboxedProcessLauncher.establishSurfacePeer(pid, type, surface,
                        primaryID, secondaryID);
                // The SandboxProcessService now holds a reference to the
                // Surface's resources, so we release our reference to it now to
                // avoid waiting for the finalizer to get around to it.
                if (surface != null) {
                    surface.release();
                }
            }
        };
    };

    private static native void nativeOnSandboxedProcessStarted(int clientContext, int pid);
}
