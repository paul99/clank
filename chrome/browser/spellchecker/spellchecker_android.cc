// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// If linux ever gains a platform specific spellchecker, it will be
// implemented here.

#include "base/logging.h"
#include "spellchecker_platform_engine.h"

namespace SpellCheckerPlatform {

bool SpellCheckerAvailable() {
  NOTIMPLEMENTED();
  return false;
}

bool PlatformSupportsLanguage(const std::string& current_language) {
  NOTIMPLEMENTED();
  return false;
}

void GetAvailableLanguages(std::vector<std::string>* spellcheck_languages) {
  spellcheck_languages->clear();
  NOTIMPLEMENTED();
}

bool SpellCheckerProvidesPanel() {
  NOTIMPLEMENTED();
  return false;
}

bool SpellingPanelVisible() {
  NOTIMPLEMENTED();
  return false;
}

void ShowSpellingPanel(bool show) {
  NOTIMPLEMENTED();
}

void UpdateSpellingPanelWithMisspelledWord(const string16& word) {
  NOTIMPLEMENTED();
}

void Init() {
  NOTIMPLEMENTED();
}

void SetLanguage(const std::string& lang_to_set) {
  NOTIMPLEMENTED();
}

bool CheckSpelling(const string16& word_to_check, int tag) {
  NOTIMPLEMENTED();
  return false;
}

void FillSuggestionList(const string16& wrong_word,
                        std::vector<string16>* optional_suggestions) {
  NOTIMPLEMENTED();
}

void AddWord(const string16& word) {
  NOTIMPLEMENTED();
}

void RemoveWord(const string16& word) {
  NOTIMPLEMENTED();
}

int GetDocumentTag() {
  NOTIMPLEMENTED();
  return 0;
}

void IgnoreWord(const string16& word) {
  NOTIMPLEMENTED();
}

void CloseDocumentWithTag(int tag) {
  NOTIMPLEMENTED();
}

void RequestTextCheck(int route_id,
                      int identifier,
                      int document_tag,
                      const string16& text,
                      BrowserMessageFilter* destination) {
  NOTIMPLEMENTED();
}

}  // namespace SpellCheckerPlatform
