// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/autocheckout_manager.h"

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autofill/autofill_country.h"
#include "chrome/browser/autofill/autofill_field.h"
#include "chrome/browser/autofill/autofill_manager.h"
#include "chrome/browser/autofill/autofill_profile.h"
#include "chrome/browser/autofill/credit_card.h"
#include "chrome/browser/autofill/field_types.h"
#include "chrome/browser/autofill/form_structure.h"
#include "chrome/browser/ui/autofill/autofill_dialog_controller_impl.h"
#include "chrome/common/autofill/web_element_descriptor.h"
#include "chrome/common/autofill_messages.h"
#include "chrome/common/form_data.h"
#include "chrome/common/form_field_data.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/ssl_status.h"
#include "googleurl/src/gurl.h"
#include "ui/gfx/rect.h"

using content::RenderViewHost;
using content::SSLStatus;
using content::WebContents;

namespace {

// Build FormFieldData based on the supplied |autocomplete_attribute|. Will
// fill rest of properties with default values.
FormFieldData BuildField(const std::string& autocomplete_attribute) {
  FormFieldData field;
  field.name = string16();
  field.value = string16();
  field.autocomplete_attribute = autocomplete_attribute;
  field.form_control_type = "text";
  return field;
}

// Build Autocheckout specific form data to be consumed by
// AutofillDialogController to show the Autocheckout specific UI.
FormData BuildAutocheckoutFormData() {
  FormData formdata;
  formdata.fields.push_back(BuildField("name"));
  formdata.fields.push_back(BuildField("tel"));
  formdata.fields.push_back(BuildField("email"));
  formdata.fields.push_back(BuildField("cc-name"));
  formdata.fields.push_back(BuildField("cc-number"));
  formdata.fields.push_back(BuildField("cc-exp"));
  formdata.fields.push_back(BuildField("cc-csc"));
  formdata.fields.push_back(BuildField("billing street-address"));
  formdata.fields.push_back(BuildField("billing locality"));
  formdata.fields.push_back(BuildField("billing region"));
  formdata.fields.push_back(BuildField("billing country"));
  formdata.fields.push_back(BuildField("billing postal-code"));
  formdata.fields.push_back(BuildField("shipping street-address"));
  formdata.fields.push_back(BuildField("shipping locality"));
  formdata.fields.push_back(BuildField("shipping region"));
  formdata.fields.push_back(BuildField("shipping country"));
  formdata.fields.push_back(BuildField("shipping postal-code"));
  return formdata;
}

}  // namespace

AutocheckoutManager::AutocheckoutManager(AutofillManager* autofill_manager)
    : autofill_manager_(autofill_manager),
      autocheckout_bubble_shown_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this)) {
}

AutocheckoutManager::~AutocheckoutManager() {
}

void AutocheckoutManager::FillForms() {
  // |page_meta_data_| should have been set by OnLoadedPageMetaData and this
  // function should only be called if page has a proceed element.
  DCHECK(page_meta_data_ && page_meta_data_->proceed_element_descriptor);

  // Fill the forms on the page with data given by user.
  std::vector<FormData> filled_forms;
  const std::vector<FormStructure*>& form_structures =
    autofill_manager_->GetFormStructures();
  for (std::vector<FormStructure*>::const_iterator iter =
           form_structures.begin(); iter != form_structures.end(); ++iter) {
    const FormStructure& form_structure = **iter;
    FormData form = form_structure.ToFormData();
    DCHECK_EQ(form_structure.field_count(), form.fields.size());

    for (size_t i = 0; i < form_structure.field_count(); ++i) {
      const AutofillField* field = form_structure.field(i);
      SetValue(*field, &form.fields[i]);
    }

    filled_forms.push_back(form);
  }

  // Send filled forms along with proceed descriptor to renderer.
  RenderViewHost* host =
      autofill_manager_->GetWebContents()->GetRenderViewHost();
  if (!host)
    return;

  host->Send(new AutofillMsg_FillFormsAndClick(
      host->GetRoutingID(),
      filled_forms,
      *page_meta_data_->proceed_element_descriptor));
}

void AutocheckoutManager::OnLoadedPageMetaData(
    scoped_ptr<autofill::AutocheckoutPageMetaData> page_meta_data) {
  page_meta_data_ = page_meta_data.Pass();
}

