// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/instant/instant_controller.h"

#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autocomplete/autocomplete_provider.h"
#include "chrome/browser/google/google_util.h"
#include "chrome/browser/history/history_service.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/history_tab_helper.h"
#include "chrome/browser/instant/instant_ntp.h"
#include "chrome/browser/instant/instant_overlay.h"
#include "chrome/browser/instant/instant_tab.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/search_engines/template_url_service.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_instant_controller.h"
#include "chrome/browser/ui/search/search_tab_helper.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "net/base/escape.h"
#include "third_party/icu/public/common/unicode/normalizer2.h"

#if defined(TOOLKIT_VIEWS)
#include "ui/views/widget/widget.h"
#endif

namespace {

// An artificial delay (in milliseconds) we introduce before telling the Instant
// page about the new omnibox bounds, in cases where the bounds shrink. This is
// to avoid the page jumping up/down very fast in response to bounds changes.
const int kUpdateBoundsDelayMS = 1000;

// The maximum number of times we'll load a non-Instant-supporting search engine
// before we give up and blacklist it for the rest of the browsing session.
const int kMaxInstantSupportFailures = 10;

// For reporting events of interest.
enum InstantControllerEvent {
  INSTANT_CONTROLLER_EVENT_URL_ADDED_TO_BLACKLIST = 0,
  INSTANT_CONTROLLER_EVENT_URL_REMOVED_FROM_BLACKLIST = 1,
  INSTANT_CONTROLLER_EVENT_URL_BLOCKED_BY_BLACKLIST = 2,
  INSTANT_CONTROLLER_EVENT_MAX = 3,
};

void RecordEventHistogram(InstantControllerEvent event) {
  UMA_HISTOGRAM_ENUMERATION("Instant.InstantControllerEvent",
                            event,
                            INSTANT_CONTROLLER_EVENT_MAX);
}

void AddSessionStorageHistogram(bool extended_enabled,
                                const content::WebContents* tab1,
                                const content::WebContents* tab2) {
  base::HistogramBase* histogram = base::BooleanHistogram::FactoryGet(
      std::string("Instant.SessionStorageNamespace") +
          (extended_enabled ? "_Extended" : "_Instant"),
      base::HistogramBase::kUmaTargetedHistogramFlag);
  const content::SessionStorageNamespaceMap& session_storage_map1 =
      tab1->GetController().GetSessionStorageNamespaceMap();
  const content::SessionStorageNamespaceMap& session_storage_map2 =
      tab2->GetController().GetSessionStorageNamespaceMap();
  bool is_session_storage_the_same =
      session_storage_map1.size() == session_storage_map2.size();
  if (is_session_storage_the_same) {
    // The size is the same, so let's check that all entries match.
    for (content::SessionStorageNamespaceMap::const_iterator
             it1 = session_storage_map1.begin(),
             it2 = session_storage_map2.begin();
         it1 != session_storage_map1.end() && it2 != session_storage_map2.end();
         ++it1, ++it2) {
      if (it1->first != it2->first || it1->second != it2->second) {
        is_session_storage_the_same = false;
        break;
      }
    }
  }
  histogram->AddBoolean(is_session_storage_the_same);
}

string16 Normalize(const string16& str) {
  UErrorCode status = U_ZERO_ERROR;
  const icu::Normalizer2* normalizer =
      icu::Normalizer2::getInstance(NULL, "nfkc_cf", UNORM2_COMPOSE, status);
  if (normalizer == NULL || U_FAILURE(status))
    return str;
  icu::UnicodeString norm_str(normalizer->normalize(
      icu::UnicodeString(FALSE, str.c_str(), str.size()), status));
  if (U_FAILURE(status))
    return str;
  return string16(norm_str.getBuffer(), norm_str.length());
}

bool NormalizeAndStripPrefix(string16* text, const string16& prefix) {
  string16 norm_prefix = Normalize(prefix);
  string16 norm_text = Normalize(*text);
  if (norm_prefix.size() <= norm_text.size() &&
      norm_text.compare(0, norm_prefix.size(), norm_prefix) == 0) {
    *text = norm_text.erase(0, norm_prefix.size());
    return true;
  }
  return false;
}

// For TOOLKIT_VIEWS, the top level widget is always focused. If the focus
// change originated in views determine the child Widget from the view that is
// being focused.
gfx::NativeView GetViewGainingFocus(gfx::NativeView view_gaining_focus) {
#if defined(TOOLKIT_VIEWS)
  views::Widget* widget = view_gaining_focus ?
      views::Widget::GetWidgetForNativeView(view_gaining_focus) : NULL;
  if (widget) {
    views::FocusManager* focus_manager = widget->GetFocusManager();
    if (focus_manager && focus_manager->is_changing_focus() &&
        focus_manager->GetFocusedView() &&
        focus_manager->GetFocusedView()->GetWidget())
      return focus_manager->GetFocusedView()->GetWidget()->GetNativeView();
  }
#endif
  return view_gaining_focus;
}

// Returns true if |view| is the top-level contents view or a child view in the
// view hierarchy of |contents|.
bool IsViewInContents(gfx::NativeView view, content::WebContents* contents) {
  content::RenderWidgetHostView* rwhv = contents->GetRenderWidgetHostView();
  if (!view || !rwhv)
    return false;

  gfx::NativeView tab_view = contents->GetNativeView();
  if (view == rwhv->GetNativeView() || view == tab_view)
    return true;

  // Walk up the view hierarchy to determine if the view is a subview of the
  // WebContents view (such as a windowed plugin or http auth dialog).
  while (view) {
    view = platform_util::GetParent(view);
    if (view == tab_view)
      return true;
  }

  return false;
}

bool IsFullHeight(const InstantModel& model) {
  return model.height() == 100 && model.height_units() == INSTANT_SIZE_PERCENT;
}

bool IsContentsFrom(const InstantPage* page,
                    const content::WebContents* contents) {
  return page && (page->contents() == contents);
}

}  // namespace

// static
const char* InstantController::kLocalOmniboxPopupURL =
    "chrome://local-omnibox-popup/local-omnibox-popup.html";

InstantController::InstantController(chrome::BrowserInstantController* browser,
                                     bool extended_enabled)
    : browser_(browser),
      extended_enabled_(extended_enabled),
      instant_enabled_(false),
      use_local_preview_only_(true),
      model_(ALLOW_THIS_IN_INITIALIZER_LIST(this)),
      last_omnibox_text_has_inline_autocompletion_(false),
      last_verbatim_(false),
      last_transition_type_(content::PAGE_TRANSITION_LINK),
      last_match_was_search_(false),
      omnibox_focus_state_(OMNIBOX_FOCUS_NONE),
      start_margin_(0),
      end_margin_(0),
      allow_preview_to_show_search_suggestions_(false) {
}

