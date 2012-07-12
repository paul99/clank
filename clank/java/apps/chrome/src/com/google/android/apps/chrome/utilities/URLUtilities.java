// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.utilities;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Collections;
import java.util.Set;
import java.util.regex.Pattern;

public class URLUtilities {

    /**
     * URLs that can be handled by ChromeView
     */
    public static final Pattern ACCEPTED_URI_SCHEMA = Pattern.compile(
            "(?i)" + // switch on case insensitive matching
            "(" +    // begin group for schema
            "(?:http|https|file):\\/\\/" +
            "|(?:inline|data|about|javascript):" +
            ")" +
            "(.*)" );

    /**
     * These are URI schemes that Chrome can download.
     */
    public static final Set<String> DOWNLOADABLE_SCHEMES =
            Collections.unmodifiableSet(CollectionsUtil.newHashSet("http", "https"));

    /**
     * This function will get a simplified version of the url for pulling up
     * existing ChromeViews for a certain url. Currently it simply returns the
     * host.
     */
    public static String getBaseUrl(String fullUrl) {
        URL url;
        try {
            url = new URL(fullUrl);
        } catch (MalformedURLException e) {
            return null;
        }
        return url.getHost();
    }

    /**
     * Determine if the two URLs are for the same page. Currently it strips the
     * hash location and then does a string comparison.
     */
    public static boolean samePage(String urlA, String urlB) {
        if (urlA == null || urlB == null) return false;
        String base1 = urlA.split("#")[0];
        String base2 = urlB.split("#")[0];
        return (base1.equals(base2));
    }

    /**
    * This function will fix up the url typed to try to make it a valid url for
    * loading in a ChromeView.
    *
    * @param url
    */
    public static String fixUrl(String url) {
        // todo(feldstein): This needs to be beefed up and
        // ensure it still supports about:crash et al.
        if (url.indexOf(":") == -1)
            url = "http://" + url;
        return url;
    }

    public static String streamToString(InputStream is) throws IOException {
        StringBuffer out = new StringBuffer();
        byte[] b = new byte[4096];
        for (int n; (n = is.read(b)) != -1;) {
            out.append(new String(b, 0, n));
        }
        return out.toString();
    }

    public static String getDomainAndRegistry(String url) {
        return nativeGetDomainAndRegistry(url);
    }

    private static native String nativeGetDomainAndRegistry(String url);
}
