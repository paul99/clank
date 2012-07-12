#!/usr/bin/python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Collection of methods for running perf tests on the legacy browser.

TODO(tonyg): Run these with CPU frequency scaling disabled.
"""

import sys

from perf_test_runner import PerfTestRunner


runner = None
def GetRunner(device):
  """Lazily creates a test runner for device."""
  global runner
  if not runner or runner.device != device:
    runner = PerfTestRunner(device)
    runner.CopyTestData([
        'chrome/test/data/dromaeo/',
        'chrome/test/data/json2.js',
        'chrome/test/data/sunspider/',
        'chrome/test/data/v8_benchmark/',
        'chrome/test/perf/sunspider_uitest.js',
        'chrome/test/perf/v8_benchmark_uitest.js',
        ], '/data/local/tmp/')
  return runner


def RunHttpPageCyclerBenchmark(device, test_name, timeout=600):
  """Runs the |test_name| page cycler on the legacy browser.

  The caller is responsible for starting a local HTTP server to server
  page cycler data and for setting up forwarding on the device port 8000.
  """
  GetRunner(device).RunLegacyBrowserPerfTest(
      'http://localhost:8000/%s/start.html?auto=1' % test_name,
      [('times', 't', 'ms')], trace_tag='_legacy_browser', timeout=timeout)
