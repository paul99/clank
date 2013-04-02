// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/app_list/app_list_controller_ash.h"

#include "ash/shell.h"
#include "chrome/browser/ui/app_list/app_list_util.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"

AppListControllerDelegateAsh::AppListControllerDelegateAsh() {}

AppListControllerDelegateAsh::~AppListControllerDelegateAsh() {}

void AppListControllerDelegateAsh::DismissView() {
  DCHECK(ash::Shell::HasInstance());
  if (ash::Shell::GetInstance()->GetAppListTargetVisibility())
    ash::Shell::GetInstance()->ToggleAppList(NULL);
}

gfx::NativeWindow AppListControllerDelegateAsh::GetAppListWindow() {
  DCHECK(ash::Shell::HasInstance());
  return ash::Shell::GetInstance()->GetAppListWindow();
}

bool AppListControllerDelegateAsh::IsAppPinned(
    const std::string& extension_id) {
  return ChromeLauncherController::instance()->IsAppPinned(extension_id);
}

void AppListControllerDelegateAsh::PinApp(const std::string& extension_id) {
  ChromeLauncherController::instance()->PinAppWithID(extension_id);
}

void AppListControllerDelegateAsh::UnpinApp(const std::string& extension_id) {
  ChromeLauncherController::instance()->UnpinAppsWithID(extension_id);
}

bool AppListControllerDelegateAsh::CanPin() {
  return ChromeLauncherController::instance()->CanPin();
}

bool AppListControllerDelegateAsh::CanShowCreateShortcutsDialog() {
  return false;
}

void AppListControllerDelegateAsh::CreateNewWindow(bool incognito) {
  if (incognito)
    ChromeLauncherController::instance()->CreateNewIncognitoWindow();
  else
    ChromeLauncherController::instance()->CreateNewWindow();
}

void AppListControllerDelegateAsh::ActivateApp(
    Profile* profile, const extensions::Extension* extension, int event_flags) {
  ChromeLauncherController::instance()->ActivateApp(extension->id(),
                                                    event_flags);
  DismissView();
}

void AppListControllerDelegateAsh::LaunchApp(
    Profile* profile, const extensions::Extension* extension, int event_flags) {
  ChromeLauncherController::instance()->LaunchApp(extension->id(),
                                                  event_flags);
  DismissView();
}

namespace chrome {

// In the win_aura build these are defined in app_list_controller_win.cc.
void ShowAppList(Profile* default_profile) {
  ash::Shell::GetInstance()->ToggleAppList(NULL);
}

bool IsAppListVisible() {
  return ash::Shell::GetInstance()->GetAppListWindow() != NULL;
}

void DismissAppList() {
  if (IsAppListVisible())
    ash::Shell::GetInstance()->ToggleAppList(NULL);
}

void SetAppListProfile(const base::FilePath& profile_file_path) {
}

void NotifyAppListOfBeginExtensionInstall(
    Profile* profile,
    const std::string& extension_id,
    const std::string& extension_name,
    const gfx::ImageSkia& installing_icon) {
}

void NotifyAppListOfDownloadProgress(
    Profile* profile,
    const std::string& extension_id,
    int percent_downloaded) {
}

void NotifyAppListOfExtensionInstallFailure(
    Profile* profile,
    const std::string& extension_id) {
}

}  // namespace chrome
