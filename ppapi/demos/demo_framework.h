// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This demo framework allows for the easy creation of pepper plugins without
// modification to the build system.  It is based on the ppapi_tests framework
// but removes automation in favor of human interaction.  A single DemoInstance
// is capable of switching between multiple pp::Instances dynamically based on
// what is called for by the page.
//
// To create a plugin demo using this framework, write your pepper plugin as
// usual, with the following exceptions:
// - Do not implement a CreateModule method or pp::Module for your plugin.  This
//     is taken care of by DemoInstance.
// - Invoke the REGISTER_DEMO macro in your implementation file.  For example,
//     if your plugin is called Foo, you should have a line in your file which
//     reads:    REGISTER_DEMO(Foo)
//
// With these exceptions, demo plugins should work as normal.  When the
// demos.html page is loaded in the browser, it will list the registered demo
// plugins.  Upon clicking on the desired demo plugin, the page will reload with
// that plugin as the backend for DemoInstance.
//
// Since all plugins are loaded into the same web page, demos.html is built to
// be manipulated by plugins.  There are a couple of methods in DemoInstance
// which demonstrate the manupulation of the HTML in the header as well as above
// and below the plugin on the page.  This should be sufficient for any UI
// elements needed by the plugin.

#ifndef PPAPI_DEMOS_DEMO_FRAMEWORK_H_
#define PPAPI_DEMOS_DEMO_FRAMEWORK_H_

#include <string>
#include "ppapi/cpp/instance.h"

class DemoInstance : public pp::Instance {
 public:
  explicit DemoInstance(PP_Instance instance);

  // pp::Instance override.
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);

  // Passes the message_data through to the HandleMessage method on the
  // TestClass object that's associated with this instance.
  virtual void HandleMessage(const pp::Var& message_data);

  // This method replaces the HTML above the plugin
  void SetBodyUpperHTML(const pp::Var& html);

  // Appends the given error test to the console in the document.
  void LogError(const pp::Var& text);

  // pp::Instance methods - the DemoInstance calls these on the plugin demo
  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip);

  virtual void DidChangeFocus(bool has_focus);

  virtual bool HandleInputEvent(const pp::InputEvent& event);

  virtual bool HandleDocumentLoad(const pp::URLLoader& url_loader);

  //virtual pp::Var GetSelectedText(bool html);

 private:
  // Creates a new plugin demo for the given demo name, or NULL if there is no such
  // demo. Ownership is passed to the caller.
  pp::Instance* CaseForDemoName(const char* name);

  // Appends a list of available plugin demos to the console in the document.
  void LogAvailableDemos();

  // Owning pointer to the current plugin demo. Valid after Init has been called.
  pp::Instance* current_demo_;
  bool displayed_demos_;
};

// Implementation detail, used by DemoInstance to find plugin demos.
class PluginDemoFactory {
 public:
  typedef pp::Instance* (*Method)(PP_Instance instance);

  PluginDemoFactory(const char* name, Method method)
      : next_(head_),
        name_(name),
        method_(method) {
    head_ = this;
  }

 private:
  friend class DemoInstance;

  PluginDemoFactory* next_;
  const char* name_;
  Method method_;

  static PluginDemoFactory* head_;
};

// Use the REGISTER_DEMO macro in your plugin demo implementation file to
// register your plugin demo.  If your demo is named Foo, then add the
// following to foo.cc:
//
//   REGISTER_DEMO(Foo);
//
// This will cause your demo to be included in the set of known demos.
//
#define REGISTER_DEMO(name)                                            \
  static pp::Instance* name##_FactoryMethod(PP_Instance instance) {    \
    return new name(instance);                                         \
  }                                                                    \
  static PluginDemoFactory g_##name_factory(                           \
    #name, &name##_FactoryMethod                                       \
  )

#endif  // PPAPI_DEMOS_DEMO_FRAMEWORK_H_

