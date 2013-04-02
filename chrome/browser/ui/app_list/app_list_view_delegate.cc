// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/app_list_view_delegate.h"

#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/apps_model_builder.h"
#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "chrome/browser/ui/app_list/chrome_signin_delegate.h"
#include "chrome/browser/ui/app_list/search_builder.h"
#include "content/public/browser/user_metrics.h"

#if defined(USE_ASH)
#include "chrome/browser/ui/ash/app_list/app_sync_ui_state_watcher.h"
#endif

AppListViewDelegate::AppListViewDelegate(AppListControllerDelegate* controller,
                                         Profile* profile)
    : controller_(controller),
      profile_(profile) {}

AppListViewDelegate::~AppListViewDelegate() {}

void AppListViewDelegate::SetModel(app_list::AppListModel* model) {
  if (model) {
    apps_builder_.reset(new AppsModelBuilder(profile_,
                                             model->apps(),
                                             controller_.get()));
    apps_builder_->Build();

    search_builder_.reset(new SearchBuilder(profile_,
                                            model->search_box(),
                                            model->results(),
                                            controller_.get()));

    signin_delegate_.reset(new ChromeSigninDelegate(profile_));

#if defined(USE_ASH)
    app_sync_ui_state_watcher_.reset(new AppSyncUIStateWatcher(profile_,
                                                               model));
#endif
  } else {
    apps_builder_.reset();
    search_builder_.reset();
#if defined(USE_ASH)
    app_sync_ui_state_watcher_.reset();
#endif
  }
}

app_list::SigninDelegate* AppListViewDelegate::GetSigninDelegate() {
  return signin_delegate_.get();
}

void AppListViewDelegate::OnBeginExtensionInstall(
    const std::string& extension_id,
    const std::string& extension_name,
    const gfx::ImageSkia& installing_icon) {
  apps_builder_->OnBeginExtensionInstall(extension_id,
                                         extension_name,
                                         installing_icon);
}

void AppListViewDelegate::OnDownloadProgress(
    const std::string& extension_id,
    int percent_downloaded) {
  apps_builder_->OnDownloadProgress(extension_id, percent_downloaded);
}

void AppListViewDelegate::OnInstallFailure(const std::string& extension_id) {
  apps_builder_->OnInstallFailure(extension_id);
}

void AppListViewDelegate::ActivateAppListItem(
    app_list::AppListItemModel* item,
    int event_flags) {
  content::RecordAction(content::UserMetricsAction("AppList_ClickOnApp"));
  static_cast<ChromeAppListItem*>(item)->Activate(event_flags);
}

void AppListViewDelegate::StartSearch() {
  if (search_builder_.get())
    search_builder_->StartSearch();
}

void AppListViewDelegate::StopSearch() {
  if (search_builder_.get())
    search_builder_->StopSearch();
}

void AppListViewDelegate::OpenSearchResult(
    const app_list::SearchResult& result,
    int event_flags) {
  if (search_builder_.get())
    search_builder_->OpenResult(result, event_flags);
}

void AppListViewDelegate::InvokeSearchResultAction(
    const app_list::SearchResult& result,
    int action_index,
    int event_flags) {
  if (search_builder_.get())
    search_builder_->InvokeResultAction(result, action_index, event_flags);
}

void AppListViewDelegate::Dismiss()  {
  controller_->DismissView();
}

void AppListViewDelegate::ViewClosing() {
  controller_->ViewClosing();
}

void AppListViewDelegate::ViewActivationChanged(bool active) {
  controller_->ViewActivationChanged(active);
}

gfx::ImageSkia AppListViewDelegate::GetWindowIcon() {
  return controller_->GetWindowIcon();
}
