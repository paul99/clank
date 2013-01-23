// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_BUILD_INFO_H_
#define BASE_ANDROID_BUILD_INFO_H_

#include <jni.h>
#include <string>
#include "base/synchronization/lock.h"

namespace base {
namespace android {


// BuildInfo is a singleton class that stores android build and device
// information.

// It is also used to store the last java exception seen during JNI.
// TODO(nileshagrawal): Find a better place to store this info.
class BuildInfo {
 public:

  // Static factory method for getting the singleton BuildInfo instance.
  static BuildInfo* const GetInstance();

  ~BuildInfo() {};

  // We use const char* instead of std::strings because we need to have these
  // values available even if the process is in a crash state. Sadly
  // std::string.c_str() doesn't guarantee that memory won't be allocated when
  // it is called.
  const char* device;
  const char* model;
  const char* brand;
  const char* android_build_id;
  const char* android_build_fp;
  const char* package_version_code;
  const char* package_version_name;
  const int sdk_version_int;
  const char* java_exception_info;


  void set_java_exception_info(std::string info);

 private:
  static BuildInfo* instance_;
  static base::Lock lock_;

  BuildInfo(
      const char* device,
      const char* model,
      const char* brand,
      const char* android_build_id,
      const char* android_build_fp,
      const char* package_version_code,
      const char* package_version_name,
      const int sdk_version_int);

};

bool RegisterBuildInfo(JNIEnv* env);

}  // namespace android
}  // namespace base


#endif  // BASE_ANDROID_BUILD_INFO_H_
