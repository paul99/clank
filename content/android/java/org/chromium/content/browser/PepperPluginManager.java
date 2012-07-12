// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.util.Log;

public class PepperPluginManager {

    private static final String LOGTAG = "PepperPluginManager";

    /**
     * Service Action: A plugin wishes to be loaded in the ChromeView must
     * provide {@link android.content.IntentFilter IntentFilter} that accepts
     * this action in its AndroidManifest.xml.
     */
    // android.annotation.SdkConstant is only for framework.
    // TODO(wangxianzhu): Should uncomment the following line when chromeview enters framework.
    // @SdkConstant(SdkConstantType.SERVICE_ACTION)
    public static final String PEPPER_PLUGIN_ACTION = "android.chrome.PEPPERPLUGIN";

    // A plugin will specify the following fields in its AndroidManifest.xml.
    private static final String FILENAME = "filename";
    private static final String MIMETYPE = "mimetype";
    private static final String NAME = "name";
    private static final String DESCRIPTION = "description";
    private static final String VERSION = "version";
    private static final String FLASH_MIME_TYPE = "application/x-shockwave-flash";

    public static String getPlugins(final Context context) {
        StringBuffer ret = new StringBuffer();
        String flash = new String();
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> plugins = pm.queryIntentServices(new Intent(
                PEPPER_PLUGIN_ACTION), PackageManager.GET_SERVICES
                | PackageManager.GET_META_DATA);
        for (ResolveInfo info : plugins) {
            // Retrieve the plugin's service information.
            ServiceInfo serviceInfo = info.serviceInfo;
            if (serviceInfo == null) {
                Log.e(LOGTAG, "Ignore bad plugin");
                continue;
            }
            if (serviceInfo.metaData == null) {
                Log.e(LOGTAG, serviceInfo.name + " misses the required metadata");
                continue;
            }
            // Retrieve the plugin's package information.
            PackageInfo pkgInfo;
            try {
                pkgInfo = pm.getPackageInfo(serviceInfo.packageName, 0);
            } catch (NameNotFoundException e) {
                Log.e(LOGTAG, "Can't find plugin: " + serviceInfo.packageName);
                continue;
            }
            if (pkgInfo == null) {
                continue;
            }
            // Find the name of the plugin's shared library.
            String filename = serviceInfo.metaData.getString(FILENAME);
            if (filename == null || filename.isEmpty()) {
                continue;
            }
            // Find the mimetype of the plugin. Flash is handled in getFlashPath.
            String mimetype = serviceInfo.metaData.getString(MIMETYPE);
            if (mimetype == null || mimetype.isEmpty()) {
                continue;
            }
            // Assemble the plugin info, according to the format described in
            // pepper_plugin_registry.cc.
            // (eg. path<#name><#description><#version>;mimetype)
            StringBuffer plugin = new StringBuffer();
            plugin.append(pkgInfo.applicationInfo.dataDir);
            plugin.append("/lib/");
            plugin.append(filename);

            // Find the (optional) name/description/version of the plugin.
            String name = serviceInfo.metaData.getString(NAME);
            String description = serviceInfo.metaData.getString(DESCRIPTION);
            String version = serviceInfo.metaData.getString(VERSION);

            if (name != null && !name.isEmpty()) {
                plugin.append("#");
                plugin.append(name);
                if (description != null && !description.isEmpty()) {
                    plugin.append("#");
                    plugin.append(description);
                    if (version != null && !version.isEmpty()) {
                        plugin.append("#");
                        plugin.append(version);
                    }
                }
            }
            plugin.append(';');
            plugin.append(mimetype);

            if (mimetype.equals(FLASH_MIME_TYPE)) {
                flash = plugin.toString();
            } else {
                // Use comma separator if there is more than one plugin.
                if (ret.length() > 0)
                    ret.append(',');
                ret.append(plugin.toString());
            }
        }
        ret.append("|");
        ret.append(flash.toString());
        return ret.toString();
    }

}
