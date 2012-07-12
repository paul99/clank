// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOFILL_AUTOFILL_EXTERNAL_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_AUTOFILL_AUTOFILL_EXTERNAL_DELEGATE_ANDROID_H_
#pragma once

#include <map>
#include <vector>

#include "base/compiler_specific.h"
#include "base/string16.h"
#include "chrome/browser/autofill/autofill_external_delegate.h"
#include "webkit/forms/form_data.h"
#include "webkit/forms/form_field.h"

class AutofillManager;
class ChromeView;
class TabContents;

// TODO(jrg):  UI mechanism for clearing autofill suggestions (long press?)

// Android specific external delegate for display and selection of
// autocomplete data.
class AutofillExternalDelegateAndroid : public AutofillExternalDelegate {
 public:
  AutofillExternalDelegateAndroid(TabContentsWrapper* tab_contents,
                                  AutofillManager* manager);
  virtual ~AutofillExternalDelegateAndroid();

  virtual void OnQuery(int query_id,
                       const webkit::forms::FormData& form,
                       const webkit::forms::FormField& field,
                       const gfx::Rect& bounds,
                       bool display_warning_if_disabled) OVERRIDE;
  virtual void OnSuggestionsReturned(
      int query_id,
      const std::vector<string16>& autofill_values,
      const std::vector<string16>& autofill_labels,
      const std::vector<string16>& autofill_icons,
      const std::vector<int>& autofill_unique_ids) OVERRIDE;

  // Called when the native view was closed.
  void OnClose();

  virtual void HideAutofillPopup() OVERRIDE;

  virtual void DidEndTextFieldEditing() OVERRIDE;

 protected:
  friend class ChromeView;

  virtual void ApplyAutofillSuggestions(
      const std::vector<string16>& autofill_values,
      const std::vector<string16>& autofill_labels,
      const std::vector<string16>& autofill_icons,
      const std::vector<int>& autofill_unique_ids,
      int separator_index) OVERRIDE;

  virtual void OnQueryPlatformSpecific(
      int query_id,
      const webkit::forms::FormData& form,
      const webkit::forms::FormField& field,
      const gfx::Rect& bounds) OVERRIDE;

 private:
  // Close external display.  Delete all cached answers.
  void Close();

  // Get the ChromeView associated with our owning TabContents.
  // May be NULL.
  ChromeView* GetChromeView();

  TabContentsWrapper* tab_contents_wrapper_; // weak; owns me
  AutofillManager* manager_;  // weak; owned by tab_contents_

  // Data associated with a query_id
  struct QueryData {
    QueryData(const webkit::forms::FormData& inform,
              const webkit::forms::FormField& infield)
        : form(inform), field(infield) {}
    QueryData(const QueryData& in) : form(in.form), field(in.field) {}
    QueryData() {}

    webkit::forms::FormData form;
    webkit::forms::FormField field;
  };

  // Map of query_id to it's associated FormData/FormField.
  typedef std::map<int, QueryData> QueryDataMap;
  QueryDataMap query_map_;

  // The query_id we are currently displaying, or -1.
  int current_display_query_id_;

  DISALLOW_COPY_AND_ASSIGN(AutofillExternalDelegateAndroid);
};

#endif /* CHROME_BROWSER_AUTOFILL_AUTOFILL_EXTERNAL_DELEGATE_ANDROID_H_ */
