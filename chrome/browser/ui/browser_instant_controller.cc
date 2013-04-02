// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_instant_controller.h"

#include "base/prefs/pref_service.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/prefs/pref_registry_syncable.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/omnibox/location_bar.h"
#include "chrome/browser/ui/omnibox/omnibox_view.h"
#include "chrome/browser/ui/search/search.h"
#include "chrome/browser/ui/search/search_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/ntp/app_launcher_handler.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/notification_service.h"
#include "grit/theme_resources.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/sys_color_change_listener.h"

namespace {
const char* GetInstantPrefName(Profile* profile) {
  return chrome::search::IsInstantExtendedAPIEnabled(profile) ?
      prefs::kInstantExtendedEnabled : prefs::kInstantEnabled;
}
}

namespace chrome {

////////////////////////////////////////////////////////////////////////////////
// BrowserInstantController, public:

BrowserInstantController::BrowserInstantController(Browser* browser)
    : browser_(browser),
      instant_(ALLOW_THIS_IN_INITIALIZER_LIST(this),
               chrome::search::IsInstantExtendedAPIEnabled(profile())),
      instant_unload_handler_(browser),
      initialized_theme_info_(false),
      theme_area_height_(0) {
  profile_pref_registrar_.Init(profile()->GetPrefs());
  profile_pref_registrar_.Add(
      GetInstantPrefName(profile()),
      base::Bind(&BrowserInstantController::ResetInstant,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      prefs::kSearchSuggestEnabled,
      base::Bind(&BrowserInstantController::ResetInstant,
                 base::Unretained(this)));
  ResetInstant();
  browser_->search_model()->AddObserver(this);

#if defined(ENABLE_THEMES)
  // Listen for theme installation.
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
                 content::Source<ThemeService>(
                     ThemeServiceFactory::GetForProfile(profile())));
#endif  // defined(ENABLE_THEMES)
}

BrowserInstantController::~BrowserInstantController() {
  browser_->search_model()->RemoveObserver(this);
}

bool BrowserInstantController::IsInstantEnabled(Profile* profile) {
  return profile && !profile->IsOffTheRecord() && profile->GetPrefs() &&
         profile->GetPrefs()->GetBoolean(GetInstantPrefName(profile));
}

void BrowserInstantController::RegisterUserPrefs(
    PrefService* prefs,
    PrefRegistrySyncable* registry) {
  // TODO(joi): Get rid of the need for PrefService param above.
  registry->RegisterBooleanPref(prefs::kInstantConfirmDialogShown, false,
                                PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(prefs::kInstantEnabled, false,
                                PrefRegistrySyncable::SYNCABLE_PREF);

  search::InstantExtendedDefault instant_extended_default_setting =
      search::GetInstantExtendedDefaultSetting();

  bool instant_extended_value = true;
  switch (instant_extended_default_setting) {
    case search::INSTANT_FORCE_ON:
      break;
    case search::INSTANT_USE_EXISTING:
      instant_extended_value = prefs->GetBoolean(prefs::kInstantEnabled);
      break;
    case search::INSTANT_FORCE_OFF:
      instant_extended_value = false;
      break;
  }

  registry->RegisterBooleanPref(prefs::kInstantExtendedEnabled,
                                instant_extended_value,
                                PrefRegistrySyncable::SYNCABLE_PREF);
}

bool BrowserInstantController::MaybeSwapInInstantNTPContents(
    const GURL& url,
    content::WebContents* source_contents,
    content::WebContents** target_contents) {
  if (url != GURL(chrome::kChromeUINewTabURL))
    return false;

  scoped_ptr<content::WebContents> instant_ntp = instant_.ReleaseNTPContents();
  if (!instant_ntp)
    return false;

  *target_contents = instant_ntp.get();
  instant_ntp->GetController().PruneAllButActive();
  if (source_contents) {
    instant_ntp->GetController().CopyStateFromAndPrune(
        &source_contents->GetController());
    ReplaceWebContentsAt(
        browser_->tab_strip_model()->GetIndexOfWebContents(source_contents),
        instant_ntp.Pass());
  } else {
    // If |source_contents| is NULL, then the caller is responsible for
    // inserting instant_ntp into the tabstrip and will take ownership.
    ignore_result(instant_ntp.release());
  }
  return true;
}

bool BrowserInstantController::OpenInstant(WindowOpenDisposition disposition) {
  // Unsupported dispositions.
  if (disposition == NEW_BACKGROUND_TAB || disposition == NEW_WINDOW)
    return false;

  // The omnibox currently doesn't use other dispositions, so we don't attempt
  // to handle them. If you hit this DCHECK file a bug and I'll (sky) add
  // support for the new disposition.
  DCHECK(disposition == CURRENT_TAB ||
         disposition == NEW_FOREGROUND_TAB) << disposition;

  return instant_.CommitIfPossible(disposition == CURRENT_TAB ?
      INSTANT_COMMIT_PRESSED_ENTER : INSTANT_COMMIT_PRESSED_ALT_ENTER);
}

Profile* BrowserInstantController::profile() const {
  return browser_->profile();
}

void BrowserInstantController::CommitInstant(
    scoped_ptr<content::WebContents> preview,
    bool in_new_tab) {
  if (profile()->GetExtensionService()->IsInstalledApp(preview->GetURL())) {
    AppLauncherHandler::RecordAppLaunchType(
        extension_misc::APP_LAUNCH_OMNIBOX_INSTANT);
  }
  if (in_new_tab) {
    // TabStripModel takes ownership of |preview|.
    browser_->tab_strip_model()->AddWebContents(preview.release(), -1,
        instant_.last_transition_type(), TabStripModel::ADD_ACTIVE);
  } else {
    ReplaceWebContentsAt(
        browser_->tab_strip_model()->active_index(),
        preview.Pass());
  }
}

void BrowserInstantController::ReplaceWebContentsAt(
    int index,
    scoped_ptr<content::WebContents> new_contents) {
  DCHECK_NE(TabStripModel::kNoTab, index);
  content::WebContents* old_contents =
      browser_->tab_strip_model()->GetWebContentsAt(index);
  // TabStripModel takes ownership of |new_contents|.
  browser_->tab_strip_model()->ReplaceWebContentsAt(
      index, new_contents.release());
  // TODO(samarth): use scoped_ptr instead of comments to document ownership
  // transfer.
  // InstantUnloadHandler takes ownership of |old_contents|.
  instant_unload_handler_.RunUnloadListenersOrDestroy(old_contents, index);
}

void BrowserInstantController::SetInstantSuggestion(
    const InstantSuggestion& suggestion) {
  browser_->window()->GetLocationBar()->SetInstantSuggestion(suggestion);
}

gfx::Rect BrowserInstantController::GetInstantBounds() {
  return browser_->window()->GetInstantBounds();
}

void BrowserInstantController::InstantPreviewFocused() {
  // NOTE: This is only invoked on aura.
  browser_->window()->WebContentsFocused(instant_.GetPreviewContents());
}

void BrowserInstantController::FocusOmniboxInvisibly() {
  OmniboxView* omnibox_view = browser_->window()->GetLocationBar()->
      GetLocationEntry();
  omnibox_view->SetFocus();
  omnibox_view->model()->SetCaretVisibility(false);
}

content::WebContents* BrowserInstantController::GetActiveWebContents() const {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

void BrowserInstantController::ActiveTabChanged() {
  instant_.ActiveTabChanged();
}

void BrowserInstantController::TabDeactivated(content::WebContents* contents) {
  instant_.TabDeactivated(contents);
}

void BrowserInstantController::SetContentHeight(int height) {
  OnThemeAreaHeightChanged(height);
}

void BrowserInstantController::UpdateThemeInfoForPreview() {
  // Update theme background info and theme area height.
  // Initialize |theme_info| if necessary.
  // |OnThemeChanged| also updates theme area height if necessary.
  if (!initialized_theme_info_)
    OnThemeChanged(ThemeServiceFactory::GetForProfile(profile()));
  else
    OnThemeChanged(NULL);
}

void BrowserInstantController::OpenURLInCurrentTab(
    const GURL& url,
    content::PageTransition transition) {
  browser_->OpenURL(content::OpenURLParams(url,
                                           content::Referrer(),
                                           CURRENT_TAB,
                                           transition,
                                           false));
}

void BrowserInstantController::SetMarginSize(int start, int end) {
  instant_.SetMarginSize(start, end);
}

void BrowserInstantController::ResetInstant() {
  bool instant_enabled = IsInstantEnabled(profile());
  bool use_local_preview_only = profile()->IsOffTheRecord() ||
      (!instant_enabled &&
       !profile()->GetPrefs()->GetBoolean(prefs::kSearchSuggestEnabled));
  instant_.SetInstantEnabled(instant_enabled, use_local_preview_only);
}

////////////////////////////////////////////////////////////////////////////////
// BrowserInstantController, search::SearchModelObserver implementation:

void BrowserInstantController::ModeChanged(const search::Mode& old_mode,
                                           const search::Mode& new_mode) {
  // If mode is now |NTP|, send theme-related information to instant.
  if (new_mode.is_ntp())
    UpdateThemeInfoForPreview();

  instant_.SearchModeChanged(old_mode, new_mode);
}

////////////////////////////////////////////////////////////////////////////////
// BrowserInstantController, content::NotificationObserver implementation:

void BrowserInstantController::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
#if defined(ENABLE_THEMES)
  DCHECK_EQ(chrome::NOTIFICATION_BROWSER_THEME_CHANGED, type);
  OnThemeChanged(content::Source<ThemeService>(source).ptr());
#endif  // defined(ENABLE_THEMES)
}

void BrowserInstantController::OnThemeChanged(ThemeService* theme_service) {
  if (theme_service) {  // Get theme information from theme service.
    theme_info_ = ThemeBackgroundInfo();

    // Set theme background color.
    SkColor background_color =
        theme_service->GetColor(ThemeService::COLOR_NTP_BACKGROUND);
    if (gfx::IsInvertedColorScheme())
      background_color = color_utils::InvertColor(background_color);
    theme_info_.color_r = SkColorGetR(background_color);
    theme_info_.color_g = SkColorGetG(background_color);
    theme_info_.color_b = SkColorGetB(background_color);
    theme_info_.color_a = SkColorGetA(background_color);

    if (theme_service->HasCustomImage(IDR_THEME_NTP_BACKGROUND)) {
      // Set theme id for theme background image url.
      theme_info_.theme_id = theme_service->GetThemeID();

      // Set theme background image horizontal alignment.
      int alignment = 0;
      theme_service->GetDisplayProperty(ThemeService::NTP_BACKGROUND_ALIGNMENT,
                                        &alignment);
      if (alignment & ThemeService::ALIGN_LEFT) {
        theme_info_.image_horizontal_alignment = THEME_BKGRND_IMAGE_ALIGN_LEFT;
      } else if (alignment & ThemeService::ALIGN_RIGHT) {
        theme_info_.image_horizontal_alignment = THEME_BKGRND_IMAGE_ALIGN_RIGHT;
      } else {  // ALIGN_CENTER
        theme_info_.image_horizontal_alignment =
            THEME_BKGRND_IMAGE_ALIGN_CENTER;
      }

      // Set theme background image vertical alignment.
      if (alignment & ThemeService::ALIGN_TOP)
        theme_info_.image_vertical_alignment = THEME_BKGRND_IMAGE_ALIGN_TOP;
      else if (alignment & ThemeService::ALIGN_BOTTOM)
        theme_info_.image_vertical_alignment = THEME_BKGRND_IMAGE_ALIGN_BOTTOM;
      else // ALIGN_CENTER
        theme_info_.image_vertical_alignment = THEME_BKGRND_IMAGE_ALIGN_CENTER;

      // Set theme background image tiling.
      int tiling = 0;
      theme_service->GetDisplayProperty(ThemeService::NTP_BACKGROUND_TILING,
                                        &tiling);
      switch (tiling) {
        case ThemeService::NO_REPEAT:
            theme_info_.image_tiling = THEME_BKGRND_IMAGE_NO_REPEAT;
            break;
        case ThemeService::REPEAT_X:
            theme_info_.image_tiling = THEME_BKGRND_IMAGE_REPEAT_X;
            break;
        case ThemeService::REPEAT_Y:
            theme_info_.image_tiling = THEME_BKGRND_IMAGE_REPEAT_Y;
            break;
        case ThemeService::REPEAT:
            theme_info_.image_tiling = THEME_BKGRND_IMAGE_REPEAT;
            break;
      }

      // Set theme background image height.
      gfx::ImageSkia* image = theme_service->GetImageSkiaNamed(
          IDR_THEME_NTP_BACKGROUND);
      DCHECK(image);
      theme_info_.image_height = image->height();
    }

    initialized_theme_info_ = true;
  }

  DCHECK(initialized_theme_info_);

  if (browser_->search_model()->mode().is_ntp()) {
    instant_.ThemeChanged(theme_info_);

    // Theme area height is only sent to preview for non-top-aligned images;
    // new theme may have a different alignment that requires preview to know
    // theme area height.
    OnThemeAreaHeightChanged(theme_area_height_);
  }
}

void BrowserInstantController::OnThemeAreaHeightChanged(int height) {
  theme_area_height_ = height;

  // Notify preview only if mode is |NTP| and theme background image is not top-
  // aligned; top-aligned images don't need theme area height to determine which
  // part of the image overlay should draw, 'cos the origin is top-left.
  if (!browser_->search_model()->mode().is_ntp() ||
      theme_info_.theme_id.empty() ||
      theme_info_.image_vertical_alignment == THEME_BKGRND_IMAGE_ALIGN_TOP) {
    return;
  }
  instant_.ThemeAreaHeightChanged(theme_area_height_);
}

}  // namespace chrome
