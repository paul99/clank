// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/demos/demo_framework.h"
#include "ppapi/demos/demo_log.h"

REGISTER_DEMO(Log);

Log::Log(PP_Instance instance) : pp::Instance(instance) {
  Debug(PP_LOGLEVEL_LOG, "Log::Log");
}
Log::~Log() {}

bool Log::Init(uint32_t argc, const char* argn[], const char* argv[]) {
  Debug(PP_LOGLEVEL_LOG, "Log::Init");
  return true;
}

void Log::Debug(PP_LogLevel_Dev level, const pp::Var& value) {
  const PPB_Console_Dev* console = reinterpret_cast<const PPB_Console_Dev*>(
      pp::Module::Get()->GetBrowserInterface(PPB_CONSOLE_DEV_INTERFACE));
  if (!console)
    return;
  console->Log(pp_instance(), level, value.pp_var());
}
