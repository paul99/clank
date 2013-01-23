# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'webkit_media',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/media/media.gyp:yuv_convert',
        '<(DEPTH)/skia/skia.gyp:skia',
      ],
      'sources': [
        'active_loader.cc',
        'active_loader.h',
        'audio_decoder.cc',
        'audio_decoder.h',
        'audio_decoder_android.cc',
        'buffered_data_source.cc',
        'buffered_data_source.h',
        'buffered_resource_loader.cc',
        'buffered_resource_loader.h',
        'filter_helpers.cc',
        'filter_helpers.h',
        'media_stream_client.h',
        'media_metadata_android.cc',
        'media_metadata_android.h',
        'media_player_constants_android.h',
        'media_player_bridge_manager_impl.cc',
        'media_player_bridge_manager_impl.h',
        'simple_data_source.cc',
        'simple_data_source.h',
        'skcanvas_video_renderer.cc',
        'skcanvas_video_renderer.h',
        'streamtextureproxy_factory_android.h',
        'web_data_source.cc',
        'web_data_source.h',
        'webmediaplayer_android.h',
        'webmediaplayer_delegate.h',
        'webmediaplayer_impl_android.cc',
        'webmediaplayer_impl_android.h',
        'webmediaplayer_impl.cc',
        'webmediaplayer_impl.h',
        'webmediaplayer_in_process_proxy_android.cc',
        'webmediaplayer_in_process_proxy_android.h',
        'webmediaplayer_in_process_android.cc',
        'webmediaplayer_in_process_android.h',
        'webmediaplayer_manager_android.cc',
        'webmediaplayer_manager_android.h',
        'webmediaplayer_proxy_android.h',
        'webmediaplayer_proxy.cc',
        'webmediaplayer_proxy.h',
        'webmediaplayer_util.cc',
        'webmediaplayer_util.h',
        'webvideoframe_impl.cc',
        'webvideoframe_impl.h',
        'webvideoframe_impl_android.cc',
        'webvideoframe_impl_android.h',
      ],
      'conditions': [
        ['inside_chromium_build==0', {
          'dependencies': [
            '<(DEPTH)/webkit/support/setup_third_party.gyp:third_party_headers',
          ],
        }],
        [ 'OS != "android"', {
          'sources!': [
            'audio_decoder_android.cc',
          ],
        }],
        [ 'OS == "android"', {
          'sources!': [
            'audio_decoder.cc',
            'webmediaplayer_impl.cc',
            'webmediaplayer_impl.h',
          ],
        }],
      ],
    },
  ],
}
