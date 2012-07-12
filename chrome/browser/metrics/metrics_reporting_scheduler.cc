// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/metrics_reporting_scheduler.h"

#include "base/compiler_specific.h"
#include "chrome/browser/metrics/metrics_service.h"

using base::Time;
using base::TimeDelta;

// The delay, in seconds, after startup before sending the first log message.
#if defined(OS_ANDROID)
// Sessions are more likely to be short on a mobile device, so handle the
// initial log quickly.
static const int kInitialUploadIntervalSeconds = 20;
#else
static const int kInitialUploadIntervalSeconds = 60;
#endif

// The delay, in seconds, between uploading when there are queued logs from
// previous sessions to send.
#if defined(OS_ANDROID)
static const int kUnsentLogsIntervalSeconds = 5;
#else
static const int kUnsentLogsIntervalSeconds = 15;
#endif

// Standard interval between log uploads, in seconds.
#if defined(OS_ANDROID)
static const int kStandardUploadIntervalSeconds = 5 * 60;  // Five minutes.
#else
static const int kStandardUploadIntervalSeconds = 30 * 60;  // Thirty minutes.
#endif

// When uploading metrics to the server fails, we progressively wait longer and
// longer before sending the next log. This backoff process helps reduce load
// on a server that is having issues.
// The following is the multiplier we use to expand that inter-log duration.
static const double kBackoffMultiplier = 1.1;

// The maximum backoff multiplier.
static const int kMaxBackoffMultiplier = 10;


MetricsReportingScheduler::MetricsReportingScheduler(
    const base::Closure& upload_callback)
    : upload_callback_(upload_callback),
      ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this)),
      upload_interval_(TimeDelta::FromSeconds(kInitialUploadIntervalSeconds)),
      running_(false),
      timer_pending_(false),
      callback_pending_(false) {
}

MetricsReportingScheduler::~MetricsReportingScheduler() {}

void MetricsReportingScheduler::Start() {
  running_ = true;
  ScheduleNextCallback();
}

void MetricsReportingScheduler::Stop() {
  running_ = false;
}

void MetricsReportingScheduler::UploadFinished(bool server_is_healthy,
                                               bool more_logs_remaining) {
  DCHECK(callback_pending_);
  callback_pending_ = false;
  // If the server is having issues, back off. Otherwise, reset to default
  // (unless there are more logs to send, in which case the next upload should
  // happen sooner).
  if (!server_is_healthy) {
    BackOffUploadInterval();
  } else if (more_logs_remaining) {
    upload_interval_ = TimeDelta::FromSeconds(kUnsentLogsIntervalSeconds);
  } else {
    upload_interval_ = TimeDelta::FromSeconds(kStandardUploadIntervalSeconds);
  }

  if (running_)
    ScheduleNextCallback();
}

void MetricsReportingScheduler::UploadCancelled() {
  DCHECK(callback_pending_);
  callback_pending_ = false;
  if (running_)
    ScheduleNextCallback();
}

void MetricsReportingScheduler::UploadCancelledDueToNetworkPermissions() {
  DCHECK(callback_pending_);
  BackOffUploadInterval();
  UploadCancelled();
}

void MetricsReportingScheduler::TriggerUpload() {
  timer_pending_ = false;
  callback_pending_ = true;
  upload_callback_.Run();
}

void MetricsReportingScheduler::ScheduleNextCallback() {
  DCHECK(running_);
  if (timer_pending_ || callback_pending_)
    return;

  timer_pending_ = true;

  MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&MetricsReportingScheduler::TriggerUpload,
                 weak_ptr_factory_.GetWeakPtr()),
      upload_interval_);
}

void MetricsReportingScheduler::BackOffUploadInterval() {
  DCHECK(kBackoffMultiplier > 1.0);
  upload_interval_ = TimeDelta::FromMicroseconds(
      static_cast<int64>(kBackoffMultiplier *
                         upload_interval_.InMicroseconds()));

  TimeDelta max_interval = kMaxBackoffMultiplier *
      TimeDelta::FromSeconds(kStandardUploadIntervalSeconds);
  if (upload_interval_ > max_interval || upload_interval_.InSeconds() < 0) {
    upload_interval_ = max_interval;
  }
}
