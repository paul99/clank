// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/build_info.h"

#include <string>

#include "base/logging.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"

#include "jni/build_info_jni.h"

namespace base {
namespace android {

BuildInfo* BuildInfo::instance_ = NULL;

BuildInfo::BuildInfo(
    const char* device,
    const char* model,
    const char* brand,
    const char* android_build_id,
    const char* android_build_fp,
    const char* package_version_code,
    const char* package_version_name)
    : device(device),
      model(model),
      brand(brand),
      android_build_id(android_build_id),
      android_build_fp(android_build_fp),
      package_version_code(package_version_code),
      package_version_name(package_version_name),
      java_exception_info(NULL) {
}

BuildInfo* const BuildInfo::GetInstance() {
  if (!instance_) {
    JNIEnv* env = AttachCurrentThread();
    jobject app_context = GetApplicationContext();

    // The const char* pointers initialized below will be owned by the
    // resultant BuildInfo.
    std::string device_str =
        ConvertJavaStringToUTF8(Java_BuildInfo_getDevice(env));
    const char* device = strdup(device_str.c_str());

    std::string model_str =
        ConvertJavaStringToUTF8(Java_BuildInfo_getDeviceModel(env));
    const char* model = strdup(model_str.c_str());

    std::string brand_str =
        ConvertJavaStringToUTF8(Java_BuildInfo_getBrand(env));
    const char* brand = strdup(brand_str.c_str());

    std::string android_build_id_str =
        ConvertJavaStringToUTF8(Java_BuildInfo_getAndroidBuildId(env));
    const char* android_build_id = strdup(android_build_id_str.c_str());

    std::string android_build_fp_str =
        ConvertJavaStringToUTF8(
            Java_BuildInfo_getAndroidBuildFingerprint(env));
    const char* android_build_fp = strdup(android_build_fp_str.c_str());

    std::string package_version_code_str =
        ConvertJavaStringToUTF8(Java_BuildInfo_getPackageVersionCode(
            env, app_context));
    const char* package_version_code =
        strdup(package_version_code_str.c_str());

    std::string package_version_name_str =
        ConvertJavaStringToUTF8(
            Java_BuildInfo_getPackageVersionName(env, app_context));
    const char* package_version_name =
        strdup(package_version_name_str.c_str());

    instance_ = new BuildInfo(
        device,
        model,
        brand,
        android_build_id,
        android_build_fp,
        package_version_code,
        package_version_name);

    LOG(INFO) << "BuildInfo instance initialized with"
              << " device=" << instance_->device
              << " model=" << instance_->model
              << " brand=" << instance_->brand
              << " android_build_id=" << instance_->android_build_id
              << " android_build_fp=" << instance_->android_build_fp
              << " package_version_code=" << instance_->package_version_code
              << " package_version_name=" << instance_->package_version_name;
  }
  return instance_;
}

void BuildInfo::set_java_exception_info(std::string info) {
  DCHECK(!java_exception_info) << "info should be set only once.";
  // Restrict exception info to 1024 bytes.
  java_exception_info = strndup(info.c_str(), 1024);
}

bool RegisterBuildInfo(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace android
}  // namespace base
