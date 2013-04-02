# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'app_list',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../skia/skia.gyp:skia',
        '../compositor/compositor.gyp:compositor',
        '../ui.gyp:ui',
      ],
      'defines': [
        'APP_LIST_IMPLEMENTATION',
      ],
      'sources': [
        'app_list_constants.cc',
        'app_list_constants.h',
        'app_list_export.h',
        'app_list_item_model.cc',
        'app_list_item_model.h',
        'app_list_item_model_observer.h',
        'app_list_model.cc',
        'app_list_model.h',
        'app_list_model_observer.h',
        'app_list_switches.cc',
        'app_list_switches.h',
        'app_list_view_delegate.h',
        'apps_grid_view_delegate.h',
        'cocoa/app_list_view.h',
        'cocoa/app_list_view.mm',
        'cocoa/app_list_view_window.h',
        'cocoa/app_list_view_window.mm',
        'pagination_model.cc',
        'pagination_model.h',
        'pagination_model_observer.h',
        'search_box_model.cc',
        'search_box_model.h',
        'search_box_model_observer.h',
        'search_box_view_delegate.h',
        'search_result.cc',
        'search_result.h',
        'search_result_list_view_delegate.h',
        'search_result_view_delegate.h',
        'signin_delegate.cc',
        'signin_delegate.h',
        'signin_delegate_observer.h',
        'views/app_list_background.cc',
        'views/app_list_background.h',
        'views/app_list_item_view.cc',
        'views/app_list_item_view.h',
        'views/app_list_main_view.cc',
        'views/app_list_main_view.h',
        'views/app_list_view.cc',
        'views/app_list_view.h',
        'views/apps_grid_view.cc',
        'views/apps_grid_view.h',
        'views/contents_view.cc',
        'views/contents_view.h',
        'views/page_switcher.cc',
        'views/page_switcher.h',
        'views/pulsing_block_view.cc',
        'views/pulsing_block_view.h',
        'views/search_box_view.cc',
        'views/search_box_view.h',
        'views/search_result_list_view.cc',
        'views/search_result_list_view.h',
        'views/search_result_view.cc',
        'views/search_result_view.h',
        'views/signin_view.cc',
        'views/signin_view.h',
      ],
      'conditions': [
        ['use_aura==1', {
          'dependencies': [
            '../aura/aura.gyp:aura',
          ],
        }],
        ['toolkit_views==1', {
          'dependencies': [
            '../views/views.gyp:views',
          ],
        }, {  # toolkit_views==0
          'sources/': [
            ['exclude', 'views/'],
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../ui.gyp:ui_cocoa_third_party_toolkits',
          ],
          'include_dirs': [
            '../../third_party/GTM',
          ],
        }, {  # OS!="mac"
          'sources/': [
            ['exclude', 'cocoa/'],
          ],
        }],
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
    {
      'target_name': 'app_list_unittests',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:test_support_base',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../compositor/compositor.gyp:compositor',
        '../compositor/compositor.gyp:compositor_test_support',
        'app_list',
      ],
      'sources': [
        'pagination_model_unittest.cc',
        'test/app_list_test_suite.cc',
        'test/app_list_test_suite.h',
        'test/run_all_unittests.cc',
        'cocoa/app_list_view_unittest.mm',
        'cocoa/app_list_view_window_unittest.mm',
        'views/apps_grid_view_unittest.cc',
        'views/test/apps_grid_view_test_api.cc',
        'views/test/apps_grid_view_test_api.h',
      ],
      'conditions': [
        ['toolkit_views==1', {
          'dependencies': [
            '../views/views.gyp:views',
            '../views/views.gyp:views_test_support',
          ],
        }, {  # toolkit_views==0
          'sources/': [
            ['exclude', 'views/'],
          ]
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../ui.gyp:ui_test_support',
          ],
        }, {  # OS!="mac"
          'sources/': [
            ['exclude', 'cocoa/'],
          ],
        }],
      ],
    },
  ],
}
