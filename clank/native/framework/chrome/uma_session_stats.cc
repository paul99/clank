// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/uma_session_stats.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/threading/thread_restrictions.h"
#include "base/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/metrics_service.h"
#include "chrome/browser/metrics/user_action_rate_counter.h"
#include "content/public/browser/browser_thread.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/user_metrics.h"
#include "jni/uma_session_stats_jni.h"

using content::BrowserThread;

namespace {
// The UmaSessionStats::mNativeUmaSessionStats field.
UmaSessionStats* g_uma_session_stats = NULL;
const int kEndSessionDelayMs = 500;
}  // namespace

UmaSessionStats::UmaSessionStats() {
  user_action_rate_counter_.reset(new UserActionRateCounter());
}

UmaSessionStats::~UmaSessionStats() {
}

void UmaSessionStats::UmaResumeSession(JNIEnv* env, jobject obj) {
  DCHECK(g_browser_process);
  session_start_time_ = base::TimeTicks::Now();

  // Tell the metrics service that the application resumes.
  MetricsService* metrics = g_browser_process->metrics_service();
  if (metrics) {
    metrics->OnAppEnterForeground();
  }

  user_action_rate_counter_->SetCurrentlyActive(true);
}

void UmaSessionStats::UmaEndSession(JNIEnv* env, jobject obj) {
  DCHECK(g_browser_process);
  base::TimeDelta duration = base::TimeTicks::Now() - session_start_time_;
  UMA_HISTOGRAM_LONG_TIMES("Session.TotalDuration", duration);

  // Should be called before metrics service is notified.
  user_action_rate_counter_->SetCurrentlyActive(false);

  // This may generate a lot of histogram calls. Since this is called when the
  // main activity is paused, we want to return as soon as possible.
  // Post a delayed task on the UI thread and return (we do not need to
  // worry about the lifetime of "this", as the object is never destroyed).
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostDelayedTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&UmaSessionStats::DeferredEndSession, base::Unretained(this)),
      kEndSessionDelayMs);
}

void UmaSessionStats::DeferredEndSession() {
  user_action_rate_counter_->GenerateRateHistograms();
  // Tell the metrics service it was cleanly shutdown.
  MetricsService* metrics = g_browser_process->metrics_service();
  if (metrics) {
    metrics->OnAppEnterBackground();
  }
}

ClankConnectionChecker::ClankConnectionChecker() {}

ClankConnectionChecker::~ClankConnectionChecker() {}

// Called by MetricsService to see if it can send logs on the current network.
bool ClankConnectionChecker::MayUpload() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_UmaSessionStats_mayUploadOnCurrentConnection(
      env, base::android::GetApplicationContext());
}

// Starts/stops the MetricsService as necessary.
static void UpdateMetricsServiceState(JNIEnv* env, jobject obj,
    jboolean may_record) {
  // TODO: We shouldn't be doing this on the UI thread. Allow it for now until
  // http://b/5882865 is fixed.
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  MetricsService* metrics = g_browser_process->metrics_service();
  DCHECK(metrics);

  const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();
  if (parsed_command_line.HasSwitch(switches::kMetricsRecordingOnly)) {
    // For testing purposes.
    LOG(INFO) << "Metrics service started recording, but not reporting.";
    metrics->StartRecordingOnly();
  } else if (may_record) {
    if (!metrics->recording_active() || !metrics->reporting_active()) {
      LOG(INFO) << "Metrics service started.";
      metrics->Start();
    }
  } else {
    if (metrics->recording_active() || metrics->reporting_active()) {
      LOG(INFO) << "Metrics service stopped.";
      metrics->Stop();
    }
  }
}

// Renderer process crashed in the foreground.
static void LogRendererCrash(JNIEnv* env, jclass clazz, jboolean is_paused) {
  DCHECK(g_browser_process);

  if (!is_paused) {
    // Increment the renderer crash count in stability metrics.
    PrefService* pref = g_browser_process->local_state();
    DCHECK(pref);
    int value = pref->GetInteger(prefs::kStabilityRendererCrashCount);
    pref->SetInteger(prefs::kStabilityRendererCrashCount, value + 1);
  }

  // Note: When we are paused, any UI metric we increment may not make it to
  // the disk before we are killed. Treat the count below as a lower bound.
  content::RecordAction(content::UserMetricsAction("MobileRendererCrashed"));
}

static int Init(JNIEnv* env, jobject obj) {
  // We should have only one UmaSessionStats instance.
  DCHECK(!g_uma_session_stats);
  g_uma_session_stats = new UmaSessionStats();

  // Register with the MetricsService to make it consult us about network
  // connectivity.
  MetricsService* metrics = g_browser_process->metrics_service();
  DCHECK(metrics);
  metrics->set_connection_checker(new ClankConnectionChecker());

  return reinterpret_cast<jint>(g_uma_session_stats);
}

// Register native methods
bool RegisterUmaSessionStats(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
