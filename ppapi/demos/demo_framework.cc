// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/demos/demo_framework.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

PluginDemoFactory* PluginDemoFactory::head_ = NULL;

DemoInstance::DemoInstance(PP_Instance instance)
    : pp::Instance(instance),
      current_demo_(NULL),
      displayed_demos_(false) {
}

bool DemoInstance::Init(uint32_t argc,
                        const char* argn[],
                        const char* argv[]) {
  // Create the proper plugin demo from the argument.
  for (uint32_t i = 0; i < argc; i++) {
    if (std::strcmp(argn[i], "src") == 0) {
      if (argv[i][0] == '\0')
        break;
      current_demo_ = CaseForDemoName(argv[i]);
      if (!current_demo_)
        LogError(std::string("Unknown plugin demo ") + argv[i]);
      else if (!current_demo_->Init(argc, argn, argv))
        LogError(" Plugin could not initialize.");
      return true;
    }
  }
  return true;
}

void DemoInstance::HandleMessage(const pp::Var& message_data) {
  if (current_demo_)
    current_demo_->HandleMessage(message_data);
}

void DemoInstance::DidChangeView(const pp::Rect& position,
                                 const pp::Rect& clip) {
  if (current_demo_)
    current_demo_->DidChangeView(position, clip);
  else
    LogAvailableDemos();
}

pp::Instance* DemoInstance::CaseForDemoName(const char* name) {
  PluginDemoFactory* iter = PluginDemoFactory::head_;
  while (iter != NULL) {
    if (std::strcmp(name, iter->name_) == 0)
      return iter->method_(this->pp_instance());
    iter = iter->next_;
  }
  return NULL;
}

void DemoInstance::LogAvailableDemos() {
  if (displayed_demos_)
    return;

  // Print out a listing of all demos.
  std::vector<std::string> plugin_demos;
  PluginDemoFactory* iter = PluginDemoFactory::head_;
  while (iter != NULL) {
    plugin_demos.push_back(iter->name_);
    iter = iter->next_;
  }
  std::sort(plugin_demos.begin(), plugin_demos.end());

  std::string html;
  html.append("Available plugin demos: <dl>");
  for (size_t i = 0; i < plugin_demos.size(); ++i) {
    html.append("<a href='?demo=");
    html.append(plugin_demos[i]);
    html.append("'>");
    html.append(plugin_demos[i]);
    html.append("</a><br/>");
  }
  html.append("</dl>");
  SetBodyUpperHTML(html);
  displayed_demos_ = true;
}

void DemoInstance::LogError(const pp::Var& text) {
  std::string html;
  html.append("<span class=\"fail\">Error</span>: <span class=\"err_msg\">");
  html.append(text.AsString());
  html.append("</span>");
  SetBodyUpperHTML(html);
}

void DemoInstance::SetBodyUpperHTML(const pp::Var& html) {
  std::string msg = "ReplaceUpperBodyHTML:";
  msg.append(html.AsString());
  PostMessage(msg);
}

void DemoInstance::DidChangeFocus(bool has_focus) {
  if (current_demo_)
    current_demo_->DidChangeFocus(has_focus);
}

bool DemoInstance::HandleInputEvent(const pp::InputEvent& event) {
  if (current_demo_)
    return current_demo_->HandleInputEvent(event);
  return false;
}

bool DemoInstance::HandleDocumentLoad(const pp::URLLoader& url_loader) {
  if (current_demo_)
    return current_demo_->HandleDocumentLoad(url_loader);
  return false;
}

class Module : public pp::Module {
 public:
  Module() : pp::Module() {}
  virtual ~Module() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new DemoInstance(instance);
  }
};

namespace pp {

Module* CreateModule() {
  return new ::Module();
}

}  // namespace pp
