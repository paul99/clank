// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/shill_manager_client_stub.h"

#include "base/bind.h"
#include "base/chromeos/chromeos_version.h"
#include "base/message_loop.h"
#include "base/values.h"
#include "chromeos/dbus/shill_property_changed_observer.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "dbus/values_util.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

// Used to compare values for finding entries to erase in a ListValue.
// (ListValue only implements a const_iterator version of Find).
struct ValueEquals {
  explicit ValueEquals(const Value* first) : first_(first) {}
  bool operator()(const Value* second) const {
    return first_->Equals(second);
  }
  const Value* first_;
};

}  // namespace

ShillManagerClientStub::ShillManagerClientStub()
: weak_ptr_factory_(this) {
  SetDefaultProperties();
}

ShillManagerClientStub::~ShillManagerClientStub() {}

// ShillManagerClient overrides.

void ShillManagerClientStub::AddPropertyChangedObserver(
    ShillPropertyChangedObserver* observer) {
  observer_list_.AddObserver(observer);
}

void ShillManagerClientStub::RemovePropertyChangedObserver(
    ShillPropertyChangedObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void ShillManagerClientStub::GetProperties(
    const DictionaryValueCallback& callback) {
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(
          &ShillManagerClientStub::PassStubProperties,
          weak_ptr_factory_.GetWeakPtr(),
          callback));
}

base::DictionaryValue* ShillManagerClientStub::CallGetPropertiesAndBlock() {
  return stub_properties_.DeepCopy();
}

void ShillManagerClientStub::GetNetworksForGeolocation(
    const DictionaryValueCallback& callback) {
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(
          &ShillManagerClientStub::PassStubGeoNetworks,
          weak_ptr_factory_.GetWeakPtr(),
          callback));
}

void ShillManagerClientStub::SetProperty(const std::string& name,
                                         const base::Value& value,
                                         const base::Closure& callback,
                                         const ErrorCallback& error_callback) {
  stub_properties_.Set(name, value.DeepCopy());
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillManagerClientStub::RequestScan(const std::string& type,
                                         const base::Closure& callback,
                                         const ErrorCallback& error_callback) {
  const int kScanDelayMilliseconds = 3000;
  CallNotifyObserversPropertyChanged(
      flimflam::kServicesProperty, kScanDelayMilliseconds);
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillManagerClientStub::EnableTechnology(
    const std::string& type,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  base::ListValue* enabled_list = NULL;
  if (!stub_properties_.GetListWithoutPathExpansion(
      flimflam::kEnabledTechnologiesProperty, &enabled_list)) {
    if (!error_callback.is_null()) {
      MessageLoop::current()->PostTask(FROM_HERE, callback);
      MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(error_callback, "StubError", "Property not found"));
    }
    return;
  }
  enabled_list->AppendIfNotPresent(new base::StringValue(type));
  CallNotifyObserversPropertyChanged(
      flimflam::kEnabledTechnologiesProperty, 0);
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillManagerClientStub::DisableTechnology(
    const std::string& type,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  base::ListValue* enabled_list = NULL;
  if (!stub_properties_.GetListWithoutPathExpansion(
      flimflam::kEnabledTechnologiesProperty, &enabled_list)) {
    if (!error_callback.is_null()) {
      MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(error_callback, "StubError", "Property not found"));
    }
    return;
  }
  base::StringValue type_value(type);
  enabled_list->Remove(type_value, NULL);
  CallNotifyObserversPropertyChanged(
      flimflam::kEnabledTechnologiesProperty, 0);
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillManagerClientStub::ConfigureService(
    const base::DictionaryValue& properties,
    const ObjectPathCallback& callback,
    const ErrorCallback& error_callback) {
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, dbus::ObjectPath()));
}

void ShillManagerClientStub::GetService(
    const base::DictionaryValue& properties,
    const ObjectPathCallback& callback,
    const ErrorCallback& error_callback) {
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, dbus::ObjectPath()));
}

