#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Generates shim headers that mirror the directory structure of bundled headers,
but just forward to the system ones.

This allows seamless compilation against system headers with no changes
to our source code.
"""


import optparse
import os.path
import sys


SHIM_TEMPLATE = """
#if defined(OFFICIAL_BUILD)
#error shim headers must not be used in official builds!
#endif

#include <%s>
"""


def GeneratorMain(argv):
  parser = optparse.OptionParser()
  parser.add_option('--headers-root')
  parser.add_option('--output-directory')
  parser.add_option('--outputs', action='store_true')
  parser.add_option('--generate', action='store_true')

  options, args = parser.parse_args(argv)

  if not options.headers_root:
    parser.error('Missing --headers-root parameter.')
  if not options.output_directory:
    parser.error('Missing --output-directory parameter.')
  if not args:
    parser.error('Missing arguments - header file names.')

  source_tree_root = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..'))

  target_directory = os.path.join(
    options.output_directory,
    os.path.relpath(options.headers_root, source_tree_root))
  if options.generate and not os.path.exists(target_directory):
    os.makedirs(target_directory)
  for header_filename in args:
    if options.outputs:
      yield os.path.join(target_directory, header_filename)
    if options.generate:
      with open(os.path.join(target_directory, header_filename), 'w') as f:
        f.write(SHIM_TEMPLATE % header_filename)


def DoMain(argv):
  return '\n'.join(GeneratorMain(argv))


if __name__ == '__main__':
  DoMain(sys.argv[1:])
