// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.net;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.CalledByNativeUnchecked;

import java.io.ByteArrayInputStream;
import java.net.URLConnection;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;

import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

// This class implements most of the Java code required by the net component.
class AndroidNetworkLibrary {
    private static final String TAG = "AndroidNetworkLibrary";

    // Stores the key pair into the CertInstaller application.
    @CalledByNative
    static public boolean storeKeyPair(Context context, byte[] public_key, byte[] private_key) {
        // This is based on android.security.Credentials.install()
        // TODO(joth): Use KeyChain API instead of hard-coding constants here: http://b/5859651
        try {
            Intent intent = new Intent("android.credentials.INSTALL");
            intent.setClassName("com.android.certinstaller",
                    "com.android.certinstaller.CertInstallerMain");
            intent.putExtra("KEY", private_key);
            intent.putExtra("PKEY", public_key);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
            return true;
        } catch (ActivityNotFoundException e) {
            Log.w(TAG, "could not store certificate: " + e);
        }
        return false;
    }

    // Get the mime type (if any) that is associated with the file extension.
    // Returns null if no corresponding mime type exists.
    @CalledByNative
    static public String getMimeTypeFromExtension(String extension) {
        return URLConnection.guessContentTypeFromName("foo." + extension);
    }

    /**
     * Validate the server's certificate chain is trusted.
     * @param certChain The bytes for certificates, typically in ASN.1 DER encoded
     * certificates format.
     * @param authType The authentication type for the cert chain
     * @return true if the server is trusted
     * @throws CertificateException,KeyStoreException,NoSuchAlgorithmException on error
     * initializing the TrustManager or reading the certChain
     */
    @CalledByNativeUnchecked
    public static boolean verifyServerCertificates(byte[][] certChain, String authType)
            throws CertificateException, KeyStoreException, NoSuchAlgorithmException {
        if (certChain == null || certChain.length == 0 || certChain[0] == null) {
            throw new IllegalArgumentException("bad certificate chain");
        }

        ensureInitialized();
        X509Certificate[] serverCertificates = new X509Certificate[certChain.length];
        for (int i = 0; i < certChain.length; ++i) {
            serverCertificates[i] =
                (X509Certificate) sCertificateFactory.generateCertificate(
                        new ByteArrayInputStream(certChain[i]));
        }

        try {
            sDefaultTrustManager.checkServerTrusted(serverCertificates, authType);
            return true;
        } catch (CertificateException e) {
            Log.i(TAG, "failed to validate the certificate chain, error: " +
                    e.getMessage());
        }
        return false;
    }

    // Default sources of authentication trust decisions and certificate object creation.
    private static X509TrustManager sDefaultTrustManager;
    private static CertificateFactory sCertificateFactory;

    private static synchronized void ensureInitialized()
            throws CertificateException, KeyStoreException, NoSuchAlgorithmException {
        if (sDefaultTrustManager == null) {
            sDefaultTrustManager = createDefaultTrustManager();
        }
        if (sCertificateFactory == null) {
            sCertificateFactory = CertificateFactory.getInstance("X.509");
        }
    }

    private static X509TrustManager createDefaultTrustManager()
            throws KeyStoreException, NoSuchAlgorithmException {
        String algorithm = TrustManagerFactory.getDefaultAlgorithm();
        TrustManagerFactory tmf = TrustManagerFactory.getInstance(algorithm);
        tmf.init((KeyStore) null);
        TrustManager[] tms = tmf.getTrustManagers();
        X509TrustManager trustManager = findX509TrustManager(tms);
        return trustManager;
    }

    private static X509TrustManager findX509TrustManager(TrustManager[] tms) {
        for (TrustManager tm : tms) {
            if (tm instanceof X509TrustManager) {
                return (X509TrustManager)tm;
            }
        }
        return null;
    }
}