void ShillManagerClientStub::VerifyDestination(
    const std::string& certificate,
    const std::string& public_key,
    const std::string& nonce,
    const std::string& signed_data,
    const std::string& device_serial,
    const BooleanCallback& callback,
    const ErrorCallback& error_callback) {
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, true));
}

void ShillManagerClientStub::VerifyAndEncryptCredentials(
    const std::string& certificate,
    const std::string& public_key,
    const std::string& nonce,
    const std::string& signed_data,
    const std::string& device_serial,
    const std::string& service_path,
    const StringCallback& callback,
    const ErrorCallback& error_callback) {
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, "encrypted_credentials"));
}

void ShillManagerClientStub::VerifyAndEncryptData(
    const std::string& certificate,
    const std::string& public_key,
    const std::string& nonce,
    const std::string& signed_data,
    const std::string& device_serial,
    const std::string& data,
    const StringCallback& callback,
    const ErrorCallback& error_callback) {
  if (callback.is_null())
    return;
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, "encrypted_data"));
}


ShillManagerClient::TestInterface* ShillManagerClientStub::GetTestInterface() {
  return this;
}

// ShillManagerClient::TestInterface overrides.

void ShillManagerClientStub::AddDevice(const std::string& device_path) {
  if (GetListProperty(flimflam::kDevicesProperty)->AppendIfNotPresent(
      base::Value::CreateStringValue(device_path))) {
    CallNotifyObserversPropertyChanged(flimflam::kDevicesProperty, 0);
  }
}

void ShillManagerClientStub::RemoveDevice(const std::string& device_path) {
  base::StringValue device_path_value(device_path);
  if (GetListProperty(flimflam::kDevicesProperty)->Remove(
      device_path_value, NULL)) {
    CallNotifyObserversPropertyChanged(flimflam::kDevicesProperty, 0);
  }
}

void ShillManagerClientStub::ResetDevices() {
  stub_properties_.Remove(flimflam::kDevicesProperty, NULL);
}

void ShillManagerClientStub::AddService(const std::string& service_path,
                                        bool add_to_watch_list) {
  if (GetListProperty(flimflam::kServicesProperty)->AppendIfNotPresent(
      base::Value::CreateStringValue(service_path))) {
    CallNotifyObserversPropertyChanged(flimflam::kServicesProperty, 0);
  }
  if (add_to_watch_list)
    AddServiceToWatchList(service_path);
}

void ShillManagerClientStub::AddServiceAtIndex(const std::string& service_path,
                                               size_t index,
                                               bool add_to_watch_list) {
  base::StringValue path_value(service_path);
  base::ListValue* service_list =
      GetListProperty(flimflam::kServicesProperty);
  base::ListValue::iterator iter =
      std::find_if(service_list->begin(), service_list->end(),
                   ValueEquals(&path_value));
  service_list->Find(path_value);
  if (iter != service_list->end())
    service_list->Erase(iter, NULL);
  service_list->Insert(index, path_value.DeepCopy());
  CallNotifyObserversPropertyChanged(flimflam::kServicesProperty, 0);
  if (add_to_watch_list)
    AddServiceToWatchList(service_path);
}

void ShillManagerClientStub::RemoveService(const std::string& service_path) {
  base::StringValue service_path_value(service_path);
  if (GetListProperty(flimflam::kServicesProperty)->Remove(
      service_path_value, NULL)) {
    CallNotifyObserversPropertyChanged(flimflam::kServicesProperty, 0);
  }
  if (GetListProperty(flimflam::kServiceWatchListProperty)->Remove(
      service_path_value, NULL)) {
    CallNotifyObserversPropertyChanged(
        flimflam::kServiceWatchListProperty, 0);
  }
}

void ShillManagerClientStub::AddTechnology(const std::string& type,
                                           bool enabled) {
  if (GetListProperty(flimflam::kAvailableTechnologiesProperty)->
      AppendIfNotPresent(base::Value::CreateStringValue(type))) {
    CallNotifyObserversPropertyChanged(
        flimflam::kAvailableTechnologiesProperty, 0);
  }
  if (enabled &&
      GetListProperty(flimflam::kEnabledTechnologiesProperty)->
      AppendIfNotPresent(base::Value::CreateStringValue(type))) {
    CallNotifyObserversPropertyChanged(
        flimflam::kEnabledTechnologiesProperty, 0);
  }
}

