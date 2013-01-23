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
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.util.Log;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;

import org.chromium.base.CalledByNative;

class SandboxedProcessConnection implements ServiceConnection {
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
        final int mMinidumpFd;
        final int mChromePakFd;
        final int mLocalePakFd;
        final ISandboxedProcessCallback mCallback;
        final Runnable mOnConnectionCallback;

        ConnectionParams(
                String[] commandLine,
                int ipcFd,
                int minidumpFd,
                int chromePakFd,
                int localePakFd,
                ISandboxedProcessCallback callback,
                Runnable onConnectionCallback) {
            mCommandLine = commandLine;
            mIpcFd = ipcFd;
            mMinidumpFd = minidumpFd;
            mChromePakFd = chromePakFd;
            mLocalePakFd = localePakFd;
            mCallback = callback;
            mOnConnectionCallback = onConnectionCallback;
        }
    }

    // This is only valid while the connection is being established.
    private ConnectionParams mConnectionParams;
    private boolean mIsBound;

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
        assert !ThreadUtils.runningOnUiThread();
        final Intent intent = new Intent(
                "org.chromium.content.browser.ISandboxedProcessService" + mServiceNumber);
        intent.setPackage(mContext.getPackageName());
        if (commandLine != null) {
            intent.putExtra(SandboxedProcessService.EXTRA_COMMAND_LINE, commandLine);
        }

        mIsBound = mContext.bindService(intent, this, Context.BIND_AUTO_CREATE);
        if (!mIsBound) {
            onBindFailed();
        }
        TraceEvent.end();
    }

    /** Setup a connection previous bound via a call to bind().
     *
     * This establishes the parameters that were not already supplied in bind.
     * @param commandLine (Optional) will be ignored if the command line was already sent in bind()
     * @param ipcFd The file descriptor that will be used by the sandbox process for IPC.
     * @param minidumpFd (Optional) file descriptor that will be used for crash dumps.
     * @param callback Used for status updates regarding this process connection.
     * @param onConnectionCallback will be run when the connection is setup and ready to use.
     */
    synchronized void setupConnection(
            String[] commandLine,
            int ipcFd,
            int minidumpFd,
            int chromePakFd,
            int localePakFd,
            ISandboxedProcessCallback callback,
            Runnable onConnectionCallback) {
        TraceEvent.begin();
        assert mConnectionParams == null;
        mConnectionParams = new ConnectionParams(commandLine, ipcFd, minidumpFd, chromePakFd,
                localePakFd, callback, onConnectionCallback);
        if (mServiceConnectComplete) {
            doConnectionSetup();
        }
        TraceEvent.end();
    }

    /**
     * Unbind the ISandboxedProcessService. It is safe to call this multiple times.
     */
    synchronized void unbind() {
        if (mIsBound) {
            mContext.unbindService(this);
            mIsBound = false;
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
    @Override
    public void onServiceConnected(ComponentName className, IBinder service) {
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
            ParcelFileDescriptor ipcFdParcel = null;
            ParcelFileDescriptor chromePakFdParcel = null;
            ParcelFileDescriptor localePakFdParcel = null;
            ParcelFileDescriptor minidumpFdParcel = null;

            // We create all Parcels even if some FDs are invalid to make sure all valid FDs get
            // closed.
            if (mConnectionParams.mIpcFd != -1) {
                // The IPC FD is owned by the native ChildProcessLauncher::Context(), so we use
                // fromFD (instead of adoptFD) which duplicates the FD.
                try {
                    ipcFdParcel = ParcelFileDescriptor.fromFd(mConnectionParams.mIpcFd);
                } catch (IOException ioe) {
                    Log.e(TAG, "Failed to start a new renderer process, invalid IPC FD provided.",
                          ioe);
                }
            }

            if (mConnectionParams.mChromePakFd != -1) {
                chromePakFdParcel = ParcelFileDescriptor.adoptFd(mConnectionParams.mChromePakFd);
            }

            if (mConnectionParams.mLocalePakFd != -1) {
                localePakFdParcel = ParcelFileDescriptor.adoptFd(mConnectionParams.mLocalePakFd);
            }

            if (mConnectionParams.mMinidumpFd != -1) {
                // The minidump FD is owned by the browser process and closed when the render
                // process terminates.
                try {
                    minidumpFdParcel = ParcelFileDescriptor.fromFd(mConnectionParams.mMinidumpFd);
                }  catch (IOException ioe) {
                    Log.e(TAG, "Failed to create parcel from minidump FD. Crash reporting will "
                            + "be disabled.", ioe);
                }
            } else {
                Log.w(TAG, "No minidump FD provided, native crash reporting is disabled.");
            }

            if (ipcFdParcel != null && chromePakFdParcel != null && localePakFdParcel != null) {
                Bundle bundle = new Bundle();
                bundle.putStringArray(SandboxedProcessService.EXTRA_COMMAND_LINE,
                                      mConnectionParams.mCommandLine);
                bundle.putParcelable(SandboxedProcessService.EXTRA_IPC_FD, ipcFdParcel);
                bundle.putParcelable(SandboxedProcessService.EXTRA_CHROME_PAK_FD,
                                     chromePakFdParcel);
                bundle.putParcelable(SandboxedProcessService.EXTRA_LOCALE_PAK_FD,
                                     localePakFdParcel);
                if (minidumpFdParcel != null) {
                    bundle.putParcelable(SandboxedProcessService.EXTRA_MINIDUMP_FD,
                                         minidumpFdParcel);
                }
                try {
                    mPID = mService.setupConnection(bundle, mConnectionParams.mCallback);
                } catch(RemoteException e) {
                    Log.w(TAG, "Exception when trying to call service method.", e);
                }
            } else {
                Log.e(TAG, "Failed to start a new renderer process, invalid FD(s) provided: "
                      + " ipc=" + mConnectionParams.mIpcFd
                      + " chromePack=" + mConnectionParams.mChromePakFd
                      + " localePak=" + mConnectionParams.mLocalePakFd);
            }

            try {
                // We proactivley close now rather than wait for GC & finalizer.
                if (ipcFdParcel != null) ipcFdParcel.close();
                if (chromePakFdParcel != null) chromePakFdParcel.close();
                if (localePakFdParcel != null) localePakFdParcel.close();
                if (minidumpFdParcel != null) minidumpFdParcel.close();
            } catch (IOException ioe) {
                Log.e(TAG, "Failed to close FD when starting renderer process.", ioe);
            }
        }
        mConnectionParams = null;
        if (onConnectionCallback != null) {
            onConnectionCallback.run();
        }
        TraceEvent.end();
    }

    // Called on the main thread to notify that the sandboxed service did not disconnect gracefully.
    @Override
    public void onServiceDisconnected(ComponentName className) {
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