InstantController::~InstantController() {
}

bool InstantController::Update(const AutocompleteMatch& match,
                               const string16& user_text,
                               const string16& full_text,
                               size_t selection_start,
                               size_t selection_end,
                               bool verbatim,
                               bool user_input_in_progress,
                               bool omnibox_popup_is_open,
                               bool escape_pressed,
                               bool is_keyword_search) {
  if (!extended_enabled_ && !instant_enabled_)
    return false;

  LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
      "Update: %s user_text='%s' full_text='%s' selection_start=%d "
      "selection_end=%d verbatim=%d typing=%d popup=%d escape_pressed=%d "
      "is_keyword_search=%d",
      AutocompleteMatch::TypeToString(match.type).c_str(),
      UTF16ToUTF8(user_text).c_str(), UTF16ToUTF8(full_text).c_str(),
      static_cast<int>(selection_start), static_cast<int>(selection_end),
      verbatim, user_input_in_progress, omnibox_popup_is_open, escape_pressed,
      is_keyword_search));

  // TODO(dhollowa): Complete keyword match UI.  For now just hide suggestions.
  // http://crbug.com/153932.  Note, this early escape is happens prior to the
  // DCHECKs below because |user_text| and |full_text| have different semantics
  // when keyword search is in effect.
  if (is_keyword_search) {
    if (instant_tab_)
      instant_tab_->Update(string16(), 0, 0, true);
    else
      HideOverlay();
    last_match_was_search_ = false;
    return false;
  }

  // If the popup is open, the user has to be typing.
  DCHECK(!omnibox_popup_is_open || user_input_in_progress);

  // If the popup is closed, there should be no inline autocompletion.
  DCHECK(omnibox_popup_is_open || user_text.empty() || user_text == full_text)
      << user_text << "|" << full_text;

  // If there's no text in the omnibox, the user can't have typed any.
  DCHECK(!full_text.empty() || user_text.empty()) << user_text;

  // If the user isn't typing, and the popup is closed, there can't be any
  // user-typed text.
  DCHECK(user_input_in_progress || omnibox_popup_is_open || user_text.empty())
      << user_text;

  // The preview is being clicked and will commit soon. Don't change anything.
  // TODO(sreeram): Add a browser test for this.
  if (overlay_ && overlay_->is_pointer_down_from_activate())
    return false;

  // In non-extended mode, SearchModeChanged() is never called, so fake it. The
  // mode is set to "disallow suggestions" here, so that if one of the early
  // "return false" conditions is hit, suggestions will be disallowed. If the
  // query is sent to the overlay, the mode is set to "allow" further below.
  if (!extended_enabled_)
    search_mode_.mode = chrome::search::Mode::MODE_DEFAULT;

  last_match_was_search_ = AutocompleteMatch::IsSearchType(match.type) &&
                           !user_text.empty();

  // In non extended mode, Instant is disabled for URLs and keyword mode.
  if (!extended_enabled_ &&
      (!last_match_was_search_ ||
       match.type == AutocompleteMatch::SEARCH_OTHER_ENGINE)) {
    HideOverlay();
    return false;
  }

  // If we have an |instant_tab_| use it, else ensure we have a overlay that is
  // current or is using local preview.
  if (!instant_tab_ && !(overlay_ && overlay_->IsUsingLocalPreview()) &&
      !EnsureOverlayIsCurrent(false)) {
    HideOverlay();
    return false;
  }

  if (extended_enabled_) {
    if (!omnibox_popup_is_open) {
      if (!user_input_in_progress) {
        // If the user isn't typing and the omnibox popup is closed, it means a
        // regular navigation, tab-switch or the user hitting Escape.
        if (instant_tab_) {
          // The user is on a search results page. It may be showing results for
          // a partial query the user typed before they hit Escape. Send the
          // omnibox text to the page to restore the original results.
          //
          // In a tab switch, |instant_tab_| won't have updated yet, so it may
          // be pointing to the previous tab (which was a search results page).
          // Ensure we don't send the omnibox text to a random webpage (the new
          // tab), by comparing the old and new WebContents.
          if (escape_pressed &&
              instant_tab_->contents() == browser_->GetActiveWebContents()) {
            instant_tab_->Submit(full_text);
          }
        } else if (!full_text.empty()) {
          // If |full_text| is empty, the user is on the NTP. The preview may
          // be showing custom NTP content; hide only if that's not the case.
          HideOverlay();
        }
      } else if (full_text.empty()) {
        // The user is typing, and backspaced away all omnibox text. Clear
        // |last_omnibox_text_| so that we don't attempt to set suggestions.
        last_omnibox_text_.clear();
        last_suggestion_ = InstantSuggestion();
        if (instant_tab_) {
          // On a search results page, tell it to clear old results.
          instant_tab_->Update(string16(), 0, 0, true);
        } else if (search_mode_.is_origin_ntp()) {
          // On the NTP, tell the preview to clear old results. Don't hide the
          // preview so it can show a blank page or logo if it wants.
          overlay_->Update(string16(), 0, 0, true);
        } else {
          HideOverlay();
        }
      } else {
        // The user switched to a tab with partial text already in the omnibox.
        HideOverlay();

        // The new tab may or may not be a search results page; we don't know
        // since SearchModeChanged() hasn't been called yet. If it later turns
        // out to be, we should store |full_text| now, so that if the user hits
        // Enter, we'll send the correct query to instant_tab_->Submit(). If the
        // partial text is not a query (|last_match_was_search_| is false), we
        // won't Submit(), so no need to worry about that.
        last_omnibox_text_ = full_text;
        last_suggestion_ = InstantSuggestion();
      }
      return false;
    } else if (full_text.empty()) {
      // The user typed a solitary "?". Same as the backspace case above.
      last_omnibox_text_.clear();
      last_suggestion_ = InstantSuggestion();
      if (instant_tab_)
        instant_tab_->Update(string16(), 0, 0, true);
      else if (search_mode_.is_origin_ntp())
        overlay_->Update(string16(), 0, 0, true);
      else
        HideOverlay();
      return false;
    }
  } else if (!omnibox_popup_is_open || full_text.empty()) {
    // In the non-extended case, hide the preview as long as the user isn't
    // actively typing a non-empty query.
    HideOverlay();
    return false;
  }

  last_omnibox_text_has_inline_autocompletion_ = user_text != full_text;

  // If the user continues typing the same query as the suggested text is
  // showing, reuse the suggestion (but only for INSTANT_COMPLETE_NEVER).
  bool reused_suggestion = false;
  if (last_suggestion_.behavior == INSTANT_COMPLETE_NEVER &&
      !last_omnibox_text_has_inline_autocompletion_) {
    if (StartsWith(last_omnibox_text_, full_text, false)) {
      // The user is backspacing away characters.
      last_suggestion_.text.insert(0, last_omnibox_text_, full_text.size(),
          last_omnibox_text_.size() - full_text.size());
      reused_suggestion = true;
    } else if (StartsWith(full_text, last_omnibox_text_, false)) {
      // The user is typing forward. Normalize any added characters.
      reused_suggestion = NormalizeAndStripPrefix(&last_suggestion_.text,
          string16(full_text, last_omnibox_text_.size()));
    }
  }
  if (!reused_suggestion)
    last_suggestion_ = InstantSuggestion();

  last_omnibox_text_ = full_text;

  if (!extended_enabled_) {
    // In non-extended mode, the query is verbatim if there's any selection
    // (including inline autocompletion) or if the cursor is not at the end.
    verbatim = verbatim || selection_start != selection_end ||
                           selection_start != full_text.size();
  }
  last_verbatim_ = verbatim;

  last_transition_type_ = match.transition;
  url_for_history_ = match.destination_url;

  // Allow search suggestions. In extended mode, SearchModeChanged() will set
  // this, but it's not called in non-extended mode, so fake it.
  if (!extended_enabled_)
    search_mode_.mode = chrome::search::Mode::MODE_SEARCH_SUGGESTIONS;

  if (instant_tab_) {
    instant_tab_->Update(user_text, selection_start, selection_end, verbatim);
  } else {
    if (first_interaction_time_.is_null())
      first_interaction_time_ = base::Time::Now();
    allow_preview_to_show_search_suggestions_ = true;

    // For extended mode, if the loader is not ready at this point, switch over
    // to a backup loader.
    if (extended_enabled_ && !overlay_->supports_instant() &&
        !overlay_->IsUsingLocalPreview() && browser_->GetActiveWebContents()) {
      CreateOverlay(kLocalOmniboxPopupURL, browser_->GetActiveWebContents());
    }

    overlay_->Update(extended_enabled_ ? user_text : full_text,
                     selection_start, selection_end, verbatim);
  }

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_INSTANT_CONTROLLER_UPDATED,
      content::Source<InstantController>(this),
      content::NotificationService::NoDetails());

  // We don't have new suggestions yet, but we can either reuse the existing
  // suggestion or reset the existing "gray text".
  browser_->SetInstantSuggestion(last_suggestion_);

  return true;
}

