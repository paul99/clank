// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_TEXT_INPUT_TYPE_H_
#define UI_BASE_IME_TEXT_INPUT_TYPE_H_
#pragma once

namespace ui {

// Intentionally keep sync with WebKit::WebTextInputType defined in:
// third_party/WebKit/Source/WebKit/chromium/public/WebTextInputType.h
enum TextInputType {
  // Input caret is not in an editable node, no input method shall be used.
  TEXT_INPUT_TYPE_NONE,

  // Input caret is in a normal editable node, any input method can be used.
  TEXT_INPUT_TYPE_TEXT,

  // Input caret is in a password box, an input method may be used only if
  // it's suitable for password input.
  TEXT_INPUT_TYPE_PASSWORD,

  // TODO(suzhe): Add more text input types when necessary, eg. Number, Date,
  // Email, URL, etc.
  TEXT_INPUT_TYPE_SEARCH,
  TEXT_INPUT_TYPE_EMAIL,
  TEXT_INPUT_TYPE_NUMBER,
  TEXT_INPUT_TYPE_TELEPHONE,
  TEXT_INPUT_TYPE_URL,

#if defined(OS_ANDROID)
  TEXT_INPUT_TYPE_TEXT_AREA,
  TEXT_INPUT_TYPE_DATE,
  TEXT_INPUT_TYPE_DATETIME,
  TEXT_INPUT_TYPE_DATETIMELOCAL,
  TEXT_INPUT_TYPE_MONTH,
  TEXT_INPUT_TYPE_TIME,
  TEXT_INPUT_TYPE_WEEK,

  // Input caret is in a contenteditable node (not an INPUT field).
  TEXT_INPUT_TYPE_CONTENT_EDITABLE,

  TEXT_INPUT_TYPE_MAX = TEXT_INPUT_TYPE_CONTENT_EDITABLE
#endif

};

}  // namespace ui

#endif  // UI_BASE_IME_TEXT_INPUT_TYPE_H_
