// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LOGIN_AUTO_LOGIN_H_
#define CHROME_BROWSER_UI_LOGIN_AUTO_LOGIN_H_
#pragma once

#include <string>

namespace auto_login {

// Create an auto-login infobar for the given x-auto-login header
// value |header_value|.  The tab for this infobar is identified by
// |render_process_id| and |tab_contents_id|.
// This function can be called on the IO thread.
void CreateAutoLogin(int render_process_id, int tab_contents_id,
                     const std::string& header_value);

}  // namespace auto_login

#endif  // CHROME_BROWSER_UI_LOGIN_AUTO_LOGIN_H_
