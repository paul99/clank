# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    # TODO(bulach): this is shared between chrome_tests.gyp and clank.gyp,
    # we should remove this inter-dependency.
    '../native.gypi',
  ],
  'targets': [
    {
      'target_name': 'chromeview',
      'type': 'shared_library',
      'dependencies': [
        '../native.gyp:jni_headers',
        '../../../chrome/chrome.gyp:renderer',
        '../../../chrome/chrome.gyp:browser',
        '../../../chrome/chrome.gyp:plugin',
        '../../../chrome/chrome.gyp:utility',
        '../../../content/content.gyp:content_gpu',
        '../../../content/content.gyp:content_ppapi_plugin',
        '../../../ui/ui.gyp:ui',
        # HACK to access proto files not exported by the syncapi target.
        '../../../chrome/browser/sync/protocol/sync_proto.gyp:sync_proto',
        '../../../skia/skia.gyp:skia',
        '../../../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '../../../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation',
        '../../../v8/tools/gyp/v8.gyp:v8',
      ],
      'include_dirs': [
        '../../..',
        '<(SHARED_INTERMEDIATE_DIR)/android',
        '<(SHARED_INTERMEDIATE_DIR)/chrome',
        '<(android_ndk_include)',  # For native_window.h, GL includes, etc.
      ],
      'sources': [
        '<@(clank_common_files)',
        'chrome/android_protocol_adapter.cc',
        'chrome/android_protocol_adapter.h',
        'chrome/android_sync_setup_flow_handler.cc',
        'chrome/android_sync_setup_flow_handler.h',
        'chrome/application_native_methods.cc',
        'chrome/application_native_methods.h',
        'chrome/autocomplete_bridge.cc',
        'chrome/autocomplete_bridge.h',
        'chrome/browser_version.cc',
        'chrome/browser_version.h',
        'chrome/chrome_browser_provider.cc',
        'chrome/chrome_browser_provider.h',
        'chrome/chrome_native_preferences.cc',
        'chrome/chrome_native_preferences.h',
        'chrome/glui_functor_renderer.cc',
        'chrome/glui_functor_renderer.h',
        'chrome/google_auto_login.cc',
        'chrome/google_auto_login.h',
        'chrome/google_location_settings_helper.cc',
        'chrome/google_location_settings_helper.h',
        'chrome/gpu_profiler.cc',
        'chrome/gpu_profiler.h',
        'chrome/infobar_android.cc',
        'chrome/infobar_android.h',
        'chrome/instant_tab.cc',
        'chrome/instant_tab.h',
        'chrome/location_bar.cc',
        'chrome/location_bar.h',
        'chrome/network_connectivity_receiver.cc',
        'chrome/network_connectivity_receiver.h',
        'chrome/page_info_viewer.cc',
        'chrome/page_info_viewer.h',
        'chrome/partner_bookmarks_shim.cc',
        'chrome/partner_bookmarks_shim.h',
        'chrome/process_utils.cc',
        'chrome/process_utils.h',
        'chrome/sqlite_cursor.cc',
        'chrome/sqlite_cursor.h',
        'chrome/sync_setup_manager.cc',
        'chrome/sync_setup_manager.h',
        'chrome/tab.cc',
        'chrome/tab.h',
        'chrome/tab_model.cc',
        'chrome/tab_model.h',
        'chrome/uma_record_action.cc',
        'chrome/uma_record_action.h',
        'chrome/uma_session_stats.cc',
        'chrome/uma_session_stats.h',
        'chrome/website_settings_utils.cc',
        'chrome/website_settings_utils.h',
        '../../../content/browser/android/android_stream_reader_url_request_job.cc',
        '../../../content/browser/android/android_stream_reader_url_request_job.h',
        '../../../content/browser/android/browser_process_main.cc',
        '../../../content/browser/android/browser_process_main.h',
        '../../../content/browser/android/chrome_http_auth_handler.cc',
        '../../../content/browser/android/chrome_http_auth_handler.h',
        '../../../content/browser/android/chrome_startup_flags.cc',
        '../../../content/browser/android/chrome_startup_flags.h',
        '../../../content/browser/android/chrome_view_client.cc',
        '../../../content/browser/android/chrome_view_client.h',
        '../../../content/browser/android/command_line.cc',
        '../../../content/browser/android/command_line.h',
        '../../../content/browser/android/devtools_server.cc',
        '../../../content/browser/android/devtools_server.h',
        '../../../content/browser/android/download_controller.cc',
        '../../../content/browser/android/download_controller.h',
        '../../../content/browser/android/ime_helper.cc',
        '../../../content/browser/android/ime_helper.h',
        '../../../content/browser/android/library_loader.cc',
        '../../../content/browser/android/touch_point.cc',
        '../../../content/browser/android/touch_point.h',
        '../../../content/browser/android/trace_event_binding.cc',
        '../../../content/browser/android/trace_event_binding.h',
        '../../../content/browser/android/v8_startup_data_mapper.cc',
        '../../../content/browser/android/v8_startup_data_mapper.h',
        'third_party/etc1.cc',
        'third_party/etc1.h',
      ],
      'link_settings': {
        'libraries': [
          '-landroid',      # ANativeWindow
          '-ljnigraphics',  # NDK access to bitmap
        ],
      },
      'conditions': [
        ['order_profiling!=0', {
          'conditions': [
            ['OS=="android"', {
              'dependencies': [ '../../../third_party/cygprofile/cygprofile.gyp:cygprofile' ],
            }],
          ],
        }],  # order_profiling!=0
        ['order_text_section!=""', {
          'conditions': [
            ['host_os=="linux"', {
              'target_conditions' : [
                ['_toolset=="target"', {
                  'ldflags': [
                    '-B./build/android/arm-linux-androideabi-gold.1.10/',
                    '-Wl,-section-ordering-file=<(order_text_section)' ],
                }],
              ]
            }],
          ],  # conditions for order_text_section
        }],  # order_text_section!=0
        ['java_bridge==1', {
          'defines': [
            'ENABLE_JAVA_BRIDGE',
          ],
        }],
      ],
    },
    {
      'target_name': 'clank_native',
      'type': 'none',
      'variables': {
        'clank_install_binary': '<(DEPTH)/clank/build/clank_install_binary',

        # TODO(bradnelson): reexamine how this is done if we change the
        # expansion of configurations
        'release_valgrind_build%': 0,
      },
      'actions': [
        {
          'action_name': 'clank_install_libchromeview',
          'inputs': [
            '<(SHARED_LIB_DIR)/libchromeview.so',
          ],
          'outputs': [
            '<(android_product_out)/obj/lib/libchromeview.so',
            '<(android_product_out)/symbols/system/lib/libchromeview.so',
          ],
          'action': [
            '<(clank_install_binary)',
            '<@(_inputs)',
            '<@(_outputs)',
            '<(release_valgrind_build)',
          ],
        },
        {
          'action_name': 'clank_enable_stack_libchromeview',
          'inputs': [
            '<(SHARED_LIB_DIR)/libchromeview.so',
          ],
          'outputs': [
            '<(android_product_out)/symbols/data/data/com.google.android.apps.chrome/lib/libchromeview.so',
          ],
          'action': [
            'ln',
            '-sf',
            '../../../../system/lib/libchromeview.so',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'clank_enable_stack_libchromeview_stable',
          'inputs': [
            '<(SHARED_LIB_DIR)/libchromeview.so',
          ],
          'outputs': [
            '<(android_product_out)/symbols/data/data/com.android.chrome/lib/libchromeview.so',
          ],
          'action': [
            'ln',
            '-sf',
            '../../../../system/lib/libchromeview.so',
            '<@(_outputs)',
          ],
        },
      ]
    },
    {
      'target_name': 'clank',
      'type': 'none',
      'dependencies': [
        '../../../chrome/chrome_resources.gyp:packed_resources',
        '../../../chrome/chrome_resources.gyp:packed_extra_resources',
        'clank_native',
      ],
      # TOOD(benm): Once gyp bug 255 is fixed, (see
      # http://code.google.com/p/gyp/issues/detail?id=255), we can collapse
      # these individiual copy steps into a single gyp 'copies' step.
      'actions': [
        {
          'action_name': 'clank_install_chrome_pak',
          'inputs': [
            '<(PRODUCT_DIR)/chrome.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/chrome.pak',
          ],
          'action': [
            'cp',
            '-f',
            '<@(_inputs)',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'clank_install_resources_pak',
          'inputs': [
            '<(PRODUCT_DIR)/resources.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/resources.pak',
          ],
          'action': [
            'cp',
            '-f',
            '<@(_inputs)',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'cp_ar',
          'inputs': [
            '<(PRODUCT_DIR)/locales/ar.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/ar.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_bg',
          'inputs': [
            '<(PRODUCT_DIR)/locales/bg.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/bg.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_ca',
          'inputs': [
            '<(PRODUCT_DIR)/locales/ca.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/ca.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_cs',
          'inputs': [
            '<(PRODUCT_DIR)/locales/cs.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/cs.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_da',
          'inputs': [
            '<(PRODUCT_DIR)/locales/da.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/da.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_de',
          'inputs': [
            '<(PRODUCT_DIR)/locales/de.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/de.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_el',
          'inputs': [
            '<(PRODUCT_DIR)/locales/el.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/el.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_en-GB',
          'inputs': [
            '<(PRODUCT_DIR)/locales/en-GB.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/en-GB.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_en-US',
          'inputs': [
            '<(PRODUCT_DIR)/locales/en-US.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/en-US.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_es',
          'inputs': [
            '<(PRODUCT_DIR)/locales/es.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/es.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_es-419',
          'inputs': [
            '<(PRODUCT_DIR)/locales/es-419.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/es-419.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_fa',
          'inputs': [
            '<(PRODUCT_DIR)/locales/fa.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/fa.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_fi',
          'inputs': [
            '<(PRODUCT_DIR)/locales/fi.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/fi.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_fr',
          'inputs': [
            '<(PRODUCT_DIR)/locales/fr.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/fr.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_he',
          'inputs': [
            '<(PRODUCT_DIR)/locales/he.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/he.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_hr',
          'inputs': [
            '<(PRODUCT_DIR)/locales/hr.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/hr.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_hu',
          'inputs': [
            '<(PRODUCT_DIR)/locales/hu.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/hu.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_id',
          'inputs': [
            '<(PRODUCT_DIR)/locales/id.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/id.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_it',
          'inputs': [
            '<(PRODUCT_DIR)/locales/it.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/it.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_ja',
          'inputs': [
            '<(PRODUCT_DIR)/locales/ja.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/ja.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_ko',
          'inputs': [
            '<(PRODUCT_DIR)/locales/ko.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/ko.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_lt',
          'inputs': [
            '<(PRODUCT_DIR)/locales/lt.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/lt.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_lv',
          'inputs': [
            '<(PRODUCT_DIR)/locales/lv.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/lv.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_nb',
          'inputs': [
            '<(PRODUCT_DIR)/locales/nb.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/nb.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_nl',
          'inputs': [
            '<(PRODUCT_DIR)/locales/nl.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/nl.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_pl',
          'inputs': [
            '<(PRODUCT_DIR)/locales/pl.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/pl.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_pt-BR',
          'inputs': [
            '<(PRODUCT_DIR)/locales/pt-BR.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/pt-BR.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_pt-PT',
          'inputs': [
            '<(PRODUCT_DIR)/locales/pt-PT.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/pt-PT.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_ro',
          'inputs': [
            '<(PRODUCT_DIR)/locales/ro.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/ro.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_ru',
          'inputs': [
            '<(PRODUCT_DIR)/locales/ru.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/ru.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_sk',
          'inputs': [
            '<(PRODUCT_DIR)/locales/sk.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/sk.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_sl',
          'inputs': [
            '<(PRODUCT_DIR)/locales/sl.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/sl.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_sr',
          'inputs': [
            '<(PRODUCT_DIR)/locales/sr.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/sr.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_sv',
          'inputs': [
            '<(PRODUCT_DIR)/locales/sv.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/sv.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_th',
          'inputs': [
            '<(PRODUCT_DIR)/locales/th.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/th.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_fil',
          'inputs': [
            '<(PRODUCT_DIR)/locales/fil.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/fil.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_tr',
          'inputs': [
            '<(PRODUCT_DIR)/locales/tr.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/tr.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_uk',
          'inputs': [
            '<(PRODUCT_DIR)/locales/uk.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/uk.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_vi',
          'inputs': [
            '<(PRODUCT_DIR)/locales/vi.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/vi.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_zh-CN',
          'inputs': [
            '<(PRODUCT_DIR)/locales/zh-CN.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/zh-CN.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'cp_zh-TW',
          'inputs': [
            '<(PRODUCT_DIR)/locales/zh-TW.pak',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../assets/zh-TW.pak',
          ],
          'action': [ 'cp', '-f', '<@(_inputs)', '<@(_outputs)', ],
        },
        {
          'action_name': 'clank_build_framework',
          'inputs': [
            '<(android_product_out)/obj/lib/libchromeview.so',
          ],
          'outputs': [
            '<(android_product_out)/../../common/obj/JAVA_LIBRARIES/chrome-view_intermediates/javalib.jar',
          ],
          'action': [
            '<(DEPTH)/clank/build/clank_mmm',
            'content/android/java',
          ],
        },
        {
          'action_name': 'clank_generate_apk',
          'inputs': [
            '<(PRODUCT_DIR)/../assets/chrome.pak',
            '<(PRODUCT_DIR)/../assets/resources.pak',
            '<(android_product_out)/obj/lib/libchromeview.so',
            '<(android_product_out)/../../common/obj/JAVA_LIBRARIES/chrome-view_intermediates/javalib.jar',
            '<(PRODUCT_DIR)/../assets/ar.pak',
            '<(PRODUCT_DIR)/../assets/bg.pak',
            '<(PRODUCT_DIR)/../assets/ca.pak',
            '<(PRODUCT_DIR)/../assets/cs.pak',
            '<(PRODUCT_DIR)/../assets/da.pak',
            '<(PRODUCT_DIR)/../assets/de.pak',
            '<(PRODUCT_DIR)/../assets/el.pak',
            '<(PRODUCT_DIR)/../assets/en-GB.pak',
            '<(PRODUCT_DIR)/../assets/en-US.pak',
            '<(PRODUCT_DIR)/../assets/es.pak',
            '<(PRODUCT_DIR)/../assets/es-419.pak',
            '<(PRODUCT_DIR)/../assets/fa.pak',
            '<(PRODUCT_DIR)/../assets/fi.pak',
            '<(PRODUCT_DIR)/../assets/fil.pak',
            '<(PRODUCT_DIR)/../assets/fr.pak',
            '<(PRODUCT_DIR)/../assets/he.pak',
            '<(PRODUCT_DIR)/../assets/hr.pak',
            '<(PRODUCT_DIR)/../assets/hu.pak',
            '<(PRODUCT_DIR)/../assets/id.pak',
            '<(PRODUCT_DIR)/../assets/it.pak',
            '<(PRODUCT_DIR)/../assets/ja.pak',
            '<(PRODUCT_DIR)/../assets/ko.pak',
            '<(PRODUCT_DIR)/../assets/lt.pak',
            '<(PRODUCT_DIR)/../assets/lv.pak',
            '<(PRODUCT_DIR)/../assets/nb.pak',
            '<(PRODUCT_DIR)/../assets/nl.pak',
            '<(PRODUCT_DIR)/../assets/pl.pak',
            '<(PRODUCT_DIR)/../assets/pt-BR.pak',
            '<(PRODUCT_DIR)/../assets/pt-PT.pak',
            '<(PRODUCT_DIR)/../assets/ro.pak',
            '<(PRODUCT_DIR)/../assets/ru.pak',
            '<(PRODUCT_DIR)/../assets/sk.pak',
            '<(PRODUCT_DIR)/../assets/sl.pak',
            '<(PRODUCT_DIR)/../assets/sr.pak',
            '<(PRODUCT_DIR)/../assets/sv.pak',
            '<(PRODUCT_DIR)/../assets/th.pak',
            '<(PRODUCT_DIR)/../assets/tr.pak',
            '<(PRODUCT_DIR)/../assets/uk.pak',
            '<(PRODUCT_DIR)/../assets/vi.pak',
            '<(PRODUCT_DIR)/../assets/zh-CN.pak',
            '<(PRODUCT_DIR)/../assets/zh-TW.pak',
          ],
          'outputs': [
            '<(android_product_out)/system/app/Chrome.apk',
          ],
          'action': [
            '<(DEPTH)/clank/build/clank_mmm',
            'clank/java/apps/chrome',
          ],
        },
      ],
    },
    {
      'target_name': 'clank_ppapi_example',
      'type': 'none',
      'dependencies': [
        '../../../ppapi/ppapi_internal.gyp:ppapi_example',
      ],
      'copies': [
        {
          'destination': '<(android_product_out)/symbols/system/lib/',
          'files': [
            '<(SHARED_LIB_DIR)/libppapi_example.so',
          ],
        },
      ],
      'actions': [
        {
          'action_name': 'clank_install_ppapi_example',
          'inputs': [
            '<(SHARED_LIB_DIR)/libppapi_example.so',
          ],
          'outputs': [
            '<(android_product_out)/obj/lib/libppapi_example.so',
          ],
          'action': [
            '<!(/bin/echo -n $STRIP)',
            '--strip-unneeded',  # All symbols not needed for relocation.
            '<@(_inputs)',
            '-o',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'clank_enable_stack_libppapi_example',
          'inputs': [
            '<(SHARED_LIB_DIR)/libppapi_example.so',
          ],
          'outputs': [
            '<(android_product_out)/symbols/data/data/com.android.ppapi/lib/libppapi_example.so',
          ],
          'action': [
            'ln',
            '-sf',
            '../../../../system/lib/libppapi_example.so',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'pepper_example_generate_apk',
          'inputs': [
            '<(android_product_out)/obj/lib/libppapi_example.so',
          ],
          'outputs': [
            '<(android_product_out)/data/app/ppapi_example.apk',
          ],
          'action': [
            '<(DEPTH)/clank/build/clank_mmm',
            'clank/java/apps/ppapi_example',
          ],
        },
      ],
    },
    {
      'target_name': 'clank_ppapi_tests',
      'type': 'none',
      'dependencies': [
        '../../../ppapi/ppapi_internal.gyp:ppapi_tests',
      ],
      'copies': [
        {
          'destination': '<(android_product_out)/symbols/system/lib/',
          'files': [
            '<(PRODUCT_DIR)/libppapi_tests.so',
          ],
        },
      ],
      'actions': [
        {
          'action_name': 'clank_install_ppapi_tests',
          'inputs': [
            '<(PRODUCT_DIR)/libppapi_tests.so',
          ],
          'outputs': [
            '<(android_product_out)/obj/lib/libppapi_tests.so',
          ],
          'action': [
            '<!(/bin/echo -n $STRIP)',
            '--strip-unneeded',  # All symbols not needed for relocation.
            '<@(_inputs)',
            '-o',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'clank_enable_stack_libppapi_tests',
          'inputs': [
            '<(PRODUCT_DIR)/libppapi_tests.so',
          ],
          'outputs': [
            '<(android_product_out)/symbols/data/data/com.android.ppapi_tests/lib/libppapi_tests.so',
          ],
          'action': [
            'ln',
            '-sf',
            '../../../../system/lib/libppapi_tests.so',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'pepper_tests_generate_apk',
          'inputs': [
            '<(android_product_out)/obj/lib/libppapi_tests.so',
          ],
          'outputs': [
            '<(android_product_out)/data/app/ppapi_tests.apk',
          ],
          'action': [
            '<(DEPTH)/clank/build/clank_mmm',
            'clank/java/apps/ppapi_tests',
          ],
        },
      ],
    },
    {
      'target_name': 'clank_ppapi_demos',
      'type': 'none',
      'dependencies': [
        '../../../ppapi/ppapi_internal.gyp:ppapi_demos',
      ],
      'copies': [
        {
          'destination': '<(android_product_out)/symbols/system/lib/',
          'files': [
            '<(PRODUCT_DIR)/libppapi_demos.so',
          ],
        },
      ],
      'actions': [
        {
          'action_name': 'clank_install_ppapi_demos',
          'inputs': [
            '<(PRODUCT_DIR)/libppapi_demos.so',
          ],
          'outputs': [
            '<(android_product_out)/obj/lib/libppapi_demos.so',
          ],
          'action': [
            '<!(/bin/echo -n $STRIP)',
            '--strip-unneeded',  # All symbols not needed for relocation.
            '<@(_inputs)',
            '-o',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'clank_enable_stack_libppapi_demos',
          'inputs': [
            '<(PRODUCT_DIR)/libppapi_demos.so',
          ],
          'outputs': [
            '<(android_product_out)/symbols/data/data/com.android.ppapi_demos/lib/libppapi_demos.so',
          ],
          'action': [
            'ln',
            '-sf',
            '../../../../system/lib/libppapi_demos.so',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'pepper_demos_generate_apk',
          'inputs': [
            '<(android_product_out)/obj/lib/libppapi_demos.so',
          ],
          'outputs': [
            '<(android_product_out)/data/app/ppapi_demos.apk',
          ],
          'action': [
            '<(DEPTH)/clank/build/clank_mmm',
            'clank/java/apps/ppapi_demos',
          ],
        },
      ],
    },
    {
      'target_name': 'devtools_resources',
      'type': 'none',
      'dependencies': [
        '../../../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:inspector_resources',
        '../../../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:devtools_extension_api',
      ]
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