scoped_ptr<content::WebContents> InstantController::ReleaseNTPContents() {
  if (!extended_enabled_ || !ntp_)
    return scoped_ptr<content::WebContents>(NULL);

  LOG_INSTANT_DEBUG_EVENT(this, "ReleaseNTPContents");

  scoped_ptr<content::WebContents> ntp_contents = ntp_->ReleaseContents();
  ntp_.reset();
  ResetNTP();
  return ntp_contents.Pass();
}

// TODO(tonyg): This method only fires when the omnibox bounds change. It also
// needs to fire when the preview bounds change (e.g.: open/close info bar).
void InstantController::SetPopupBounds(const gfx::Rect& bounds) {
  if (!extended_enabled_ && !instant_enabled_)
    return;

  if (popup_bounds_ == bounds)
    return;

  popup_bounds_ = bounds;
  if (popup_bounds_.height() > last_popup_bounds_.height()) {
    update_bounds_timer_.Stop();
    SendPopupBoundsToPage();
  } else if (!update_bounds_timer_.IsRunning()) {
    update_bounds_timer_.Start(FROM_HERE,
        base::TimeDelta::FromMilliseconds(kUpdateBoundsDelayMS), this,
        &InstantController::SendPopupBoundsToPage);
  }
}

void InstantController::SetMarginSize(int start, int end) {
  if (!extended_enabled_ || (start_margin_ == start && end_margin_ == end))
    return;

  start_margin_ = start;
  end_margin_ = end;
  if (overlay_)
    overlay_->SetMarginSize(start_margin_, end_margin_);
  if (ntp_)
    ntp_->SetMarginSize(start_margin_, end_margin_);
  if (instant_tab_)
    instant_tab_->SetMarginSize(start_margin_, end_margin_);
}

void InstantController::HandleAutocompleteResults(
    const std::vector<AutocompleteProvider*>& providers) {
  if (!extended_enabled_)
    return;

  if (!instant_tab_ && !overlay_)
    return;

  DVLOG(1) << "AutocompleteResults:";
  std::vector<InstantAutocompleteResult> results;
  for (ACProviders::const_iterator provider = providers.begin();
       provider != providers.end(); ++provider) {
    for (ACMatches::const_iterator match = (*provider)->matches().begin();
         match != (*provider)->matches().end(); ++match) {
      InstantAutocompleteResult result;
      result.provider = UTF8ToUTF16((*provider)->GetName());
      result.type = UTF8ToUTF16(AutocompleteMatch::TypeToString(match->type));
      result.description = match->description;
      result.destination_url = UTF8ToUTF16(match->destination_url.spec());
      result.transition = match->transition;
      result.relevance = match->relevance;
      DVLOG(1) << "    " << result.relevance << " " << result.type << " "
               << result.provider << " " << result.destination_url << " '"
               << result.description << "' " <<  result.transition;
      results.push_back(result);
    }
  }
  LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
      "HandleAutocompleteResults: total_results=%d",
      static_cast<int>(results.size())));

  if (instant_tab_)
    instant_tab_->SendAutocompleteResults(results);
  else
    overlay_->SendAutocompleteResults(results);
}

bool InstantController::OnUpOrDownKeyPressed(int count) {
  if (!extended_enabled_)
    return false;

  if (!instant_tab_ && !overlay_)
    return false;

  if (instant_tab_)
    instant_tab_->UpOrDownKeyPressed(count);
  else
    overlay_->UpOrDownKeyPressed(count);

  return true;
}

content::WebContents* InstantController::GetPreviewContents() const {
  return overlay_ ? overlay_->contents() : NULL;
}

bool InstantController::IsPreviewingSearchResults() const {
  return model_.mode().is_search_suggestions() && IsFullHeight(model_) &&
         (last_match_was_search_ ||
          last_suggestion_.behavior == INSTANT_COMPLETE_NEVER);
}

