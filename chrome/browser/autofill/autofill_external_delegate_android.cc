// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/autofill_external_delegate_android.h"

#include "base/string_util.h"
#include "chrome/browser/autofill/autofill_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "content/browser/android/chrome_view.h"

using content::BrowserThread;

AutofillExternalDelegate* AutofillExternalDelegate::Create(
    TabContentsWrapper* tab_contents,
    AutofillManager* manager) {
  return new AutofillExternalDelegateAndroid(tab_contents, manager);
}

AutofillExternalDelegateAndroid::AutofillExternalDelegateAndroid(
    TabContentsWrapper* tab_contents, AutofillManager* manager)
    : AutofillExternalDelegate(tab_contents, manager),
      tab_contents_wrapper_(tab_contents),
      manager_(manager),
      current_display_query_id_(-1) {
}

AutofillExternalDelegateAndroid::~AutofillExternalDelegateAndroid() {}

void AutofillExternalDelegateAndroid::OnQuery(
    int query_id,
    const webkit::forms::FormData& form,
    const webkit::forms::FormField& field,
    const gfx::Rect& bounds,
    bool display_warning_if_disabled) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // We only need to store QueryData for the pending query and the one
  // currently being displayed (if any).
  QueryDataMap newmap;
  if (current_display_query_id_ >= 0)
    newmap[current_display_query_id_] = query_map_[current_display_query_id_];
  query_map_ = newmap;

  QueryData querydata(form, field);
  query_map_[query_id] = querydata;
}

void AutofillExternalDelegateAndroid::OnSuggestionsReturned(
    int query_id,
    const std::vector<string16>& autofill_values,
    const std::vector<string16>& autofill_labels,
    const std::vector<string16>& autofill_icons,
    const std::vector<int>& autofill_unique_ids) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  ChromeView* chrome_view = GetChromeView();
  if (chrome_view) {
    current_display_query_id_ = query_id;
    // TODO(jrg): I'm ignoring icons for now
    chrome_view->OpenAutofillPopup(this,
                                   query_id,
                                   autofill_values,
                                   autofill_labels,
                                   autofill_unique_ids);
  }
}

void AutofillExternalDelegateAndroid::DidEndTextFieldEditing() {
  ChromeView* chrome_view = GetChromeView();
  if (chrome_view)
    chrome_view->resetLastPressAck();
}

void AutofillExternalDelegateAndroid::ApplyAutofillSuggestions(
    const std::vector<string16>& autofill_values,
    const std::vector<string16>& autofill_labels,
    const std::vector<string16>& autofill_icons,
    const std::vector<int>& autofill_unique_ids,
    int separator_index) {
  // Merge 11/29/2011
  NOTIMPLEMENTED();
}

void AutofillExternalDelegateAndroid::OnQueryPlatformSpecific(
    int query_id,
    const webkit::forms::FormData& form,
    const webkit::forms::FormField& field,
    const gfx::Rect& bounds) {
  // Merge 01/22/2012
  NOTIMPLEMENTED();
}

ChromeView* AutofillExternalDelegateAndroid::GetChromeView() {
  RenderWidgetHostViewAndroid* rwhva = static_cast<RenderWidgetHostViewAndroid*>(
      tab_contents_wrapper_->web_contents()->GetRenderWidgetHostView());
  ChromeView* chrome_view = rwhva ? rwhva->GetChromeView() : NULL;
  return chrome_view;
}

void AutofillExternalDelegateAndroid::Close() {
  ChromeView* chrome_view = GetChromeView();
  if (chrome_view)
    chrome_view->CloseAutofillPopup();
}

void AutofillExternalDelegateAndroid::HideAutofillPopup() {
}

void AutofillExternalDelegateAndroid::OnClose() {
  query_map_.clear();
  current_display_query_id_ = 0;
}
