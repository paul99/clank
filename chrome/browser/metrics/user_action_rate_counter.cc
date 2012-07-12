// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/user_action_rate_counter.h"

#include "base/metrics/histogram.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"

namespace {
// The minimum duration for a sample before histograms will be generated. This
// is to prevent the possibility of lots of very short logging sessions skewing
// all the histograms toward 0.
const float kMinTimeThreshold = 15.0;

// Histograms only record integers, so normalize to a large interval to
// compute the rate; this ensure that even one use over the logged interval will
// not get rounded to 0.
const int kSecondsPerNormalizedPeriod = 60 * 60;  // 1 hour.
}

// Use a range that gives good precision up to once per second.
#define UMA_USER_ACTION_RATE_COUNTS(name) UMA_HISTOGRAM_CUSTOM_COUNTS(\
    name, UsageRateForHistogram(name), 1, kSecondsPerNormalizedPeriod, 50)

UserActionRateCounter::UserActionRateCounter() : active_(false) {
  registrar_.Add(this, content::NOTIFICATION_USER_ACTION,
                 content::NotificationService::AllSources());
}

UserActionRateCounter::~UserActionRateCounter() {}

void UserActionRateCounter::Observe(int type,
                                    const content::NotificationSource& source,
                                    const content::NotificationDetails& details) {
  // TODO(nileshagrawal): Filter out incognito, as is done in MetricsService.
  DCHECK(type == content::NOTIFICATION_USER_ACTION);

  std::string action_name = *content::Details<const char*>(details).ptr();
  std::string histogram_name = HistogramNameForAction(action_name);
  std::map<std::string, unsigned int>::iterator entry =
      action_counts_.find(histogram_name);
  if (entry == action_counts_.end()) {
    action_counts_.insert(
        std::pair<std::string, unsigned int>(histogram_name, 1));
  } else {
    entry->second += 1;
  }
}

void UserActionRateCounter::SetCurrentlyActive(bool active) {
  if (active == active_)
    return;
  active_ = active;
  if (active)
    StartActivityTimer();
  else
    StopActivityTimer();
}

void UserActionRateCounter::StartActivityTimer() {
  activity_start_time_ = base::TimeTicks::Now();
}

void UserActionRateCounter::StopActivityTimer() {
  total_interval_ += base::TimeTicks::Now() - activity_start_time_;
}

// Creates histograms of the usage rate of tracked user actions.
void UserActionRateCounter::GenerateRateHistograms() {
  VLOG(1) << "Generating rate histograms for:" << action_counts_.size();
  // Toggle the timer to update the total interval.
  if (active_) {
    StopActivityTimer();
    StartActivityTimer();
  }
  // If too little time has elapsed, just keep all the data to include in the
  // next sample (assuming there is one; this may result in losing some rate
  // data about very short sessions, but that's okay for now.)
  // TODO(stuartmorgan): Revisit the handling once there is enough data about
  // average session lengths.
  if (total_interval_.InSecondsF() < kMinTimeThreshold)
    return;

  // Record all of the Mobile* actions.
  // TODO(stuartmorgan): Autogenerate this list from source code into a file.
  // Unfortunately this can't be turned into a loop, because UMA_HISTOGRAM_*
  // requires a constant name, so there must be an actual call per histogram.
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuLink");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuImage");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuText");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuOpenLink");
  UMA_USER_ACTION_RATE_COUNTS(
       "UserActionRate.MobileContextMenuOpenLinkInIncognito");
  UMA_USER_ACTION_RATE_COUNTS(
       "UserActionRate.MobileContextMenuOpenLinkInNewTab");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuViewImage");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuCopyImageLinkAddress");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuCopyLinkAddress");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuCopyLinkText");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuShareLink");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileContextMenuSaveImage");

  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuAddToBookmarks");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuAllBookmarks");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuCloseAllTabs");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuCloseTab");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuFeedback");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuFindInPage");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuFullscreen");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuNewIncognitoTab");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuNewTab");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuSettings");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuShare");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuShow");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuReload");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuBack");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileMenuForward");

  // TODO(nileshagrawal): Add RecordAction calls for the actions which are commented out.
  // UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileNTPShowBookmarks"); // Add call
  // UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileNTPShowMostVisited"); // add call
  // UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileNTPShowOpenTabs"); // add call
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileNTPSwitchToBookmarks");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileNTPSwitchToMostVisited");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileNTPSwitchToOpenTabs");

  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileNewTabOpened");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileOmniboxSearch");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileOmniboxVoiceSearch");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileStackViewCloseTab");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileStackViewSwipeCloseTab");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileTabClobbered");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileTabClosed");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileTabSwitched");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileSideSwipeFinished");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobilePageLoaded");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobilePageLoadedDesktopUserAgent");

  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileTabStripNewTab");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileTabStripCloseTab");

  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileToolbarBack");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileToolbarForward");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileToolbarReload");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileToolbarNewTab");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileToolbarShowStackView");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileToolbarToggleBookmark");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileToolbarShowMenu");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileToolbarStackViewNewTab");

  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileBeamCallbackSuccess");
  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileBeamInvalidAppState");

  UMA_USER_ACTION_RATE_COUNTS("UserActionRate.MobileBreakpadUploadAttempt");

  // Reset the counters.
  action_counts_.clear();
  total_interval_ = base::TimeDelta();
}

std::string UserActionRateCounter::HistogramNameForAction(
    const std::string& action_name) const {
  std::string histogram_name("UserActionRate.");
  histogram_name.append(action_name);
  return histogram_name;
}

int UserActionRateCounter::UsageRateForHistogram(
    const std::string& histogram_name) const {
  std::map<std::string, unsigned int>::const_iterator entry =
      action_counts_.find(histogram_name);
  unsigned int count = (entry == action_counts_.end()) ? 0 : entry->second;
  return count / total_interval_.InSecondsF() * kSecondsPerNormalizedPeriod;
}
