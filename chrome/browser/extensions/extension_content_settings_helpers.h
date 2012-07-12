// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_CONTENT_SETTINGS_HELPERS_H__
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_CONTENT_SETTINGS_HELPERS_H__
#pragma once

#include <string>

#include "chrome/common/content_settings.h"
#include "chrome/common/content_settings_pattern.h"

namespace extension_content_settings_helpers {

// Parses an extension match pattern and returns a corresponding
// content settings pattern object.
// If |pattern_str| is invalid or can't be converted to a content settings
// pattern, |error| is set to the parsing error and an invalid pattern
// is returned.
ContentSettingsPattern ParseExtensionPattern(const std::string& pattern_str,
                                             std::string* error);

// Converts a content settings type string to the corresponding
// ContentSettingsType. Returns CONTENT_SETTINGS_TYPE_DEFAULT if the string
// didn't specify a valid content settings type.
ContentSettingsType StringToContentSettingsType(
    const std::string& content_type);
// Returns a string representation of a ContentSettingsType.
const char* ContentSettingsTypeToString(ContentSettingsType type);

// Converts a content setting string to the corresponding ContentSetting.
// Returns true if |setting_str| specifies a valid content setting,
// false otherwise.
bool StringToContentSetting(const std::string& setting_str,
                            ContentSetting* setting);
// Returns a string representation of a ContentSetting.
const char* ContentSettingToString(ContentSetting setting);

}  // namespace extension_content_settings_helpers

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_CONTENT_SETTINGS_HELPERS_H__
