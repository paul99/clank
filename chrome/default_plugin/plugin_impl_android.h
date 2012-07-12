// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_DEFAULT_PLUGIN_PLUGIN_IMPL_ANDROID_H_
#define CHROME_DEFAULT_PLUGIN_PLUGIN_IMPL_ANDROID_H_
#pragma once

#include <string>

#include "base/logging.h"
#include "third_party/npapi/bindings/npapi.h"

// Provides the plugin installation functionality. This class is
// instantiated with the information like the mime type of the
// target plugin, the display mode, etc.
//
// TODO(tedbo): This is currently a dummy implementation based on the
// Linux gtk PluginInstallerImpl.
class PluginInstallerImpl {
 public:
  // mode is the plugin instantiation mode, i.e. whether it is a full
  // page plugin (NP_FULL) or an embedded plugin (NP_EMBED)
  explicit PluginInstallerImpl(int16 mode) { }
  virtual ~PluginInstallerImpl() { }

  // Initializes the plugin with the instance information, mime type
  // and the list of parameters passed down to the plugin from the webpage.
  //
  // Parameters:
  // module_handle
  //   The handle to the dll in which this object is instantiated.
  // instance
  //   The plugins opaque instance handle.
  // mime_type
  //   Identifies the third party plugin which would be eventually installed.
  // argc
  //   Indicates the count of arguments passed in from the webpage.
  // argv
  //   Pointer to the arguments.
  // Returns true on success.
  bool Initialize(void* module_handle, NPP instance, NPMIMEType mime_type,
                  int16 argc, char* argn[], char* argv[]) {
    NOTIMPLEMENTED();
    return false;
  }

  // Informs the plugin of its window information.
  //
  // Parameters:
  // window_info
  //   The window info passed to npapi.
  bool NPP_SetWindow(NPWindow* window_info) {
    NOTIMPLEMENTED();
    return false;
  }

  // Destroys the install dialog.
  void Shutdown() {
    NOTIMPLEMENTED();
  }

  // Initializes the plugin download stream.
  //
  // Parameters:
  // stream
  //   Pointer to the new stream being created.
  void NewStream(NPStream* stream) {
    NOTIMPLEMENTED();
  }

  // Uninitializes the plugin download stream.
  //
  // Parameters:
  // stream
  //   Pointer to the stream being destroyed.
  // reason
  //   Indicates why the stream is being destroyed.
  //
  void DestroyStream(NPStream* stream, NPError reason) {
    NOTIMPLEMENTED();
  }

  // Determines whether the plugin is ready to accept data.
  // We only accept data when we have initiated a download for the plugin
  // database.
  //
  // Parameters:
  // stream
  //   Pointer to the stream being destroyed.
  // Returns true if the plugin is ready to accept data.
  bool WriteReady(NPStream* stream) {
    NOTIMPLEMENTED();
    return false;
  }

  // Delivers data to the plugin instance.
  //
  // Parameters:
  // stream
  //   Pointer to the stream being destroyed.
  // offset
  //   Indicates the data offset.
  // buffer_length
  //   Indicates the length of the data buffer.
  // buffer
  //   Pointer to the actual buffer.
  // Returns the number of bytes actually written, 0 on error.
  int32 Write(NPStream* stream, int32 offset, int32 buffer_length,
              void* buffer) {
    NOTIMPLEMENTED();
    return 0;
  }

  // Handles notifications received in response to GetURLNotify calls issued
  // by the plugin.
  //
  // Parameters:
  // url
  //   Pointer to the URL.
  // reason
  //   Describes why the notification was sent.
  void URLNotify(const char* url, NPReason reason) {
    NOTIMPLEMENTED();
  }

  // Used by the renderer to indicate plugin install through the infobar.
  int16 NPP_HandleEvent(void* event) {
    NOTIMPLEMENTED();
    return 0;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PluginInstallerImpl);
};

#endif  // CHROME_DEFAULT_PLUGIN_PLUGIN_IMPL_ANDROID_H_
