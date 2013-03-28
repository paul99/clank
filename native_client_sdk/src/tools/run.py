#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Launch a local server on an ephemeral port, then launch a executable that
points to that server.
"""

import copy
import optparse
import os
import subprocess
import sys
import httpd


def main(args):
  usage = """usage: %prog [options] -- executable args...

  This command creates a local server on an ephemeral port, then runs:
    <executable> <args..> http://localhost:<port>/<page>.

  Where <page> can be set by -P, or uses index.html by default."""
  parser = optparse.OptionParser(usage)
  parser.add_option('-C', '--serve-dir',
      help='Serve files out of this directory.',
      dest='serve_dir', default=os.path.abspath('.'))
  parser.add_option('-P', '--path', help='Path to load from local server.',
      dest='path', default='index.html')
  parser.add_option('-E',
      help='Add environment variables when launching the executable.',
      dest='environ', action='append', default=[])
  parser.add_option('--test-mode',
      help='Listen for posts to /ok or /fail and shut down the server with '
          ' errorcodes 0 and 1 respectively.',
      dest='test_mode', action='store_true')
  options, args = parser.parse_args(args)
  if not args:
    parser.error('No executable given.')

  # 0 means use an ephemeral port.
  server = httpd.LocalHTTPServer(options.serve_dir, 0, options.test_mode)
  print 'Serving %s on %s...' % (options.serve_dir, server.GetURL(''))

  env = copy.copy(os.environ)
  for e in options.environ:
    key, value = map(str.strip, e.split('='))
    env[key] = value

  cmd = args + [server.GetURL(options.path)]
  print 'Running: %s...' % (' '.join(cmd),)
  process = subprocess.Popen(cmd, env=env)
  try:
    return server.ServeUntilSubprocessDies(process)
  finally:
    if process.returncode is None:
      process.kill()


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
