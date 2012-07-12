// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_UMA_SESSION_STATS_
#define CLANK_NATIVE_FRAMEWORK_CHROME_UMA_SESSION_STATS_

#include <jni.h>

#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "chrome/common/metrics/metrics_log_base.h"
#include "content/browser/android/jni_helper.h"

class UserActionRateCounter;

// The native part of java UmaSessionStats class.
class UmaSessionStats {
 public:
  UmaSessionStats();

  void UmaResumeSession(JNIEnv* env, jobject obj);
  void UmaEndSession(JNIEnv* env, jobject obj);

 private:
  ~UmaSessionStats();
  void DeferredEndSession();
  // Start of the current session, used for UMA.
  base::TimeTicks session_start_time_;
  scoped_ptr<UserActionRateCounter> user_action_rate_counter_;

  DISALLOW_COPY_AND_ASSIGN(UmaSessionStats);
};

// Communicates with Java-side Clank to see if we're allowed to use the current
// network connection.
class ClankConnectionChecker : public ConnectionChecker {
 public:
  ClankConnectionChecker();
  virtual ~ClankConnectionChecker();

  virtual bool MayUpload() OVERRIDE;
};

// Registers the native methods through jni
bool RegisterUmaSessionStats(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_UMA_SESSION_STATS_
