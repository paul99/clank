# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'action_name': 'repack_resources',
  'variables': {
    'pak_inputs': [
      '<(grit_out_dir)/net_internals_resources.pak',
      '<(grit_out_dir)/shared_resources.pak',
      '<(grit_out_dir)/sync_internals_resources.pak',
      '<(grit_out_dir)/workers_resources.pak',
    ],
    'conditions': [
      ['OS != "mac" and OS != "android"', {
        'pak_inputs': [
          '<(grit_out_dir)/quota_internals_resources.pak',
        ],
      }],
      ['OS != "android"', {
        'pak_inputs': [
          '<(SHARED_INTERMEDIATE_DIR)/webkit/devtools_resources.pak',
          '<(grit_out_dir)/component_extension_resources.pak',
          '<(grit_out_dir)/devtools_discovery_page_resources.pak',
          '<(grit_out_dir)/options_resources.pak',
          '<(grit_out_dir)/options2_resources.pak',
          '<(grit_out_dir)/quota_internals_resources.pak',
        ],
      }],
    ],
  },
  'inputs': [
    '<(repack_path)',
    '<@(pak_inputs)',
  ],
  'outputs': [
    '<(SHARED_INTERMEDIATE_DIR)/repack/resources.pak',
  ],
  'action': ['python', '<(repack_path)', '<@(_outputs)', '<@(pak_inputs)'],
}
