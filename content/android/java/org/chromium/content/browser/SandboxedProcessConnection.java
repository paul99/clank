// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.util.concurrent.atomic.AtomicBoolean;

import org.chromium.base.CalledByNative;

class SandboxedProcessConnection {
    interface DeathCallback {
        void onSandboxedProcessDied(int pid);
    }

    private final Context mContext;
    private final int mServiceNumber;  // TODO(tedbo): Remove this when we swtich to new service api.
    private final SandboxedProcessConnection.DeathCallback mDeathCallback;

    // Synchronization: While most internal flow occurs on the UI thread, the public API
    // (specifically bind and unbind) may be called from any thread, hence all entry point methods
    // into the class are synchronized on the SandboxedProcessConnection instance to protect access
    // to these members. But see also the TODO where AsyncBoundServiceConnection is created.
    private ISandboxedProcessService mService = null;
    private boolean mServiceConnectComplete = false;
    private int mPID = 0;  // Process ID of the corresponding sandboxed process.
    private HighPriorityConnection mHighPriorityConnection = null;
    private int mHighPriorityConnectionCount = 0;

    private static final String TAG = "SandboxedProcessConnection";

    private static class ConnectionParams {
        final String[] mCommandLine;
        final int mIpcFd;
        final int mCrashFd;
        final ISandboxedProcessCallback mCallback;
        final Runnable mOnConnectionCallback;

        ConnectionParams(
                String[] commandLine,
                int ipcFd,
                int crashFd,
                ISandboxedProcessCallback callback,
                Runnable onConnectionCallback) {
            mCommandLine = commandLine;
            mIpcFd = ipcFd;
            mCrashFd = crashFd;
            mCallback = callback;
            mOnConnectionCallback = onConnectionCallback;
        }
    };

    // Implement the ServiceConnection as an inner class, so it can stem the service
    // callbacks when cancelled or unbound.
    class AsyncBoundServiceConnection extends AsyncTask<Intent, Void, Boolean>
            implements ServiceConnection {
        private boolean mIsDestroyed = false;
        private AtomicBoolean mIsBound = new AtomicBoolean(false);

        // AsyncTask
        @Override
        protected Boolean doInBackground(Intent... intents) {
            boolean isBound = mContext.bindService(intents[0], this, Context.BIND_AUTO_CREATE);
            mIsBound.set(isBound);
            return isBound;
        }

        @Override
        protected void onPostExecute(Boolean boundOK) {
            synchronized (SandboxedProcessConnection.this) {
                if (!boundOK && !mIsDestroyed) {
                    SandboxedProcessConnection.this.onBindFailed();
                }
                // else: bind will complete asynchronously with a callback to onServiceConnected().
            }
        }

        @Override
        protected void onCancelled(Boolean boundOK) {
            // According to {@link AsyncTask#onCancelled(Object)}, the Object can be null.
            if (boundOK != null && boundOK) {
                unBindIfAble();
            }
        }

        /**
         * Unbinds this connection if it hasn't already been unbound. There's a guard to check that
         * we haven't already been unbound because the Browser process cancelling a connection can
         * race with something else (Android?) cancelling the connection.
         */
        private void unBindIfAble() {
            if (mIsBound.getAndSet(false)) {
                mContext.unbindService(this);
            }
        }

        // ServiceConnection
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            synchronized (SandboxedProcessConnection.this) {
                if (!mIsDestroyed) {
                    SandboxedProcessConnection.this.onServiceConnected(name, service);
                }
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            synchronized (SandboxedProcessConnection.this) {
                if (!mIsDestroyed) {
                    SandboxedProcessConnection.this.onServiceDisconnected(name);
                }
            }
        }

        public void destroy() {
            assert Thread.holdsLock(SandboxedProcessConnection.this);
            if (!cancel(false)) {
                unBindIfAble();
            }
            mIsDestroyed = true;
        }
    }

    // This is only valid while the connection is being established.
    private ConnectionParams mConnectionParams;
    private AsyncBoundServiceConnection mServiceConnection;