bool InstantController::CommitIfPossible(InstantCommitType type) {
  if (!extended_enabled_ && !instant_enabled_)
    return false;

  LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
      "CommitIfPossible: type=%d last_omnibox_text_='%s' "
      "last_match_was_search_=%d instant_tab_=%d", type,
      UTF16ToUTF8(last_omnibox_text_).c_str(), last_match_was_search_,
      instant_tab_ != NULL));

  // If we are on an already committed search results page, send a submit event
  // to the page, but otherwise, nothing else to do.
  if (instant_tab_) {
    if (type == INSTANT_COMMIT_PRESSED_ENTER &&
        (last_match_was_search_ ||
         last_suggestion_.behavior == INSTANT_COMPLETE_NEVER)) {
      instant_tab_->Submit(last_omnibox_text_);
      instant_tab_->contents()->Focus();
      return true;
    }
    return false;
  }

  if (!IsPreviewingSearchResults() && type != INSTANT_COMMIT_NAVIGATED)
    return false;

  // There may re-entrance here, from the call to browser_->CommitInstant below,
  // which can cause a TabDeactivated notification which gets back here.
  // In this case, overlay_->ReleaseContents() was called already.
  if (!GetPreviewContents())
    return false;

  // Never commit the local omnibox.
  if (overlay_->IsUsingLocalPreview())
    return false;

  if (type == INSTANT_COMMIT_FOCUS_LOST)
    overlay_->Cancel(last_omnibox_text_);
  else if (type != INSTANT_COMMIT_NAVIGATED &&
           type != INSTANT_COMMIT_CLICKED_QUERY_SUGGESTION)
    overlay_->Submit(last_omnibox_text_);

  scoped_ptr<content::WebContents> preview = overlay_->ReleaseContents();

  if (extended_enabled_) {
    // Consider what's happening:
    //   1. The user has typed a query in the omnibox and committed it (either
    //      by pressing Enter or clicking on the preview).
    //   2. We commit the preview to the tab strip, and tell the page.
    //   3. The page will update the URL hash fragment with the query terms.
    // After steps 1 and 3, the omnibox will show the query terms. However, if
    // the URL we are committing at step 2 doesn't already have query terms, it
    // will flash for a brief moment as a plain URL. So, avoid that flicker by
    // pretending that the plain URL is actually the typed query terms.
    // TODO(samarth,beaudoin): Instead of this hack, we should add a new field
    // to NavigationEntry to keep track of what the correct query, if any, is.
    content::NavigationEntry* entry =
        preview->GetController().GetVisibleEntry();
    std::string url = entry->GetVirtualURL().spec();
    if (!google_util::IsInstantExtendedAPIGoogleSearchUrl(url) &&
        google_util::IsGoogleDomainUrl(url, google_util::ALLOW_SUBDOMAIN,
                                       google_util::ALLOW_NON_STANDARD_PORTS)) {
      // Hitting ENTER searches for what the user typed, so use
      // last_omnibox_text_. Clicking on the overlay commits what is currently
      // showing, so add in the gray text in that case.
      std::string query(UTF16ToUTF8(last_omnibox_text_));
      if (type != INSTANT_COMMIT_PRESSED_ENTER) {
        query += UTF16ToUTF8(last_suggestion_.text);
        // Update |last_omnibox_text_| so that the controller commits the proper
        // query if the user focuses the omnibox and presses Enter.
        last_omnibox_text_ += last_suggestion_.text;
      }
      entry->SetVirtualURL(GURL(
          url + "#q=" +
          net::EscapeQueryParamValue(query, true)));
      chrome::search::SearchTabHelper::FromWebContents(preview.get())->
          NavigationEntryUpdated();
    }
  }

  // If the preview page has navigated since the last Update(), we need to add
  // the navigation to history ourselves. Else, the page will navigate after
  // commit, and it will be added to history in the usual manner.
  const history::HistoryAddPageArgs& last_navigation =
      overlay_->last_navigation();
  if (!last_navigation.url.is_empty()) {
    content::NavigationEntry* entry = preview->GetController().GetActiveEntry();

    // The last navigation should be the same as the active entry if the overlay
    // is in search mode. During navigation, the active entry could have
    // changed since DidCommitProvisionalLoadForFrame is called after the entry
    // is changed.
    // TODO(shishir): Should we commit the last navigation for
    // INSTANT_COMMIT_NAVIGATED.
    DCHECK(type == INSTANT_COMMIT_NAVIGATED ||
           last_navigation.url == entry->GetURL());

    // Add the page to history.
    HistoryTabHelper* history_tab_helper =
        HistoryTabHelper::FromWebContents(preview.get());
    history_tab_helper->UpdateHistoryForNavigation(last_navigation);

    // Update the page title.
    history_tab_helper->UpdateHistoryPageTitle(*entry);
  }

  // Add a fake history entry with a non-Instant search URL, so that search
  // terms extraction (for autocomplete history matches) works.
  HistoryService* history = HistoryServiceFactory::GetForProfile(
      Profile::FromBrowserContext(preview->GetBrowserContext()),
      Profile::EXPLICIT_ACCESS);
  if (history) {
    history->AddPage(url_for_history_, base::Time::Now(), NULL, 0, GURL(),
                     history::RedirectList(), last_transition_type_,
                     history::SOURCE_BROWSED, false);
  }

  preview->GetController().PruneAllButActive();

  if (type != INSTANT_COMMIT_PRESSED_ALT_ENTER) {
    content::WebContents* active_tab = browser_->GetActiveWebContents();
    AddSessionStorageHistogram(extended_enabled_, active_tab, preview.get());
    preview->GetController().CopyStateFromAndPrune(
        &active_tab->GetController());
  }

  // Save notification source before we release the preview.
  content::Source<content::WebContents> notification_source(preview.get());

  browser_->CommitInstant(preview.Pass(),
                          type == INSTANT_COMMIT_PRESSED_ALT_ENTER);

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_INSTANT_COMMITTED,
      notification_source,
      content::NotificationService::NoDetails());

  // Hide explicitly. See comments in HideOverlay() for why.
  model_.SetPreviewState(chrome::search::Mode(), 0, INSTANT_SIZE_PERCENT);

  // Delay deletion as we could've gotten here from an InstantOverlay method.
  MessageLoop::current()->DeleteSoon(FROM_HERE, overlay_.release());

  // Try to create another overlay immediately so that it is ready for the next
  // user interaction.
  EnsureOverlayIsCurrent(false);

  LOG_INSTANT_DEBUG_EVENT(this, "Committed");
  return true;
}

