#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os
import re
import sys


def FindCurrentPackage(file_name):
  re_package = re.compile(r'package="([\w\.]+)"')
  with file(file_name, 'r') as f:
    for line in f:
      m = re_package.search(line)
      if m:
        return m.group(1)
  return None


def ReplaceFileContents(file_name, replacements, current_package, new_package, count):
  with file(file_name, 'r') as f:
    data = f.read()
  for r in replacements:
    pattern = r[0] % {'package': current_package}
    m = re.search(pattern, data)
    assert m, 'Unable to find %s' % pattern
    data = re.sub(pattern,
                  r[1] % {'package': new_package},
                  data, count=count)
  with file(file_name, 'w') as f:
    f.write(data)
  print 'Updated %s' % file_name


def ChangePackage(new_package, app_name, base_path):
  app_manifest = os.path.join(base_path, 'AndroidManifest.xml')
  current_package = FindCurrentPackage(app_manifest)

  replacements = [
    (r'package="%(package)s"',
     'package="%(package)s"'),
    (r'android:authorities="%(package)s.snapshot"',
     'android:authorities="%(package)s.snapshot"'),
    (r'android:authorities="%(package)s.ChromeBrowserProvider(.*?);%(package)s.browser;%(package)s"',
     'android:authorities="%(package)s.ChromeBrowserProvider;%(package)s.browser;%(package)s"'),
    (r'android:label="\@string/(.*?)app_name"',
     'android:label="@string/example_app_name"'),
  ]
  ReplaceFileContents(app_manifest, replacements, current_package, new_package, 1)

  replacements = [
    (r'<category android:name="%(package)s"',
     '<category android:name="%(package)s"'),
    (r'android:name="%(package)s.permission.C2D_MESSAGE"',
     'android:name="%(package)s.permission.C2D_MESSAGE"'),
  ]
  # Since these replacements both occur twice, we need to set count=2.
  ReplaceFileContents(app_manifest, replacements, current_package, new_package, 2)

  sync_adapter = os.path.join(base_path, 'res/xml/syncadapter.xml')
  replacements = [
    (r'android:contentAuthority="[^\"]*"',
     'android:contentAuthority="%(package)s"')
  ]
  ReplaceFileContents(sync_adapter, replacements, current_package, new_package, 1)

  searchable = os.path.join(base_path, 'res/xml/searchable.xml')
  replacements = [
    (r'android:searchSuggestAuthority="[^\"]*"',
     'android:searchSuggestAuthority="%(package)s.browser"')
  ]
  ReplaceFileContents(searchable, replacements, current_package, new_package, 1)

  strings = os.path.join(base_path, 'res/values/strings.xml')
  replacements = [
    (r'string name="example_app_name">[^<]*</string>',
     'string name="example_app_name">%s</string>' % app_name)
  ]
  ReplaceFileContents(strings, replacements, current_package, new_package, 1)


def main(argv):
  option_parser = optparse.OptionParser(description=
      'Changes the package and app names of the Chrome application.')
  option_parser.add_option('-u', dest='unpacked_apk_path',
                           default='Chrome',
                           help='Path to the unpacked apk files. [default: %default]')
  option_parser.add_option('-a', dest='app_name', default='Example Chromium',
                           help='Name of the application. [default: "%default"]')
  option_parser.add_option('-p', dest='package_name',
                           default='com.example.chromium',
                           help=('Desired application package name. '
                                 '[default: %default]'))
  options, _ = option_parser.parse_args(argv)

  official_package_names = [
    'com.android.chrome',
    'com.google.android.apps.chrome',
    'com.google.android.apps.chrome_beta',
    'com.google.android.apps.chrome_dev',
  ]

  if options.package_name in official_package_names:
    print ('Package %s would conflict with an official Chrome package. '
          'Use a different name.' % options.package_name)
    sys.exit(1)

  if not os.path.isdir(options.unpacked_apk_path):
    print "Cannot find unpacked apk folder '%s'.\n" % options.unpacked_apk_path
    option_parser.print_help()
    sys.exit(1)

  ChangePackage(options.package_name,
                options.app_name,
                options.unpacked_apk_path)

if __name__ == '__main__':
  sys.exit(main(sys.argv))
