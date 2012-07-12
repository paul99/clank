// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_USER_ACTION_RATE_COUNTER_H_
#define CHROME_BROWSER_METRICS_USER_ACTION_RATE_COUNTER_H_

#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "base/time.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

// Note: This class is copied from https://chromereviews.googleplex.com/3488021/
// (A version of this will be upstreamed soon.)
//
// Counts the occurence of various user actions, and then records them as
// histograms of the usage rate over time.
// Since it logs user actions while it exists, it should only be instantiated
// while metrics recording is enabled.
class UserActionRateCounter : public content::NotificationObserver {
 public:
  UserActionRateCounter();
  virtual ~UserActionRateCounter();

  // Implementation of NotificationObserver
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Sets the current activity state of the application; should be called
  // whenever the application starts or stops being active.
  // No-op if |active| is the same as the last call.
  void SetCurrentlyActive(bool active);

  // Creates histograms of the usage rate of tracked user actions since the last
  // call.
  void GenerateRateHistograms();

 private:
  // Starts the activitiy timer.
  void StartActivityTimer();
  // Stops the activitiy timer, updating total_interval_ with the elapsed time.
  void StopActivityTimer();

  // Returns the name of the rate histogram for the given action.
  std::string HistogramNameForAction(const std::string& action_name) const;

  // Returns the usage rate for the action corresponding to |histogram_name|
  // over |total_interval_|, normalized for the histogram.
  int UsageRateForHistogram(const std::string& histogram_name) const;

  content::NotificationRegistrar registrar_;

  bool active_;

  base::TimeTicks activity_start_time_;
  base::TimeDelta total_interval_;

  std::map<std::string, unsigned int> action_counts_;
};

#endif  // CHROME_BROWSER_METRICS_USER_ACTION_RATE_COUNTER_H_
