// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_DEMOS_DEMO_LOG_H_
#define PPAPI_DEMOS_DEMO_LOG_H_

#include "ppapi/c/dev/ppb_console_dev.h"

class Log : public pp::Instance {
 public:
  explicit Log(PP_Instance instance);
  virtual ~Log();

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);

  void Debug(PP_LogLevel_Dev level, const pp::Var& value);
};

#endif // PPAPI_DEMOS_DEMO_LOG_H_
