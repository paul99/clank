# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'ssl',
      'type': 'none',
      'direct_dependent_settings': {
        'defines': [
          'USE_OPENSSL',
        ],
        'include_dirs': [
          '../../third_party/openssl/openssl/include',
          '../../third_party/openssl/config/android',
        ],
      },
      'dependencies': [
        '../../third_party/openssl/openssl.gyp:openssl',
      ],
    },
    {
      'target_name': 'ft2',
      'type': 'none',
      'link_settings': {
        'libraries': [
          '<(android_static_lib)/libft2_intermediates/libft2.a',
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(android_src)/external/freetype/include',  # For ft2build.h
        ],
      },
    },
    {
      'target_name': 'harfbuzz',
      'type': 'none',
      'direct_dependent_settings': {
        'defines': [
          'HAVE_ENDIAN_H',
        ],
        'include_dirs': [
          '<(android_src)/external/harfbuzz/contrib',
          '<(android_src)/external/harfbuzz/src',
        ],
        'libraries': [
          '-lharfbuzz',
        ],
      },
    },
  ],
}
