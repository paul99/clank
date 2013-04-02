// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/wallet/wallet_address.h"

#include "base/logging.h"
#include "base/values.h"

namespace wallet {

Address::Address() {}

Address::Address(const std::string& country_name_code,
                 const std::string& recipient_name,
                 const std::string& address_line_1,
                 const std::string& address_line_2,
                 const std::string& locality_name,
                 const std::string& administrative_area_name,
                 const std::string& postal_code_number,
                 const std::string& phone_number,
                 const std::string& object_id)
    : country_name_code_(country_name_code),
      recipient_name_(recipient_name),
      address_line_1_(address_line_1),
      address_line_2_(address_line_2),
      locality_name_(locality_name),
      administrative_area_name_(administrative_area_name),
      postal_code_number_(postal_code_number),
      phone_number_(phone_number),
      object_id_(object_id) {}

Address::~Address() {}

scoped_ptr<base::DictionaryValue> Address::ToDictionaryWithID() const {
  scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue());

  if (!object_id_.empty())
    dict->SetString("id", object_id_);
  dict->SetString("phone_number", phone_number_);
  dict->Set("postal_address", ToDictionaryWithoutID().release());

  return dict.Pass();
}

scoped_ptr<base::DictionaryValue> Address::ToDictionaryWithoutID() const {
  scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue());

  scoped_ptr<base::ListValue> address_lines(new base::ListValue());
  address_lines->AppendString(address_line_1_);
  if (!address_line_2_.empty())
    address_lines->AppendString(address_line_2_);
  dict->Set("address_line", address_lines.release());

  dict->SetString("country_name_code", country_name_code_);
  dict->SetString("recipient_name", recipient_name_);
  dict->SetString("locality_name", locality_name_);
  dict->SetString("administrative_area_name",
                  administrative_area_name_);
  dict->SetString("postal_code_number", postal_code_number_);

  return dict.Pass();
}


scoped_ptr<Address>
    Address::CreateAddressWithID(const base::DictionaryValue& dictionary) {
  std::string object_id;
  if (!dictionary.GetString("id", &object_id)) {
    DLOG(ERROR) << "Response from Google Wallet missing object id";
    return scoped_ptr<Address>();
  }

  std::string country_name_code;
  if (!dictionary.GetString("postal_address.country_name_code",
                            &country_name_code)) {
    DLOG(ERROR) << "Response from Google Wallet missing country name";
    return scoped_ptr<Address>();
  }

  std::string recipient_name;
  if (!dictionary.GetString("postal_address.recipient_name",
                            &recipient_name)) {
    DLOG(ERROR) << "Response from Google Wallet recipient name";
    return scoped_ptr<Address>();
  }

  std::string postal_code_number;
  if (!dictionary.GetString("postal_address.postal_code_number",
                            &postal_code_number)) {
    DLOG(ERROR) << "Response from Google Wallet missing postal code number";
    return scoped_ptr<Address>();
  }

  std::string phone_number;
  if (!dictionary.GetString("phone_number", &phone_number))
    DVLOG(1) << "Response from Google Wallet missing phone number";

  std::string address_line_1;
  std::string address_line_2;
  const ListValue* address_line_list;
  if (dictionary.GetList("postal_address.address_line", &address_line_list)) {
    if (!address_line_list->GetString(0, &address_line_1))
      DVLOG(1) << "Response from Google Wallet missing address line 1";
    if (!address_line_list->GetString(1, &address_line_2))
      DVLOG(1) << "Response from Google Wallet missing address line 2";
  } else {
    DVLOG(1) << "Response from Google Wallet missing address lines";
  }

  std::string locality_name;
  if (!dictionary.GetString("postal_address.locality_name",
                            &locality_name)) {
    DVLOG(1) << "Response from Google Wallet missing locality name";
  }

  std::string administrative_area_name;
  if (!dictionary.GetString("postal_address.administrative_area_name",
                            &administrative_area_name)) {
    DVLOG(1) << "Response from Google Wallet missing administrative area name";
  }

  return scoped_ptr<Address>(new Address(country_name_code,
                                         recipient_name ,
                                         address_line_1,
                                         address_line_2,
                                         locality_name,
                                         administrative_area_name,
                                         postal_code_number,
                                         phone_number,
                                         object_id));
}


scoped_ptr<Address>
    Address::CreateDisplayAddress(const base::DictionaryValue& dictionary) {
  std::string country_code;
  if (!dictionary.GetString("country_code", &country_code)) {
    DLOG(ERROR) << "Reponse from Google Wallet missing country code";
    return scoped_ptr<Address>();
  }

  std::string name;
  if (!dictionary.GetString("name", &name)) {
    DLOG(ERROR) << "Reponse from Google Wallet missing name";
    return scoped_ptr<Address>();
  }

  std::string postal_code;
  if (!dictionary.GetString("postal_code", &postal_code)) {
    DLOG(ERROR) << "Reponse from Google Wallet missing postal code";
    return scoped_ptr<Address>();
  }

  std::string address1;
  if (!dictionary.GetString("address1", &address1))
    DVLOG(1) << "Reponse from Google Wallet missing address1";

  std::string address2;
  if (!dictionary.GetString("address2", &address2))
    DVLOG(1) << "Reponse from Google Wallet missing address2";

  std::string city;
  if (!dictionary.GetString("city", &city))
    DVLOG(1) << "Reponse from Google Wallet missing city";

  std::string state;
  if (!dictionary.GetString("state", &state))
    DVLOG(1) << "Reponse from Google Wallet missing state";

  std::string phone_number;
  if (!dictionary.GetString("phone_number", &phone_number))
    DVLOG(1) << "Reponse from Google Wallet missing phone number";

  return scoped_ptr<Address>(new Address(country_code,
                                         name,
                                         address1,
                                         address2,
                                         city,
                                         state,
                                         postal_code,
                                         phone_number,
                                         std::string()));
}

bool Address::operator==(const Address& other) const {
  return country_name_code_ == other.country_name_code_ &&
         recipient_name_ == other.recipient_name_ &&
         address_line_1_ == other.address_line_1_ &&
         address_line_2_ == other.address_line_2_ &&
         locality_name_ == other.locality_name_ &&
         administrative_area_name_ == other.administrative_area_name_ &&
         postal_code_number_ == other.postal_code_number_ &&
         phone_number_ == other.phone_number_ &&
         object_id_ == other.object_id_;
}

bool Address::operator!=(const Address& other) const {
  return !(*this == other);
}

}  // namespace wallet

