// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include "base/file_path.h"
#include "chrome/browser/ui/select_file_dialog.h"

class SelectFileDialogImpl : public SelectFileDialog {
 public:
  explicit SelectFileDialogImpl(Listener* listener);

  void OnFileSelected(JNIEnv*, jobject, jstring filepath);
  void OnFileNotSelected(JNIEnv*, jobject);

  // From SelectFileDialog
  virtual bool IsRunning(gfx::NativeWindow) const OVERRIDE;
  virtual void ListenerDestroyed() OVERRIDE;

  // Called when it is time to display the file picker.
  virtual void SelectFileImpl(
      SelectFileDialog::Type,
      const string16& title,
      const FilePath& default_path,
      const SelectFileDialog::FileTypeInfo* file_types,
      int file_type_index,
      const std::string& default_extension,
      gfx::NativeWindow owning_window,
      void* params) OVERRIDE;

 private:
  virtual bool HasMultipleFileTypeChoicesImpl() OVERRIDE;
  bool is_running_;
};

bool RegisterSelectFileDialog(JNIEnv*);