void ShillManagerClientStub::RemoveTechnology(const std::string& type) {
  base::StringValue type_value(type);
  if (GetListProperty(flimflam::kAvailableTechnologiesProperty)->Remove(
      type_value, NULL)) {
    CallNotifyObserversPropertyChanged(
        flimflam::kAvailableTechnologiesProperty, 0);
  }
  if (GetListProperty(flimflam::kEnabledTechnologiesProperty)->Remove(
      type_value, NULL)) {
    CallNotifyObserversPropertyChanged(
        flimflam::kEnabledTechnologiesProperty, 0);
  }
}

void ShillManagerClientStub::ClearProperties() {
  stub_properties_.Clear();
}

void ShillManagerClientStub::AddGeoNetwork(
    const std::string& technology,
    const base::DictionaryValue& network) {
  base::ListValue* list_value = NULL;
  if (!stub_geo_networks_.GetListWithoutPathExpansion(
      technology, &list_value)) {
    list_value = new base::ListValue;
    stub_geo_networks_.Set(technology, list_value);
  }
  list_value->Append(network.DeepCopy());
}

void ShillManagerClientStub::AddServiceToWatchList(
    const std::string& service_path) {
  if (GetListProperty(
      flimflam::kServiceWatchListProperty)->AppendIfNotPresent(
          base::Value::CreateStringValue(service_path))) {
    CallNotifyObserversPropertyChanged(
        flimflam::kServiceWatchListProperty, 0);
  }
}

void ShillManagerClientStub::SetDefaultProperties() {
  // Stub Devices, Note: names match Device stub map.
  AddDevice("stub_wifi_device1");
  AddDevice("stub_cellular_device1");

  // Stub Services, Note: names match Service stub map.
  AddService("stub_ethernet", true);
  AddService("stub_wifi1", true);
  AddService("stub_wifi2", true);
  AddService("stub_cellular1", true);

  // Stub Technologies
  AddTechnology(flimflam::kTypeEthernet, true);
  AddTechnology(flimflam::kTypeWifi, true);
  AddTechnology(flimflam::kTypeCellular, true);
}

void ShillManagerClientStub::PassStubProperties(
    const DictionaryValueCallback& callback) const {
  callback.Run(DBUS_METHOD_CALL_SUCCESS, stub_properties_);
}

void ShillManagerClientStub::PassStubGeoNetworks(
    const DictionaryValueCallback& callback) const {
  callback.Run(DBUS_METHOD_CALL_SUCCESS, stub_geo_networks_);
}

void ShillManagerClientStub::CallNotifyObserversPropertyChanged(
    const std::string& property,
    int delay_ms) {
  // Avoid unnecessary delayed task if we have no observers (e.g. during
  // initial setup).
  if (observer_list_.size() == 0)
    return;
  MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&ShillManagerClientStub::NotifyObserversPropertyChanged,
                 weak_ptr_factory_.GetWeakPtr(),
                 property),
                 base::TimeDelta::FromMilliseconds(delay_ms));
}

void ShillManagerClientStub::NotifyObserversPropertyChanged(
    const std::string& property) {
  base::Value* value = NULL;
  if (!stub_properties_.GetWithoutPathExpansion(property, &value)) {
    LOG(ERROR) << "Notify for unknown property: " << property;
    return;
  }
  FOR_EACH_OBSERVER(ShillPropertyChangedObserver,
                    observer_list_,
                    OnPropertyChanged(property, *value));
}

base::ListValue* ShillManagerClientStub::GetListProperty(
    const std::string& property) {
  base::ListValue* list_property = NULL;
  if (!stub_properties_.GetListWithoutPathExpansion(
      property, &list_property)) {
    list_property = new base::ListValue;
    stub_properties_.Set(property, list_property);
  }
  return list_property;
}

}  // namespace chromeos