void AutocheckoutManager::OnFormsSeen() {
  autocheckout_bubble_shown_ = false;
}

bool AutocheckoutManager::MaybeShowAutocheckoutBubble(
    const GURL& frame_url,
    const content::SSLStatus& ssl_status,
    const gfx::NativeView& native_view,
    const gfx::RectF& bounding_box) {
  if (autocheckout_bubble_shown_)
    return false;

  base::Closure callback = base::Bind(
      &AutocheckoutManager::ShowAutocheckoutDialog,
      weak_ptr_factory_.GetWeakPtr(),
      frame_url,
      ssl_status);
  autofill_manager_->delegate()->ShowAutocheckoutBubble(
      bounding_box,
      native_view,
      callback);
  autocheckout_bubble_shown_ = true;
  return true;
}

void AutocheckoutManager::ShowAutocheckoutDialog(
    const GURL& frame_url,
    const SSLStatus& ssl_status) {
  base::Callback<void(const FormStructure*)> callback =
      base::Bind(&AutocheckoutManager::ReturnAutocheckoutData,
                 weak_ptr_factory_.GetWeakPtr());
  autofill_manager_->delegate()->ShowRequestAutocompleteDialog(
      BuildAutocheckoutFormData(), frame_url, ssl_status, callback);
}

bool AutocheckoutManager::IsStartOfAutofillableFlow() const {
  return page_meta_data_ && page_meta_data_->IsStartOfAutofillableFlow();
}

bool AutocheckoutManager::IsInAutofillableFlow() const {
  return page_meta_data_ && page_meta_data_->IsInAutofillableFlow();
}

void AutocheckoutManager::ReturnAutocheckoutData(const FormStructure* result) {
  if (!result)
    return;

  profile_.reset(new AutofillProfile());
  credit_card_.reset(new CreditCard());

  for (size_t i = 0; i < result->field_count(); ++i) {
    AutofillFieldType type = result->field(i)->type();
    if (type == CREDIT_CARD_VERIFICATION_CODE) {
      // TODO(ramankk): CVV is not handled by CreditCard, not sure how to
      // handle it yet.
      cvv_ = result->field(i)->value;
      continue;
    }
    if (AutofillType(type).group() == AutofillType::CREDIT_CARD) {
      credit_card_->SetRawInfo(result->field(i)->type(),
                               result->field(i)->value);
      // TODO(ramankk): Fix NAME_FULL when dialog controller can return it.
      if (result->field(i)->type() == CREDIT_CARD_NAME) {
        profile_->SetRawInfo(NAME_FULL, result->field(i)->value);
      }
    } else {
      profile_->SetRawInfo(result->field(i)->type(), result->field(i)->value);
    }
  }

  FillForms();

  // TODO(ramankk): Manage lifecycle of autofill dialog controller correctly.
  autofill_manager_->RequestAutocompleteDialogClosed();
}

void AutocheckoutManager::SetValue(const AutofillField& field,
                                   FormFieldData* field_to_fill) {
  AutofillFieldType type = field.type();

  if (type == FIELD_WITH_DEFAULT_VALUE) {
    DCHECK(field.is_checkable);
    // For a form with radio buttons, like:
    // <form>
    //   <input type="radio" name="sex" value="male">Male<br>
    //   <input type="radio" name="sex" value="female">Female
    // </form>
    // If the default value specified at the server is "female", then
    // Autofill server responds back with following field mappings
    //   (fieldtype: FIELD_WITH_DEFAULT_VALUE, value: "female")
    //   (fieldtype: FIELD_WITH_DEFAULT_VALUE, value: "female")
    // Note that, the field mapping is repeated twice to respond to both the
    // input elements with the same name/signature in the form.
    string16 default_value = UTF8ToUTF16(field.default_value());
    // Mark the field checked if server says the default value of the field
    // to be this field's value.
    field_to_fill->is_checked = (field.value == default_value);
    return;
  }

  // Handle verification code directly.
  if (type == CREDIT_CARD_VERIFICATION_CODE) {
    field_to_fill->value = cvv_;
    return;
  }

  // TODO(ramankk): Handle variants in a better fashion, need to distinguish
  // between shipping and billing address.
  if (AutofillType(type).group() == AutofillType::CREDIT_CARD)
    credit_card_->FillFormField(field, 0, field_to_fill);
  else
    profile_->FillFormField(field, 0, field_to_fill);
}
