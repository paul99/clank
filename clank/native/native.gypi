# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
      # TODO(bulach): this is shared between chrome_tests.gyp and clank.gyp,
      # we should remove this inter-dependency.
      'clank_common_files': [
            'framework/chrome/url_utilities.cc',
            'framework/chrome/url_utilities.h',
            '../../content/browser/android/chrome_library_loader_hooks.cc',
            '../../content/browser/android/chrome_library_loader_hooks.h',
            '../../content/browser/android/chrome_view.cc',
            '../../content/browser/android/chrome_view.h',
            '../../content/browser/android/chrome_video_view.cc',
            '../../content/browser/android/chrome_video_view.h',
            '../../content/browser/android/remote_debugging_controller.cc',
            '../../content/browser/android/remote_debugging_controller.h',
            '../../content/browser/android/waitable_native_event.cc',
            '../../content/browser/android/waitable_native_event.h',
            '../../content/browser/android/web_settings.cc',
            '../../content/browser/android/web_settings.h',
      ],
  },
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
