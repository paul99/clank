# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets' : [
    {
      'target_name': 'image_diff',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../ui/ui.gyp:ui',
      ],
      'sources': [
        'image_diff.cc',
      ],
      'conditions': [
        [ 'OS == "android"', {
          # FIXME(wangxianzhu): Without this dependency, image_diff won't be
          # linked with '-lskia -lharfbuzz' libraries. In theory, the dependency
          # app_base should automatically bring them.
          'dependencies': [
             '../../skia/skia.gyp:skia',
          ],
        }],
      ],
    },
  ],
}
