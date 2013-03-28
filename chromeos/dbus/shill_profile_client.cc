// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/shill_profile_client.h"

#include "base/bind.h"
#include "base/message_loop.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "chromeos/dbus/shill_property_changed_observer.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/values_util.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

// The ShillProfileClient implementation.
class ShillProfileClientImpl : public ShillProfileClient {
 public:
  explicit ShillProfileClientImpl(dbus::Bus* bus);

  /////////////////////////////////////
  // ShillProfileClient overrides.
  virtual void AddPropertyChangedObserver(
      const dbus::ObjectPath& profile_path,
      ShillPropertyChangedObserver* observer) OVERRIDE {
    GetHelper(profile_path)->AddPropertyChangedObserver(observer);
  }

  virtual void RemovePropertyChangedObserver(
      const dbus::ObjectPath& profile_path,
      ShillPropertyChangedObserver* observer) OVERRIDE {
    GetHelper(profile_path)->RemovePropertyChangedObserver(observer);
  }
  virtual void GetProperties(
      const dbus::ObjectPath& profile_path,
      const DictionaryValueCallbackWithoutStatus& callback,
      const ErrorCallback& error_callback) OVERRIDE;
  virtual void GetEntry(const dbus::ObjectPath& profile_path,
                        const std::string& entry_path,
                        const DictionaryValueCallbackWithoutStatus& callback,
                        const ErrorCallback& error_callback) OVERRIDE;
  virtual void DeleteEntry(const dbus::ObjectPath& profile_path,
                           const std::string& entry_path,
                           const base::Closure& callback,
                           const ErrorCallback& error_callback) OVERRIDE;

 private:
  typedef std::map<std::string, ShillClientHelper*> HelperMap;

  // Returns the corresponding ShillClientHelper for the profile.
  ShillClientHelper* GetHelper(const dbus::ObjectPath& profile_path);

  dbus::Bus* bus_;
  HelperMap helpers_;
  STLValueDeleter<HelperMap> helpers_deleter_;

  DISALLOW_COPY_AND_ASSIGN(ShillProfileClientImpl);
};

ShillProfileClientImpl::ShillProfileClientImpl(dbus::Bus* bus)
    : bus_(bus),
      helpers_deleter_(&helpers_) {
}

ShillClientHelper* ShillProfileClientImpl::GetHelper(
    const dbus::ObjectPath& profile_path) {
  HelperMap::iterator it = helpers_.find(profile_path.value());
  if (it != helpers_.end())
    return it->second;

  // There is no helper for the profile, create it.
  dbus::ObjectProxy* object_proxy =
      bus_->GetObjectProxy(flimflam::kFlimflamServiceName, profile_path);
  ShillClientHelper* helper = new ShillClientHelper(bus_, object_proxy);
  helper->MonitorPropertyChanged(flimflam::kFlimflamProfileInterface);
  helpers_.insert(HelperMap::value_type(profile_path.value(), helper));
  return helper;
}

void ShillProfileClientImpl::GetProperties(
    const dbus::ObjectPath& profile_path,
    const DictionaryValueCallbackWithoutStatus& callback,
    const ErrorCallback& error_callback) {
  dbus::MethodCall method_call(flimflam::kFlimflamProfileInterface,
                               flimflam::kGetPropertiesFunction);
  GetHelper(profile_path)->CallDictionaryValueMethodWithErrorCallback(
      &method_call, callback, error_callback);
}

void ShillProfileClientImpl::GetEntry(
    const dbus::ObjectPath& profile_path,
    const std::string& entry_path,
    const DictionaryValueCallbackWithoutStatus& callback,
    const ErrorCallback& error_callback) {
  dbus::MethodCall method_call(flimflam::kFlimflamProfileInterface,
                               flimflam::kGetEntryFunction);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(entry_path);
  GetHelper(profile_path)->CallDictionaryValueMethodWithErrorCallback(
      &method_call, callback, error_callback);
}

void ShillProfileClientImpl::DeleteEntry(
    const dbus::ObjectPath& profile_path,
    const std::string& entry_path,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  dbus::MethodCall method_call(flimflam::kFlimflamProfileInterface,
                               flimflam::kDeleteEntryFunction);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(entry_path);
  GetHelper(profile_path)->CallVoidMethodWithErrorCallback(
      &method_call, callback, error_callback);
}

// A stub implementation of ShillProfileClient.
class ShillProfileClientStubImpl : public ShillProfileClient {
 public:
  ShillProfileClientStubImpl() : weak_ptr_factory_(this) {}

  virtual ~ShillProfileClientStubImpl() {}

  //////////////////////////////////////
  // ShillProfileClient overrides.
  virtual void AddPropertyChangedObserver(
      const dbus::ObjectPath& profile_path,
      ShillPropertyChangedObserver* observer) OVERRIDE {}

  virtual void RemovePropertyChangedObserver(
      const dbus::ObjectPath& profile_path,
      ShillPropertyChangedObserver* observer) OVERRIDE {}

  virtual void GetProperties(
      const dbus::ObjectPath& profile_path,
      const DictionaryValueCallbackWithoutStatus& callback,
      const ErrorCallback& error_callback) OVERRIDE {
    if (callback.is_null())
      return;
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&ShillProfileClientStubImpl::PassEmptyDictionaryValue,
                   weak_ptr_factory_.GetWeakPtr(),
                   callback));
  }

  virtual void GetEntry(const dbus::ObjectPath& profile_path,
                        const std::string& entry_path,
                        const DictionaryValueCallbackWithoutStatus& callback,
                        const ErrorCallback& error_callback) OVERRIDE {
    if (callback.is_null())
      return;
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&ShillProfileClientStubImpl::PassEmptyDictionaryValue,
                   weak_ptr_factory_.GetWeakPtr(),
                   callback));
  }

  virtual void DeleteEntry(const dbus::ObjectPath& profile_path,
                           const std::string& entry_path,
                           const base::Closure& callback,
                           const ErrorCallback& error_callback) OVERRIDE {
    if (callback.is_null())
      return;
    MessageLoop::current()->PostTask(FROM_HERE, callback);
  }

 private:
  void PassEmptyDictionaryValue(
      const DictionaryValueCallbackWithoutStatus& callback) const {
    base::DictionaryValue dictionary;
    callback.Run(dictionary);
  }

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<ShillProfileClientStubImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShillProfileClientStubImpl);
};

}  // namespace

ShillProfileClient::ShillProfileClient() {}

ShillProfileClient::~ShillProfileClient() {}

// static
ShillProfileClient* ShillProfileClient::Create(
    DBusClientImplementationType type,
    dbus::Bus* bus) {
  if (type == REAL_DBUS_CLIENT_IMPLEMENTATION)
    return new ShillProfileClientImpl(bus);
  DCHECK_EQ(STUB_DBUS_CLIENT_IMPLEMENTATION, type);
  return new ShillProfileClientStubImpl();
}

}  // namespace chromeos
