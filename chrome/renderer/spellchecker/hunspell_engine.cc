// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hunspell_engine.h"

#include <algorithm>
#include <iterator>

#include "base/metrics/histogram.h"
#include "base/time.h"
#include "chrome/common/spellcheck_common.h"
#include "chrome/common/spellcheck_messages.h"
#include "content/public/renderer/render_thread.h"
#include "third_party/hunspell/src/hunspell/hunspell.hxx"

using base::TimeTicks;
using content::RenderThread;

namespace {
  // Maximum length of words we actually check.
  // 64 is the observed limits for OSX system checker.
  const size_t kMaxCheckedLen = 64;

  // Maximum length of words we provide suggestions for.
  // 24 is the observed limits for OSX system checker.
  const size_t kMaxSuggestLen = 24;

  COMPILE_ASSERT(kMaxCheckedLen <= size_t(MAXWORDLEN), MaxCheckedLen_too_long);
  COMPILE_ASSERT(kMaxSuggestLen <= kMaxCheckedLen, MaxSuggestLen_too_long);
}

#if !defined(OS_MACOSX)
SpellingEngine* CreateNativeSpellingEngine() {
  return new HunspellEngine();
}
#endif

HunspellEngine::HunspellEngine()
    : file_(base::kInvalidPlatformFileValue),
      initialized_(false),
      dictionary_requested_(false) {
  // Wait till we check the first word before doing any initializing.
}

HunspellEngine::~HunspellEngine() {
}

void HunspellEngine::Init(base::PlatformFile file,
                          const std::vector<std::string>& custom_words) {
  initialized_ = true;
  hunspell_.reset();
  bdict_file_.reset();
  file_ = file;

  custom_words_.insert(custom_words_.end(),
                       custom_words.begin(), custom_words.end());

  // Delay the actual initialization of hunspell until it is needed.
}

void HunspellEngine::InitializeHunspell() {
  if (hunspell_.get())
    return;

  bdict_file_.reset(new file_util::MemoryMappedFile);

  if (bdict_file_->Initialize(file_)) {
    TimeTicks debug_start_time = base::Histogram::DebugNow();

    hunspell_.reset(
        new Hunspell(bdict_file_->data(), bdict_file_->length()));

    // Add custom words to Hunspell.
    AddWordsToHunspell(custom_words_);

    DHISTOGRAM_TIMES("Spellcheck.InitTime",
                     base::Histogram::DebugNow() - debug_start_time);
  } else {
    NOTREACHED() << "Could not mmap spellchecker dictionary.";
  }
}

void HunspellEngine::AddWordsToHunspell(const std::vector<std::string>& words) {
  std::string word;
  for (chrome::spellcheck_common::WordList::const_iterator it = words.begin();
       it != words.end();
       ++it) {
    word = *it;
    if (!word.empty() &&
        word.length() <=
            chrome::spellcheck_common::MAX_CUSTOM_DICTIONARY_WORD_BYTES) {
      hunspell_->add(word.c_str());
    }
  }
}

void HunspellEngine::RemoveWordsFromHunspell(
    const std::vector<std::string>& words) {
  std::string word;
  for (std::vector<std::string>::const_iterator it = words.begin();
       it != words.end();
       ++it) {
    word = *it;
    if (!word.empty() &&
        word.length() <=
            chrome::spellcheck_common::MAX_CUSTOM_DICTIONARY_WORD_BYTES) {
      hunspell_->remove(word.c_str());
    }
  }
}

bool HunspellEngine::CheckSpelling(const string16& word_to_check, int tag) {
  // Assume all words that cannot be checked are valid. Since Chrome can't
  // offer suggestions on them, either, there's no point in flagging them to
  // the user.
  bool word_correct = true;
  std::string word_to_check_utf8(UTF16ToUTF8(word_to_check));

  // Limit the size of checked words.
  if (word_to_check_utf8.length() <= kMaxCheckedLen) {
    // If |hunspell_| is NULL here, an error has occurred, but it's better
    // to check rather than crash.
    if (hunspell_.get()) {
      // |hunspell_->spell| returns 0 if the word is misspelled.
      word_correct = (hunspell_->spell(word_to_check_utf8.c_str()) != 0);
    }
  }

  return word_correct;
}

void HunspellEngine::FillSuggestionList(
    const string16& wrong_word,
    std::vector<string16>* optional_suggestions) {
  std::string wrong_word_utf8(UTF16ToUTF8(wrong_word));
  if (wrong_word_utf8.length() > kMaxSuggestLen)
    return;

  // If |hunspell_| is NULL here, an error has occurred, but it's better
  // to check rather than crash.
  // TODO(groby): Technically, it's not. We should track down the issue.
  if (!hunspell_.get())
    return;

  char** suggestions = NULL;
  int number_of_suggestions =
      hunspell_->suggest(&suggestions, wrong_word_utf8.c_str());

  // Populate the vector of WideStrings.
  for (int i = 0; i < number_of_suggestions; ++i) {
    if (i < chrome::spellcheck_common::kMaxSuggestions)
      optional_suggestions->push_back(UTF8ToUTF16(suggestions[i]));
    free(suggestions[i]);
  }
  if (suggestions != NULL)
    free(suggestions);
}

void HunspellEngine::OnCustomDictionaryChanged(
    const std::vector<std::string>& words_added,
    const std::vector<std::string>& words_removed) {
  if (!hunspell_.get()) {
    // Save it for later---add it when hunspell is initialized.
    custom_words_.insert(custom_words_.end(),
                         words_added.begin(),
                         words_added.end());
    // Remove words.
    std::vector<std::string> words_removed_copy(words_removed);
    std::sort(words_removed_copy.begin(), words_removed_copy.end());
    std::sort(custom_words_.begin(), custom_words_.end());
    std::vector<std::string> updated_custom_words;
    std::set_difference(custom_words_.begin(),
                        custom_words_.end(),
                        words_removed_copy.begin(),
                        words_removed_copy.end(),
                        std::back_inserter(updated_custom_words));
    std::swap(custom_words_, updated_custom_words);
  } else {
    AddWordsToHunspell(words_added);
    RemoveWordsFromHunspell(words_removed);
  }
}

bool HunspellEngine::InitializeIfNeeded() {
  if (!initialized_ && !dictionary_requested_) {
    // RenderThread will not exist in test.
    if (RenderThread::Get())
      RenderThread::Get()->Send(new SpellCheckHostMsg_RequestDictionary);
    dictionary_requested_ = true;
    return true;
  }

  // Don't initialize if hunspell is disabled.
  if (file_ != base::kInvalidPlatformFileValue)
    InitializeHunspell();

  return !initialized_;
}

bool HunspellEngine::IsEnabled() {
  return file_ != base::kInvalidPlatformFileValue;
}