void InstantController::OmniboxFocusChanged(
    OmniboxFocusState state,
    OmniboxFocusChangeReason reason,
    gfx::NativeView view_gaining_focus) {
  LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
      "OmniboxFocusChanged: %d to %d for reason %d", omnibox_focus_state_,
      state, reason));

  OmniboxFocusState old_focus_state = omnibox_focus_state_;
  omnibox_focus_state_ = state;
  if (!extended_enabled_ && !instant_enabled_)
    return;

  // Tell the page if the key capture mode changed unless the focus state
  // changed because of TYPING. This is because in that case, the browser hasn't
  // really stopped capturing key strokes.
  //
  // (More practically, if we don't do this check, the page would receive
  // onkeycapturechange before the corresponding onchange, and the page would
  // have no way of telling whether the keycapturechange happened because of
  // some actual user action or just because they started typing.)
  if (extended_enabled_ && GetPreviewContents() &&
      reason != OMNIBOX_FOCUS_CHANGE_TYPING) {
    const bool is_key_capture_enabled =
        omnibox_focus_state_ == OMNIBOX_FOCUS_INVISIBLE;
    if (overlay_)
      overlay_->KeyCaptureChanged(is_key_capture_enabled);
    if (instant_tab_)
      instant_tab_->KeyCaptureChanged(is_key_capture_enabled);
  }

  // If focus went from outside the omnibox to the omnibox, preload the default
  // search engine, in anticipation of the user typing a query. If the reverse
  // happened, commit or discard the preview.
  if (state != OMNIBOX_FOCUS_NONE && old_focus_state == OMNIBOX_FOCUS_NONE) {
    // On explicit user actions, ignore the Instant blacklist.
    EnsureOverlayIsCurrent(reason == OMNIBOX_FOCUS_CHANGE_EXPLICIT);
  } else if (state == OMNIBOX_FOCUS_NONE &&
             old_focus_state != OMNIBOX_FOCUS_NONE) {
    OmniboxLostFocus(view_gaining_focus);
  }
}

void InstantController::SearchModeChanged(
    const chrome::search::Mode& old_mode,
    const chrome::search::Mode& new_mode) {
  if (!extended_enabled_)
    return;

  LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
      "SearchModeChanged: [origin:mode] %d:%d to %d:%d", old_mode.origin,
      old_mode.mode, new_mode.origin, new_mode.mode));

  search_mode_ = new_mode;
  if (!new_mode.is_search_suggestions())
    HideOverlay();

  if (overlay_)
    overlay_->SearchModeChanged(new_mode);

  ResetInstantTab();
}

void InstantController::ActiveTabChanged() {
  if (!extended_enabled_ && !instant_enabled_)
    return;

  LOG_INSTANT_DEBUG_EVENT(this, "ActiveTabChanged");

  // When switching tabs, always hide the preview.
  HideOverlay();

  if (extended_enabled_)
    ResetInstantTab();
}

void InstantController::TabDeactivated(content::WebContents* contents) {
  LOG_INSTANT_DEBUG_EVENT(this, "TabDeactivated");
  if (extended_enabled_ && !contents->IsBeingDestroyed())
    CommitIfPossible(INSTANT_COMMIT_FOCUS_LOST);
}

void InstantController::SetInstantEnabled(bool instant_enabled,
                                          bool use_local_preview_only) {
  LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
      "SetInstantEnabled: instant_enabled=%d, use_local_preview_only=%d",
      instant_enabled, use_local_preview_only));

  // Non extended mode does not care about |use_local_preview_only|.
  if (instant_enabled == instant_enabled_ &&
      (!extended_enabled_ ||
       use_local_preview_only == use_local_preview_only_)) {
    return;
  }

  instant_enabled_ = instant_enabled;
  use_local_preview_only_ = use_local_preview_only;
  HideInternal();
  overlay_.reset();
  if (extended_enabled_ || instant_enabled_)
    EnsureOverlayIsCurrent(false);
  if (extended_enabled_)
    ResetNTP();
  if (instant_tab_)
    instant_tab_->SetDisplayInstantResults(instant_enabled_);
}

void InstantController::ThemeChanged(const ThemeBackgroundInfo& theme_info) {
  if (!extended_enabled_)
    return;

  if (overlay_)
    overlay_->SendThemeBackgroundInfo(theme_info);
  if (ntp_)
    ntp_->SendThemeBackgroundInfo(theme_info);
  if (instant_tab_)
    instant_tab_->SendThemeBackgroundInfo(theme_info);
}

void InstantController::ThemeAreaHeightChanged(int height) {
  if (!extended_enabled_)
    return;

  if (overlay_)
    overlay_->SendThemeAreaHeight(height);
  if (ntp_)
    ntp_->SendThemeAreaHeight(height);
  if (instant_tab_)
    instant_tab_->SendThemeAreaHeight(height);
}

void InstantController::SwappedOverlayContents() {
  model_.SetPreviewContents(GetPreviewContents());
}

void InstantController::FocusedOverlayContents() {
#if defined(USE_AURA)
  // On aura the omnibox only receives a focus lost if we initiate the focus
  // change. This does that.
  if (!model_.mode().is_default())
    browser_->InstantPreviewFocused();
#endif
}

void InstantController::ReloadOverlayIfStale() {
  // The local popup is never stale.
  if (overlay_ && overlay_->IsUsingLocalPreview())
    return;

  // If the preview is showing or the omnibox has focus, don't delete the
  // overlay. It will get refreshed the next time the preview is hidden or the
  // omnibox loses focus.
  if ((!overlay_ || overlay_->is_stale()) &&
      omnibox_focus_state_ == OMNIBOX_FOCUS_NONE &&
      model_.mode().is_default()) {
    overlay_.reset();
    EnsureOverlayIsCurrent(false);
  }
}

void InstantController::LogDebugEvent(const std::string& info) const {
  DVLOG(1) << info;

  debug_events_.push_front(std::make_pair(
      base::Time::Now().ToInternalValue(), info));
  static const size_t kMaxDebugEventSize = 2000;
  if (debug_events_.size() > kMaxDebugEventSize)
    debug_events_.pop_back();
}

// TODO(shishir): We assume that the WebContent's current RenderViewHost is the
// RenderViewHost being created which is not always true. Fix this.
void InstantController::InstantPageRenderViewCreated(
    const content::WebContents* contents) {
  if (!extended_enabled_)
    return;

  // Update theme info so that the page picks it up.
  browser_->UpdateThemeInfoForPreview();

  // Ensure the searchbox API has the correct initial state.
  if (IsContentsFrom(overlay(), contents)) {
    overlay_->SetDisplayInstantResults(instant_enabled_);
    overlay_->SearchModeChanged(search_mode_);
    overlay_->KeyCaptureChanged(
        omnibox_focus_state_ == OMNIBOX_FOCUS_INVISIBLE);
    overlay_->SetMarginSize(start_margin_, end_margin_);
    overlay_->InitializeFonts();
  } else if (IsContentsFrom(ntp(), contents)) {
    ntp_->SetDisplayInstantResults(instant_enabled_);
    ntp_->SetMarginSize(start_margin_, end_margin_);
    ntp_->InitializeFonts();
  } else {
    NOTREACHED();
  }
}

void InstantController::InstantSupportDetermined(
    const content::WebContents* contents,
    bool supports_instant) {
  if (IsContentsFrom(instant_tab(), contents)) {
    if (!supports_instant)
      MessageLoop::current()->DeleteSoon(FROM_HERE, instant_tab_.release());
  } else if (IsContentsFrom(ntp(), contents)) {
    if (supports_instant)
      RemoveFromBlacklist(ntp_->instant_url());
    else
      BlacklistAndResetNTP();

    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_INSTANT_NTP_SUPPORT_DETERMINED,
        content::Source<InstantController>(this),
        content::NotificationService::NoDetails());

  } else if (IsContentsFrom(overlay(), contents)) {
    if (supports_instant)
      RemoveFromBlacklist(overlay_->instant_url());
    else
      BlacklistAndResetOverlay();

    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_INSTANT_OVERLAY_SUPPORT_DETERMINED,
        content::Source<InstantController>(this),
        content::NotificationService::NoDetails());
  }
}

