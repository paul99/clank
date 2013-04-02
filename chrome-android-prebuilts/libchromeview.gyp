# This hunk of gyp can be used to regenerate libchromeview.so using a prebuilt
# static library.
#
# To use this gyp file:
# - Take the provided libchrome_android_prebuilt.a and place it the relevant
#   build out directory (e.g. out/$BUILD_TYPE/libchrome_android_prebuilt.a).
# - Run gyp using this gyp target as root:
#     CHROMIUM_GYP_FILE="chrome-android-prebuilts/chromeview_target.gyp" build/gyp_chromium
# - Build libchromeview_prebuilt:
#      ninja (or make) libchromeview_prebuilt
# - Rename libchromeview_prebuilt.so to libchromeview.so and replace the
#     existing shared library with it (ensuring to re-sign the apk).
{
  'targets': [
    {
      # An "All" target is required for a top-level gyp-file.
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
          'libchromeview_prebuilt',
      ],
    },
    {
      # Target which allows rebuilding of full libchromeview.so provided an appropriate static library.
      'target_name': 'libchromeview_prebuilt',
      'type': 'shared_library',
      'dependencies': [
        '../chrome/chrome.gyp:chrome_android_core'
      ],
      'sources': [
        # This file must always be included in the shared_library step to en sure
        # JNI_OnLoad is exported.
        '../chrome/app/android/chrome_jni_onload.cc',
      ],
      'libraries': [
        '<(PRODUCT_DIR)/libchrome_android_prebuilt.a',
      ],
      'conditions': [
        ['host_os=="linux"', {
          'target_conditions' : [
            ['_toolset=="target"', {
              'ldflags': [
                '-B./build/android/arm-linux-androideabi-gold.1.10/',
              ],
            }],
          ]
        }],
      ],
    }],
}
