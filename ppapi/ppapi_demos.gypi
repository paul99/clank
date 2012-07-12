# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    'ppapi.gypi',
  ],
  'targets': [
    {
      'target_name': 'ppapi_demos',
      'type': 'loadable_module',
      'sources': [
        '<@(ppp_entrypoints_sources)',

        # Common demo files.
        'demos/demo_framework.cc',
        'demos/demo_framework.h',

        # Plugin demos.
        'demos/demo_log.cc',
        'demos/demo_log.h',
        'demos/fingerpaint.cc',
        'demos/fingerpaint.h',
        'demos/focus.cc',
        'demos/focus.h',
      ],
      'dependencies': [
        'ppapi.gyp:ppapi_cpp',
        '../skia/skia.gyp:skia',
        '../base/base.gyp:base',
      ],
      'conditions': [
        ['OS=="win"', {
          'defines': [
            '_CRT_SECURE_NO_DEPRECATE',
            '_CRT_NONSTDC_NO_WARNINGS',
            '_CRT_NONSTDC_NO_DEPRECATE',
            '_SCL_SECURE_NO_DEPRECATE',
          ],
        }],
        ['OS=="mac"', {
          'mac_bundle': 1,
          'product_name': 'ppapi_demos',
          'product_extension': 'plugin',
        }],
      ],
    },
  ],
}