void InstantController::InstantPageRenderViewGone(
    const content::WebContents* contents) {
  if (IsContentsFrom(overlay(), contents))
    BlacklistAndResetOverlay();
  else if (IsContentsFrom(ntp(), contents))
    BlacklistAndResetNTP();
  else
    NOTREACHED();
}

void InstantController::InstantPageAboutToNavigateMainFrame(
    const content::WebContents* contents,
    const GURL& url) {
  DCHECK(IsContentsFrom(overlay(), contents));

  // If the page does not yet support instant, we allow redirects and other
  // navigations to go through since the instant URL can redirect - e.g. to
  // country specific pages.
  if (!overlay_->supports_instant())
    return;

  GURL instant_url(overlay_->instant_url());

  // If we are navigating to the instant URL, do nothing.
  if (url == instant_url)
    return;

  // Commit the navigation if either:
  //  - The page is in NTP mode (so it could only navigate on a user click) or
  //  - The page is not in NTP mode and we are navigating to a URL with a
  //    different host or path than the instant URL. This enables the instant
  //    page when it is showing search results to change the query parameters
  //    and fragments of the URL without it navigating.
  if (model_.mode().is_ntp() ||
      (url.host() != instant_url.host() || url.path() != instant_url.path())) {
    CommitIfPossible(INSTANT_COMMIT_NAVIGATED);
  }
}

void InstantController::SetSuggestions(
    const content::WebContents* contents,
    const std::vector<InstantSuggestion>& suggestions) {
  LOG_INSTANT_DEBUG_EVENT(this, "SetSuggestions");

  // Ignore if the message is from an unexpected source.
  if (IsContentsFrom(ntp(), contents))
    return;
  if (instant_tab_ && !IsContentsFrom(instant_tab(), contents))
    return;
  if (IsContentsFrom(overlay(), contents) &&
      !allow_preview_to_show_search_suggestions_)
    return;

  InstantSuggestion suggestion;
  if (!suggestions.empty())
    suggestion = suggestions[0];

  if (instant_tab_ && search_mode_.is_search_results() &&
      suggestion.behavior == INSTANT_COMPLETE_REPLACE) {
    // Update |last_omnibox_text_| so that the controller commits the proper
    // query if the user focuses the omnibox and presses Enter.
    last_omnibox_text_ = suggestion.text;
    last_suggestion_ = InstantSuggestion();
    last_match_was_search_ = suggestion.type == INSTANT_SUGGESTION_SEARCH;
    // This means a committed page in state search called setValue(). We should
    // update the omnibox to reflect what the search page says.
    browser_->SetInstantSuggestion(suggestion);
    return;
  }

  // Ignore if we are not currently accepting search suggestions.
  if (!search_mode_.is_search_suggestions() || last_omnibox_text_.empty())
    return;

  if (suggestion.behavior == INSTANT_COMPLETE_REPLACE) {
    // We don't get an Update() when changing the omnibox due to a REPLACE
    // suggestion (so that we don't inadvertently cause the preview to change
    // what it's showing, as the user arrows up/down through the page-provided
    // suggestions). So, update these state variables here.
    last_omnibox_text_ = suggestion.text;
    last_suggestion_ = InstantSuggestion();
    last_match_was_search_ = suggestion.type == INSTANT_SUGGESTION_SEARCH;
    LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
        "ReplaceSuggestion text='%s' type=%d",
        UTF16ToUTF8(suggestion.text).c_str(), suggestion.type));
    browser_->SetInstantSuggestion(suggestion);
  } else {
    bool is_valid_suggestion = true;

    // Suggestion text should be a full URL for URL suggestions, or the
    // completion of a query for query suggestions.
    if (suggestion.type == INSTANT_SUGGESTION_URL) {
      // If the suggestion is not a valid URL, perhaps it's something like
      // "foo.com". Try prefixing "http://". If it still isn't valid, drop it.
      if (!GURL(suggestion.text).is_valid()) {
        suggestion.text.insert(0, ASCIIToUTF16("http://"));
        if (!GURL(suggestion.text).is_valid())
          is_valid_suggestion = false;
      }
    } else if (StartsWith(suggestion.text, last_omnibox_text_, true)) {
      // The user typed an exact prefix of the suggestion.
      suggestion.text.erase(0, last_omnibox_text_.size());
    } else if (!NormalizeAndStripPrefix(&suggestion.text, last_omnibox_text_)) {
      // Unicode normalize and case-fold the user text and suggestion. If the
      // user text is a prefix, suggest the normalized, case-folded completion;
      // for instance, if the user types 'i' and the suggestion is 'INSTANT',
      // suggest 'nstant'. Otherwise, the user text really isn't a prefix, so
      // suggest nothing.
      is_valid_suggestion = false;
    }

    // Don't suggest gray text if there already was inline autocompletion.
    // http://crbug.com/162303
    if (suggestion.behavior == INSTANT_COMPLETE_NEVER &&
        last_omnibox_text_has_inline_autocompletion_)
      is_valid_suggestion = false;

    // Don't allow inline autocompletion if the query was verbatim.
    if (suggestion.behavior == INSTANT_COMPLETE_NOW && last_verbatim_)
      is_valid_suggestion = false;

    if (is_valid_suggestion) {
      last_suggestion_ = suggestion;
      LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
          "SetInstantSuggestion: text='%s' behavior=%d",
          UTF16ToUTF8(suggestion.text).c_str(), suggestion.behavior));
      browser_->SetInstantSuggestion(suggestion);
    } else {
      last_suggestion_ = InstantSuggestion();
    }
  }

  // Extended mode pages will call ShowOverlay() when they are ready.
  if (!extended_enabled_)
    ShowOverlay(INSTANT_SHOWN_QUERY_SUGGESTIONS, 100, INSTANT_SIZE_PERCENT);
}

void InstantController::ShowInstantPreview(const content::WebContents* contents,
                                           InstantShownReason reason,
                                           int height,
                                           InstantSizeUnits units) {
  if (extended_enabled_ && IsContentsFrom(overlay(), contents))
    ShowOverlay(reason, height, units);
}

void InstantController::StartCapturingKeyStrokes(
    const content::WebContents* contents) {
  if (!extended_enabled_)
    return;

  DCHECK(IsContentsFrom(instant_tab(), contents));
  browser_->FocusOmniboxInvisibly();
}

