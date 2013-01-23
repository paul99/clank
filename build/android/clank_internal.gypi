# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# DO NOT UPSTREAM THIS FILE which includes the information can't be public
# temporarily.
{
  'variables': {
    'conditions': [
      ['OS=="android"', {
        # Locatin of Android source tree that we are using.
        'android_src%': '<!(/bin/echo -n $ANDROID_BUILD_TOP)',

        # Location of Android product out directory.
        'android_product_out%': '<!(/bin/echo -n $ANDROID_PRODUCT_OUT)',

        # Location of Android system libraries.
        'android_lib%': '<(android_product_out)/system/lib',

        # Location of Android static libraries.
        'android_static_lib%': '<(android_product_out)/obj/STATIC_LIBRARIES',

        # Default to not compressing v8 startup data.
        # TODO(torne): Might completely remove this variable when startup
        # feature isn't need.
        'v8_compress_startup_data%': 'off',
      }],
    ],
  },
  'conditions': [
    ['OS=="android"', {
      'target_defaults': {
        'target_conditions': [
          # The 'android_build_type==0' means stand-alone (NDK/SDK), and ==1
          # means bundled with Android OS which is confidential so don't
          # mention it publicly.
          ['android_build_type==1', {
            'ldflags': [
              '-Wl,-rpath-link=<(android_ndk_lib):<(android_lib)',
              '-L<(android_ndk_lib)',
              '-L<(android_lib)',
            ],
          }],
          ['_toolset=="target"', {
            'defines': [
              # TODO(torne): Might need to remove the temporary hacks to reduce
              # binsize.
              'ANDROID_BINSIZE_HACK',
              'CHROME_SYMBOLS_LOCATION="<(chrome_symbols_location)"'
            ],
            'include_dirs': [
              # TODO(michaelbai): Needed for dependency on
              # base/test/test_stub_android.cc
              '<(android_src)/system/core/include',
            ],
            'conditions': [
              ['use_system_stlport==1', {
                'cflags': [
                  '-I<(android_src)/bionic',  # For libstdc++/include
                  '-I<(android_src)/external/stlport/stlport',
                ],
              }],
            ],
          }],
        ],
      },
    }],
  ],
}
