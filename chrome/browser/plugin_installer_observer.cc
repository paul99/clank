// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugin_installer_observer.h"

#include "chrome/browser/plugin_installer.h"

PluginInstallerObserver::PluginInstallerObserver(PluginInstaller* installer)
    : installer_(installer) {
  installer->AddObserver(this);
}

PluginInstallerObserver::~PluginInstallerObserver() {
  installer_->RemoveObserver(this);
}

void PluginInstallerObserver::DidStartDownload() {
}

void PluginInstallerObserver::DidFinishDownload() {
}

void PluginInstallerObserver::DownloadError(const std::string& message) {
}

WeakPluginInstallerObserver::WeakPluginInstallerObserver(
    PluginInstaller* installer) : PluginInstallerObserver(installer) {
  installer->AddWeakObserver(this);
}

WeakPluginInstallerObserver::~WeakPluginInstallerObserver() {
  installer()->RemoveWeakObserver(this);
}

void WeakPluginInstallerObserver::OnlyWeakObserversLeft() {
}