void InstantController::StopCapturingKeyStrokes(
    content::WebContents* contents) {
  // Nothing to do if omnibox doesn't have invisible focus.
  if (!extended_enabled_ || omnibox_focus_state_ != OMNIBOX_FOCUS_INVISIBLE)
    return;

  DCHECK(IsContentsFrom(instant_tab(), contents));
  contents->Focus();
}

void InstantController::NavigateToURL(const content::WebContents* contents,
                                      const GURL& url,
                                      content::PageTransition transition) {
  // TODO(samarth): handle case where contents are no longer "active" (e.g. user
  // has switched tabs).
  if (!extended_enabled_)
    return;
  if (overlay_)
    HideOverlay();
  browser_->OpenURLInCurrentTab(url, transition);
}

void InstantController::OmniboxLostFocus(gfx::NativeView view_gaining_focus) {
  // If the preview is showing custom NTP content, don't hide it, commit it
  // (no matter where the user clicked) or try to recreate it.
  if (model_.mode().is_ntp())
    return;

  // If the preview is not showing at all, recreate it if it's stale.
  if (model_.mode().is_default()) {
    ReloadOverlayIfStale();
    MaybeSwitchToRemoteOverlay();
    return;
  }

  // The preview is showing search suggestions. If GetPreviewContents() is NULL,
  // we are in the commit path. Don't do anything.
  if (!GetPreviewContents())
    return;

#if defined(OS_MACOSX)
  // TODO(sreeram): See if Mac really needs this special treatment.
  if (!overlay_->is_pointer_down_from_activate())
    HideOverlay();
#else
  if (IsViewInContents(GetViewGainingFocus(view_gaining_focus),
                       overlay_->contents()))
    CommitIfPossible(INSTANT_COMMIT_FOCUS_LOST);
  else
    HideOverlay();
#endif
}

void InstantController::ResetNTP() {
  ntp_.reset();
  std::string instant_url;
  if (!GetInstantURL(browser_->profile(), false, &instant_url))
    return;

  ntp_.reset(new InstantNTP(this, instant_url));
  ntp_->InitContents(browser_->profile(), browser_->GetActiveWebContents(),
                     base::Bind(&InstantController::ResetNTP,
                                base::Unretained(this)));
}

bool InstantController::EnsureOverlayIsCurrent(bool ignore_blacklist) {
  // If there's no active tab, the browser is closing.
  const content::WebContents* active_tab = browser_->GetActiveWebContents();
  if (!active_tab)
    return false;

  Profile* profile = Profile::FromBrowserContext(
      active_tab->GetBrowserContext());
  std::string instant_url;
  if (!GetInstantURL(profile, ignore_blacklist, &instant_url)) {
    // If we are in extended mode, fallback to the local popup.
    if (extended_enabled_)
      instant_url = kLocalOmniboxPopupURL;
    else
      return false;
  }

  if (!overlay_ || overlay_->instant_url() != instant_url)
    CreateOverlay(instant_url, active_tab);

  return true;
}

void InstantController::CreateOverlay(const std::string& instant_url,
                                      const content::WebContents* active_tab) {
  HideInternal();
  overlay_.reset(new InstantOverlay(this, instant_url));
  overlay_->InitContents(browser_->profile(), active_tab);
  LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
      "CreateOverlay: instant_url='%s'", instant_url.c_str()));
}

void InstantController::MaybeSwitchToRemoteOverlay() {
  if (!overlay_ || omnibox_focus_state_ != OMNIBOX_FOCUS_NONE ||
      !model_.mode().is_default()) {
    return;
  }

  EnsureOverlayIsCurrent(false);
}

void InstantController::ResetInstantTab() {
  // Do not wire up the InstantTab if instant should only use local previews, to
  // prevent it from sending data to the page.
  if (!search_mode_.is_origin_default() && !use_local_preview_only_) {
    content::WebContents* active_tab = browser_->GetActiveWebContents();
    if (!instant_tab_ || active_tab != instant_tab_->contents()) {
      instant_tab_.reset(new InstantTab(this));
      instant_tab_->Init(active_tab);
      // Update theme info for this tab.
      browser_->UpdateThemeInfoForPreview();
      instant_tab_->SetDisplayInstantResults(instant_enabled_);
      instant_tab_->SetMarginSize(start_margin_, end_margin_);
      instant_tab_->InitializeFonts();
      instant_tab_->KeyCaptureChanged(
          omnibox_focus_state_ == OMNIBOX_FOCUS_INVISIBLE);
    }

    // Hide the |overlay_| since we are now using |instant_tab_| instead.
    HideOverlay();
  } else {
    instant_tab_.reset();
  }
}

void InstantController::HideOverlay() {
  HideInternal();
  ReloadOverlayIfStale();
  MaybeSwitchToRemoteOverlay();
}

void InstantController::HideInternal() {
  LOG_INSTANT_DEBUG_EVENT(this, "Hide");

  // If GetPreviewContents() returns NULL, either we're already in the desired
  // MODE_DEFAULT state, or we're in the commit path. For the latter, don't
  // change the state just yet; else we may hide the preview unnecessarily.
  // Instead, the state will be set correctly after the commit is done.
  if (GetPreviewContents()) {
    model_.SetPreviewState(chrome::search::Mode(), 0, INSTANT_SIZE_PERCENT);
    allow_preview_to_show_search_suggestions_ = false;

    // Send a message asking the preview to clear out old results.
    overlay_->Update(string16(), 0, 0, true);
  }

  // Clear the first interaction timestamp for later use.
  first_interaction_time_ = base::Time();
}