    SandboxedProcessConnection(Context context, int number,
            SandboxedProcessConnection.DeathCallback deathCallback) {
        mContext = context;
        mServiceNumber = number;
        mDeathCallback = deathCallback;
    }

    // TODO(tedbo): Remove when we have the sandboxed service api.
    int getServiceNumber() {
        return mServiceNumber;
    }

    synchronized ISandboxedProcessService getService() {
        return mService;
    }

    /**
     * Bind to an ISandboxedProcessService. This must be followed by a call to setupConnection()
     * to setup the connection parameters. (These methods are separated to allow the client
     * to pass whatever parameters they have available here, and complete the remainder
     * later while reducing the connection setup latency).
     *
     * @param commandLine (Optional) Command line for the sandboxed process. If omitted, then
     * the command line parameters must instead be passed to setupConnection().
     */
    synchronized void bind(String[] commandLine) {
        TraceEvent.begin();
        final Intent intent = new Intent(
                "org.chromium.content.browser.ISandboxedProcessService" + mServiceNumber);
        intent.setPackage(mContext.getPackageName());
        if (commandLine != null) {
            intent.putExtra(SandboxedProcessService.EXTRA_COMMAND_LINE, commandLine);
        }
        // TODO(joth): By the docs, AsyncTasks should only be created on the UI thread, but
        // bind() currently 'may' be called on any thread. In practice it's only ever called
        // from UI, but it's not guaranteed. See http://b/5694925.
        assert Looper.getMainLooper() == Looper.myLooper();
        // On completion this will call back to onServiceConnected().
        mServiceConnection = new AsyncBoundServiceConnection();
        mServiceConnection.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, intent);
        TraceEvent.end();
    }

    /** Setup a connection previous bound via a call to bind().
     *
     * This establishes the parameters that were not already supplied in bind.
     * @param commandLine (Optional) will be ignored if the command line was already sent in bind()
     * @param ipcFd The file descriptor that will be used by the sandbox process for IPC.
     * @param crashFd (Optional) file descriptor that will be used for crash dumps.
     * @param callback Used for status updates regarding this process connection.
     * @param onConnectionCallback will be run when the connection is setup and ready to use.
     */
    synchronized void setupConnection(
            String[] commandLine,
            int ipcFd,
            int crashFd,
            ISandboxedProcessCallback callback,
            Runnable onConnectionCallback) {
        TraceEvent.begin();
        assert mConnectionParams == null;
        mConnectionParams = new ConnectionParams(commandLine, ipcFd, crashFd, callback,
                onConnectionCallback);
        if (mServiceConnectComplete) {
            doConnectionSetup();
        }
        TraceEvent.end();
    }

    /**
     * Unbind the ISandboxedProcessService. It is safe to call this multiple times.
     */
    synchronized void unbind() {
        if (mServiceConnection != null) {
            mServiceConnection.destroy();
            mServiceConnection = null;
        }
        if (mService != null) {
            if (mHighPriorityConnection != null) {
                unbindHighPriority(true);
            }
            mService = null;
            mPID = 0;
        }
        mConnectionParams = null;
        mServiceConnectComplete = false;
    }

    // Called on the main thread to notify that the service is connected.
    private void onServiceConnected(ComponentName className, IBinder service) {
        assert Thread.holdsLock(this);
        TraceEvent.begin();
        mServiceConnectComplete = true;
        mService = ISandboxedProcessService.Stub.asInterface(service);
        if (mConnectionParams != null) {
            doConnectionSetup();
        }
        TraceEvent.end();
    }

    // Called on the main thread to notify that the bindService() call failed (returned false).
    private void onBindFailed() {
        assert Thread.holdsLock(this);
        mServiceConnectComplete = true;
        if (mConnectionParams != null) {
            doConnectionSetup();
        }
    }

    /**
     * Called when the connection parameters have been set, and a connection has been established
     * (as signaled by onServiceConnected), or if the connection failed (mService will be false).
     */
    private void doConnectionSetup() {
        TraceEvent.begin();
        assert mServiceConnectComplete && mConnectionParams != null;
        // Capture the callback before it is potentially nulled in unbind().
        Runnable onConnectionCallback =
            mConnectionParams != null ? mConnectionParams.mOnConnectionCallback : null;
        if (onConnectionCallback == null) {
            unbind();
        } else if (mService != null) {
            try {
                ParcelFileDescriptor ipcFdParcel =
                    ParcelFileDescriptor.fromFd(mConnectionParams.mIpcFd);
                Bundle bundle = new Bundle();
                bundle.putStringArray(SandboxedProcessService.EXTRA_COMMAND_LINE,
                        mConnectionParams.mCommandLine);
                bundle.putParcelable(SandboxedProcessService.EXTRA_IPC_FD, ipcFdParcel);

                try {
                    ParcelFileDescriptor crashFdParcel =
                        ParcelFileDescriptor.fromFd(mConnectionParams.mCrashFd);
                    bundle.putParcelable(SandboxedProcessService.EXTRA_CRASH_FD, crashFdParcel);
                  // We will let the GC close the crash ParcelFileDescriptor.
                } catch (java.io.IOException e) {
                    Log.w(TAG, "Invalid crash Fd. Native crash reporting will be disabled.");
                }

                mPID = mService.setupConnection(bundle, mConnectionParams.mCallback);
                ipcFdParcel.close();  // We proactivley close now rather than wait for GC & finalizer.
            } catch(java.io.IOException e) {
                Log.w(TAG, "Invalid ipc FD.");
            } catch(android.os.RemoteException e) {
                Log.w(TAG, "Exception when trying to call service method: "
                        + e);
            }
        }
        mConnectionParams = null;
        if (onConnectionCallback != null) {
            onConnectionCallback.run();
        }
        TraceEvent.end();
    }

    // Called on the main thread to notify that the sandboxed service did not disconnect gracefully.
    private void onServiceDisconnected(ComponentName className) {
        assert Thread.holdsLock(this);
        int pid = mPID;  // Stash pid & connection callback since unbind() will clear them.
        Runnable onConnectionCallback =
            mConnectionParams != null ? mConnectionParams.mOnConnectionCallback : null;
        Log.w(TAG, "onServiceDisconnected (crash?): pid=" + pid);
        unbind();  // We don't want to auto-restart on crash. Let the browser do that.
        if (pid != 0) {
            mDeathCallback.onSandboxedProcessDied(pid);
        }
        if (onConnectionCallback != null) {
            onConnectionCallback.run();
        }
    }

    /**
     * Bind the service with a new high priority connection. This will make the service
     * as important as the main process.
     */
    synchronized void bindHighPriority() {
        if (mService == null) {
            Log.w(TAG, "The connection is not bound for " + mPID);
            return;
        }
        if (mHighPriorityConnection == null) {
            mHighPriorityConnection = new HighPriorityConnection();
            mHighPriorityConnection.bind();
        }
        mHighPriorityConnectionCount++;
    }

    /**
     * Unbind the service as the high priority connection.
     */
    synchronized void unbindHighPriority(boolean force) {
        if (mService == null) {
            Log.w(TAG, "The connection is not bound for " + mPID);
            return;
        }
        mHighPriorityConnectionCount--;
        if (force || (mHighPriorityConnectionCount == 0 && mHighPriorityConnection != null)) {
            mHighPriorityConnection.unbind();
            mHighPriorityConnection = null;
        }
    }

    private class HighPriorityConnection implements ServiceConnection {

        private boolean mHBound = false;

        void bind() {
            Intent intent = new Intent(
                    "org.chromium.content.browser.ISandboxedProcessService" + mServiceNumber);
            intent.setPackage(mContext.getPackageName());
            mHBound = mContext.bindService(intent, this,
                    Context.BIND_AUTO_CREATE | Context.BIND_IMPORTANT);
        }

        void unbind() {
            if (mHBound) {
                mContext.unbindService(this);
                mHBound = false;
            }
        }

        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
        }
    }

    /**
     * @return The connection PID, or 0 if not yet connected.
     */
    synchronized public int getPid() {
        return mPID;
    }
}