void InstantController::ShowOverlay(InstantShownReason reason,
                                    int height,
                                    InstantSizeUnits units) {
  // If we are on a committed search results page, the |overlay_| is not in use.
  if (instant_tab_)
    return;

  LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
      "Show: reason=%d height=%d units=%d", reason, height, units));

  // INSTANT_SHOWN_CUSTOM_NTP_CONTENT is no longer supported.
  // TODO(samarth): remove once the server has been updated.
  if (reason == INSTANT_SHOWN_CUSTOM_NTP_CONTENT)
    return;

  // Must have updated omnibox after the last HideOverlay() to show suggestions.
  if ((reason == INSTANT_SHOWN_QUERY_SUGGESTIONS ||
       reason == INSTANT_SHOWN_CLICKED_QUERY_SUGGESTION) &&
      !allow_preview_to_show_search_suggestions_)
    return;

  // The page is trying to hide itself. Hide explicitly (i.e., don't use
  // HideOverlay()) so that it can change its mind.
  if (height == 0) {
    model_.SetPreviewState(chrome::search::Mode(), 0, INSTANT_SIZE_PERCENT);
    return;
  }

  // If the preview is being shown for the first time since the user started
  // typing, record a histogram value.
  if (!first_interaction_time_.is_null() && model_.mode().is_default()) {
    base::TimeDelta delta = base::Time::Now() - first_interaction_time_;
    UMA_HISTOGRAM_TIMES("Instant.TimeToFirstShow", delta);
  }

  // Show at 100% height except in the following cases:
  // - The local omnibox popup is being loaded.
  // - Instant is disabled. The page needs to be able to show only a dropdown.
  // - The page wants to show custom NTP content.
  // - The page is over a website other than search or an NTP, and is not
  //   already showing at 100% height.
  if (overlay_->IsUsingLocalPreview() || !instant_enabled_ ||
      reason == INSTANT_SHOWN_CUSTOM_NTP_CONTENT ||
      (search_mode_.is_origin_default() && !IsFullHeight(model_)))
    model_.SetPreviewState(search_mode_, height, units);
  else
    model_.SetPreviewState(search_mode_, 100, INSTANT_SIZE_PERCENT);

  // If the user clicked on a query suggestion, also go ahead and commit the
  // overlay. This is necessary because if the overlay was partially visible
  // when the suggestion was clicked, the click itself would not commit the
  // overlay (because we're not full height).
  if (reason == INSTANT_SHOWN_CLICKED_QUERY_SUGGESTION)
    CommitIfPossible(INSTANT_COMMIT_CLICKED_QUERY_SUGGESTION);
}

void InstantController::SendPopupBoundsToPage() {
  if (last_popup_bounds_ == popup_bounds_ || !overlay_ ||
      overlay_->is_pointer_down_from_activate())
    return;

  last_popup_bounds_ = popup_bounds_;
  gfx::Rect preview_bounds = browser_->GetInstantBounds();
  gfx::Rect intersection = gfx::IntersectRects(popup_bounds_, preview_bounds);

  // Translate into window coordinates.
  if (!intersection.IsEmpty()) {
    intersection.Offset(-preview_bounds.origin().x(),
                        -preview_bounds.origin().y());
  }

  // In the current Chrome UI, these must always be true so they sanity check
  // the above operations. In a future UI, these may be removed or adjusted.
  // There is no point in sanity-checking |intersection.y()| because the omnibox
  // can be placed anywhere vertically relative to the preview (for example, in
  // Mac fullscreen mode, the omnibox is fully enclosed by the preview bounds).
  DCHECK_LE(0, intersection.x());
  DCHECK_LE(0, intersection.width());
  DCHECK_LE(0, intersection.height());

  overlay_->SetPopupBounds(intersection);
}

bool InstantController::GetInstantURL(Profile* profile,
                                      bool ignore_blacklist,
                                      std::string* instant_url) const {
  DCHECK(profile);
  instant_url->clear();

  if (extended_enabled_ && use_local_preview_only_) {
    *instant_url = kLocalOmniboxPopupURL;
    return true;
  }

  const TemplateURL* template_url = TemplateURLServiceFactory::GetForProfile(
      profile)->GetDefaultSearchProvider();

  if (!template_url) {
    LOG_INSTANT_DEBUG_EVENT(this, "GetInstantURL: No template URL");
    return false;
  }

  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kInstantURL))
    *instant_url = command_line->GetSwitchValueASCII(switches::kInstantURL);

  if (instant_url->empty()) {
    const TemplateURLRef& instant_url_ref = template_url->instant_url_ref();
    if (!instant_url_ref.IsValid()) {
      LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
          "GetInstantURL: TemplateRef invalid: url=%s",
          template_url->instant_url().c_str()));
      return false;
    }

    // Even if the URL template doesn't have search terms, it may have other
    // components (such as {google:baseURL}) that need to be replaced.
    *instant_url = instant_url_ref.ReplaceSearchTerms(
        TemplateURLRef::SearchTermsArgs(string16()));

    // Extended mode should always use HTTPS. TODO(sreeram): This section can be
    // removed if TemplateURLs supported "https://{google:host}/..." instead of
    // only supporting "{google:baseURL}...".
    if (extended_enabled_) {
      GURL url_obj(*instant_url);
      if (!url_obj.is_valid()) {
        LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
            "GetInstantURL: Instant URL invalid: url=%s",
            url_obj.possibly_invalid_spec().c_str()));
        return false;
      }

      // Extended mode won't work properly unless the TemplateURL supports the
      // param to enable it on the server.
      if (!template_url->HasSearchTermsReplacementKey(url_obj)) {
        LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
            "GetInstantURL: No search terms replacement key: url=%s",
            url_obj.spec().c_str()));
        return false;
      }

      if (!url_obj.SchemeIsSecure()) {
        std::string new_scheme = "https";
        std::string new_port = "443";
        GURL::Replacements secure;
        secure.SetSchemeStr(new_scheme);
        secure.SetPortStr(new_port);
        url_obj = url_obj.ReplaceComponents(secure);

        if (!url_obj.is_valid()) {
          LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
              "GetInstantURL: HTTPS URL invalid: url=%s",
              url_obj.possibly_invalid_spec().c_str()));
          return false;
        }

        *instant_url = url_obj.spec();
      }
    }
  }

  if (!ignore_blacklist) {
    std::map<std::string, int>::const_iterator iter =
        blacklisted_urls_.find(*instant_url);
    if (iter != blacklisted_urls_.end() &&
        iter->second > kMaxInstantSupportFailures) {
      RecordEventHistogram(INSTANT_CONTROLLER_EVENT_URL_BLOCKED_BY_BLACKLIST);
      LOG_INSTANT_DEBUG_EVENT(this, base::StringPrintf(
          "GetInstantURL: Instant URL blacklisted: url=%s",
          instant_url->c_str()));
      return false;
    }
  }

  return true;
}

void InstantController::BlacklistAndResetNTP() {
  ++blacklisted_urls_[ntp_->instant_url()];
  RecordEventHistogram(INSTANT_CONTROLLER_EVENT_URL_ADDED_TO_BLACKLIST);
  delete ntp_->ReleaseContents().release();
  MessageLoop::current()->DeleteSoon(FROM_HERE, ntp_.release());
  ResetNTP();
}

void InstantController::BlacklistAndResetOverlay() {
  ++blacklisted_urls_[overlay_->instant_url()];
  RecordEventHistogram(INSTANT_CONTROLLER_EVENT_URL_ADDED_TO_BLACKLIST);
  HideInternal();
  delete overlay_->ReleaseContents().release();
  MessageLoop::current()->DeleteSoon(FROM_HERE, overlay_.release());
  EnsureOverlayIsCurrent(false);
}

void InstantController::RemoveFromBlacklist(const std::string& url) {
  if (blacklisted_urls_.erase(url)) {
    RecordEventHistogram(INSTANT_CONTROLLER_EVENT_URL_REMOVED_FROM_BLACKLIST);
  }
}
