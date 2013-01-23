#!/usr/bin/python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Collection of methods for running perf tests on the legacy browser."""

import collections
import fnmatch
import optparse
import pexpect
import random
import re
import sys
import time
import urlparse

import android_commands
from base_test_runner import BaseTestRunner
from perf_tests_helper import PrintPerfResult
from run_tests_helper import *


# Match logcat output that corresponds to console.log() in JavaScript.
LEGACY_BROWSER_CONSOLE_FORMAT_RE = '.*Console: %s: ([^\s]+).*'
CHROME_CONSOLE_FORMAT_RE = '.*INFO:CONSOLE.*"%s: ([^\s"]+)".*'

# Identify browser crashes in logcat.
ACTIVITY_CRASH_RE = 'ActivityManager: Process %s \(pid \d+\) has died.'

# Log marker controlling monitor of page flip count of Surface.
SURFACE_FPS_MONITOR_START = 'startSurfaceFpsMonitor'
SURFACE_FPS_MONITOR_STOP = 'stopSurfaceFpsMonitor'


class PerfTestRunner(BaseTestRunner):
  """Class for running performance tests.

  Args:
    device: Tests will run on the device of this ID.
  """

  TARGET_TRACE_FILE = '/sdcard/prof.dat'

  def __init__(self, device):
    BaseTestRunner.__init__(self, device, 0)
    self.trace = None

  @classmethod
  def GetAllTests(cls, test_filter=None):
    """Returns a list of all tests available in the test suite."""
    all_tests =  [f for f in dir(cls) if f.endswith('Benchmark')]

    if not test_filter:
      return all_tests

    re_filter = None
    try:
      re_filter = re.compile(test_filter)
    except re.exception as e:
      print 'Bad filter: ', e
      return None

    return [t for t in all_tests if re_filter.match(t)]

  @staticmethod
  def OutputFailure(msg):
    print msg
    print '[  FAILED  ]'

  def _SetupBrowserPreferences(self, package):
    """Sets up the browser's preferences for perf testing.

    This includes suppressing the "restore tabs" prompt and allowing replay of
    SSL content with WPR.
    """
    # Only necessary on legacy browser, as Chrome uses command line flags.
    if package != LEGACY_BROWSER_PACKAGE:
      return
    # After force-stopping the android browser, it will display a "restore tabs"
    # prompt any time it is opened over the next 30 minutes. Since we need the
    # tabs to be restored but don't want a prompt, we set this pref to make
    # the browser think it crashed longer than 30 minutes ago.
    self.adb.SetFileContents(
        '/data/data/%s/shared_prefs/browser_recovery_prefs.xml' % package,
        """<?xml version="1.0" encoding="utf-8" standalone="yes" ?>
<map>
  <long name="last_recovered" value="0" />
</map>
""")
    # Avoid security prompts to allow WPR to serve SSL content.
    self.adb.SetFileContents(
        '/data/data/%s/shared_prefs/%s_preferences.xml' % (package, package),
        """<?xml version="1.0" encoding="utf-8" standalone="yes" ?>
<map>
  <boolean name="show_security_warnings" value="false" />
</map>
""")

  def WaitForLogMatchOrPackageCrash(self, success_re, package, crash_msg):
    """Blocks until a matching line is logged, package crash or timeout.

    Args:
      success_re: A compiled re to search each line for.
      package: Package to monitor for crash.
      msg: Additional message to be output upon crash

    Raises:
      pexpect.TIMEOUT upon the timeout specified by StartMonitoringLogcat().

    Returns:
      The re match object if |success_re| is matched first or None if crashed.
    """
    error_re = re.compile(ACTIVITY_CRASH_RE % re.escape(package))
    m = self.adb.WaitForLogMatch(success_re, error_re)
    if m:
      return m
    # TODO(tonyg): Dump crash stack here (b/5915899).
    PerfTestRunner.OutputFailure(
        '%s CRASHED while waiting for %s' % (package, crash_msg))
    return None

  def RunChromeTestLauncherPerfTest(self, url, perf_test_param, test_activity,
                                    expected_results, trace_tag='', timeout=30):
    """Runs a JavaScript based performance test on Chrome's TestLauncher.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      url: The URL of the JavaScript performance test. The caller is responsible
          for ensuring this URL is accessible on the phone (either by copying
          locally or starting an HTTP server + forwarder).
      perf_test_param: A param to be used by this test (such as fling speed,
          zoom distance, etc.).
      test_activity: Name of the test activity.
      expected_results: A list of tuple of (log_marker, chart_name, trace_name,
          units).
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    return self._RunPerfTest(
        CHROME_PACKAGE, '%s.%s' % (CHROME_TESTS_PACKAGE, test_activity),
        '.*ChromeTest.*%s: ([^\s]+)$', url, expected_results,
        trace_tag=trace_tag, browser_extras={'speed': perf_test_param},
        timeout=timeout) != None

  def RunChromePerfTest(self, url, expected_results, trace_tag='', timeout=30):
    """Runs a JavaScript based performance test on Chrome.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      url: The URL of the JavaScript performance test. The caller is responsible
          for ensuring this URL is accessible on the phone (either by copying
          locally or starting an HTTP server + forwarder).
      expected_results: A list of tuple of (log_marker, chart_name, trace_name,
          units).
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    return self._RunPerfTest(
        CHROME_PACKAGE, CHROME_ACTIVITY, CHROME_CONSOLE_FORMAT_RE,
        url, expected_results, trace_tag=trace_tag, timeout=timeout) != None

  def RunChromePerfTestResults(self, url, expected_results, trace_tag='', timeout=30):
    """Runs a JavaScript based performance test on Chrome.

    Same as RunChromePerfTest, except that this returns a list of results
    in case of success, or None on failure.

    Args:
      url: The URL of the JavaScript performance test. The caller is responsible
          for ensuring this URL is accessible on the phone (either by copying
          locally or starting an HTTP server + forwarder).
      expected_results: A list of tuple of (log_marker, chart_name, trace_name,
          units).
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    return self._RunPerfTest(
        CHROME_PACKAGE, CHROME_ACTIVITY, CHROME_CONSOLE_FORMAT_RE,
        url, expected_results, trace_tag=trace_tag, timeout=timeout)

  def RunChromeUrlCyclerPerfTest(self, urls, expected_results, trace_tag='',
                                 timeout=30):
    """Runs a page loading performance test on Chrome.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      urls: List of URLs to load. The caller is responsible for ensuring this
          URL is accessible on the phone.
      expected_results: A list of tuple of (log_marker, units).
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    return self._RunUrlCyclerPerfTest(
        CHROME_PACKAGE, CHROME_ACTIVITY, CHROME_CONSOLE_FORMAT_RE,
        urls, expected_results, trace_tag=trace_tag, timeout=timeout)

  def RunChromeBackgroundMemoryPerfTest(self, urls, expected_results,
                                        trace_tag='', timeout=60):
    """Measure memory usage while Chrome has the given URLs open but is hidden.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      urls: List of URLs to load. The caller is responsible for ensuring this
          URL is accessible on the phone.
      expected_results: A list of log marker strings.
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    return self._RunBackgroundMemoryPerfTest(
        CHROME_PACKAGE, CHROME_ACTIVITY, CHROME_CONSOLE_FORMAT_RE,
        urls, expected_results, trace_tag=trace_tag, timeout=timeout)

  def RunLegacyBrowserBackgroundMemoryPerfTest(self, urls, expected_results,
                                               trace_tag='', timeout=60):
    """Measure memory usage while Browser has the given URLs open but is hidden.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      urls: List of URLs to load. The caller is responsible for ensuring this
          URL is accessible on the phone.
      expected_results: A list of log marker strings.
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    return self._RunBackgroundMemoryPerfTest(
        LEGACY_BROWSER_PACKAGE, LEGACY_BROWSER_ACTIVITY,
        LEGACY_BROWSER_CONSOLE_FORMAT_RE, urls, expected_results,
        trace_tag=trace_tag, timeout=timeout)

  def RunLegacyBrowserUrlCyclerPerfTest(self, urls, expected_results,
                                        trace_tag='', timeout=30):
    """Runs a page loading performance test on the legacy browser.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      urls: List of URLs to load. The caller is responsible for ensuring this
          URL is accessible on the phone.
      expected_results: A list of tuple of (log_marker, units).
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    return self._RunUrlCyclerPerfTest(
        LEGACY_BROWSER_PACKAGE, LEGACY_BROWSER_ACTIVITY,
        LEGACY_BROWSER_CONSOLE_FORMAT_RE, urls, expected_results,
        trace_tag=trace_tag, timeout=timeout)

  def RunLegacyBrowserPerfTest(self, url, expected_results, trace_tag='',
                               timeout=30):
    """Runs a JavaScript based performance test on the legacy Android browser.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      url: The URL of the JavaScript performance test. The caller is responsible
          for ensuring this URL is accessible on the phone (either by copying
          locally or starting an HTTP server + forwarder).
      expected_results: A list of tuple of (log_marker, chart_name, trace_name,
          units).
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    return self._RunPerfTest(
        LEGACY_BROWSER_PACKAGE, LEGACY_BROWSER_ACTIVITY,
        LEGACY_BROWSER_CONSOLE_FORMAT_RE, url, expected_results,
        trace_tag=trace_tag, timeout=timeout) != None

  def StartupBrowser(self, browser_package, browser_activity, token=None,
                     action='android.intent.action.VIEW', url=None,
                     browser_extras=None):
    """Starts the given browser to url.

    Args:
      browser_package: The package of the browser to start (e.g.
          'com.google.android.apps.chrome').
      browser_activity: The activity of the browser to start (e.g. 'Main').
      token: A unique token to identify this load.
      action: The action to pass to the browser Intent.
      url: The URL to start in the browser, may be empty.
      browser_extras: Extra data to pass to the browser Intent.

    Returns:
      The time at which the intent to start the browser was sent.
    """
    benchmark_url = None
    log_match_url_re = ''
    if url:
      benchmark_url = url
      if token:
        benchmark_url += '#' + token
      # We did not match the token in here because now Android does NOT output
      # the fragment or query params into log due to privacy concern, or output
      # them in different encoding. But you can still check it in console log
      # if you want to identify the specified load. We need to remove any
      # fragment or query params from the original url.
      (scheme, netloc, path, query, fragment) = urlparse.urlsplit(benchmark_url)
      log_match_url = urlparse.urlunsplit((scheme, netloc, path, '', ''))
      # Android may log URLs unescaped, so we must tolerate the mismatches.
      log_match_url_re = re.sub(
          r'(%[0-9A-Fa-f][0-9A-Fa-f])', r'(\1|.)', re.escape(log_match_url))

    activity_started_re = re.compile(
        '.*ActivityManager: START.*%s.*%s.*' % (
            log_match_url_re, re.escape(browser_package)))
    self.adb.StartActivity(browser_package, browser_activity, action=action,
                           data=benchmark_url, extras=browser_extras,
                           trace_file_name=PerfTestRunner.TARGET_TRACE_FILE
                                           if self.trace else None)
    m = self.WaitForLogMatchOrPackageCrash(
        activity_started_re, browser_package, url)
    assert m
    start_line = m.group(0)
    print 'Starting %s...' % browser_package
    return android_commands.GetLogTimestamp(start_line)

  def CollectTraceIfNeeded(self, host_file_name, delay_until_trace_ready):
    """Collect both traceview and chrome trace files from the device."""
    if not self.trace:
      return
    host_file_name = os.path.join(CHROME_DIR, host_file_name.replace('/', '_'))
    print 'Waiting for tracing to complete...'
    time.sleep(max(delay_until_trace_ready, 2.5))
    print 'Collecting traceview file: %s' % host_file_name
    host_trace_view = host_file_name + '.traceview'
    for i in xrange(sys.maxint):
      if not os.path.exists(host_trace_view):
        break
      host_trace_view = host_file_name +'_' + str(i) + '.traceview'
    self.adb.Adb().Pull(PerfTestRunner.TARGET_TRACE_FILE, host_trace_view)
    CHROME_TRACE_DIRECTORY = '/sdcard/Download/'
    CHROME_TRACE_FILE_PATTERN = 'chrome-profile-results-*'
    device_contents = self.adb.ListPathContents(CHROME_TRACE_DIRECTORY)
    for device_content in device_contents:
      if fnmatch.fnmatch(device_content, CHROME_TRACE_FILE_PATTERN):
        print 'Collecting chrome_trace: %s' % device_content
        device_content = os.path.join(CHROME_TRACE_DIRECTORY, device_content)
        trace_event_name = os.path.join(CHROME_DIR,
                                        host_file_name + '_' +
                                        os.path.basename(device_content) +
                                        '.chrometrace')
        self.adb.Adb().Pull(device_content, trace_event_name)
        self.adb.RunShellCommand('rm %s' % device_content)

  def _RunUrlCyclerPerfTest(self, browser_package, browser_activity,
                            browser_console_log_re, urls, expected_results,
                            trace_tag='', timeout=30):
    """Runs a JavaScript based performance test on a list of cycling URLs.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      browser_package: The package of the browser to start (e.g.
          'com.google.android.apps.chrome').
      browser_activity: The activity of the browser to start (e.g. 'Main').
      browser_console_log_re: Regular expression string which identifies
          console.log output in adb logcat. Must contain a %s placeholder for
          the log_marker.
      urls: List of URLs to load. The caller is responsible for ensuring this
          URL is accessible on the phone.
      expected_results: A list of tuple of (log_marker, units).
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    self.adb.StartMonitoringLogcat(timeout=timeout)
    self.adb.ClearApplicationState(browser_package)
    self.StartupBrowser(browser_package, browser_activity, url=urls[0])

    results = collections.defaultdict(list)
    pss_usages = []
    private_dirty_usages = []
    try:
      for i, url in enumerate(urls):
        group_floor = 1 + 10 * (i / 10)
        group_ceil = group_floor + 9
        group = '%d-%d' % (group_floor, group_ceil)
        display_url = url.replace('http://', '')

        for log_marker, units in expected_results:
          result_re_str = browser_console_log_re % (
              log_marker + '_' + re.escape(url))
          result_re = re.compile(result_re_str)
          m = self.WaitForLogMatchOrPackageCrash(
              result_re, browser_package, url)
          if not m:
            return False
          result = m.group(1).split(',')
          results[log_marker] += result

          # Create chart tabs for 1-10, 11-20, etc.
          # Each tab displays 10 individual page traces.
          PrintPerfResult(log_marker + trace_tag + '_' + group,
                          log_marker + '_' + display_url,
                          result, units)

        # Sample memory usage after each collection of expectations.
        memory_usage = self.adb.GetMemoryUsage(browser_package)
        pss_usages.append(memory_usage['Pss'])
        private_dirty_usages.append(memory_usage['Private_Dirty'])
        PrintPerfResult('pss' + trace_tag + '_' + group, 'pss_' + display_url,
                        [memory_usage['Pss']], 'kb')
        PrintPerfResult('private_dirty' + trace_tag + '_' + group,
                        'private_dirty_' + display_url,
                        [memory_usage['Private_Dirty']], 'kb')
        if 'Nvidia' in memory_usage:
          PrintPerfResult('nvidia' + trace_tag + '_' + group,
                          'nvidia_' + display_url,
                          [memory_usage['Nvidia']], 'kb')

      # Create a chart tabs for averages of all pages.
      for log_marker, units in expected_results:
        PrintPerfResult(log_marker + '_avg', log_marker + '_avg' + trace_tag,
                        results[log_marker], units)
      PrintPerfResult('pss_avg', 'pss_avg' + trace_tag, pss_usages, 'kb')
      PrintPerfResult('private_dirty_avg', 'private_dirty_avg' + trace_tag,
                      private_dirty_usages, 'kb')
    except pexpect.TIMEOUT:
      PerfTestRunner.OutputFailure(
          'Timed out after %d seconds while waiting for %s' % (timeout,
                                                               result_re_str))
      return False
    finally:
      self.adb.CloseApplication(browser_package)
    return True

  def _RunBackgroundMemoryPerfTest(self, browser_package, browser_activity,
                                   browser_console_log_re, urls,
                                   expected_results, trace_tag='', timeout=30):
    """Measure memory usage while browser has the given URLs open but is hidden.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      browser_package: The package of the browser to start (e.g.
          'com.google.android.apps.chrome').
      browser_activity: The activity of the browser to start (e.g. 'Main').
      browser_console_log_re: Regular expression string which identifies
          console.log output in adb logcat. Must contain a %s placeholder for
          the log_marker.
      urls: List of URLs to load. The caller is responsible for ensuring this
          URL is accessible on the phone.
      expected_results: A list of log marker strings.
      trace_tag: An optional tag string to append to all trace_names.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      True if the test ran successfully.
    """
    self.adb.StartMonitoringLogcat(timeout=timeout)
    self.adb.ClearApplicationState(browser_package)

    try:
      for url in urls:
        self.StartupBrowser(browser_package, browser_activity, url=url,
                            browser_extras={'create_new_tab': True})
        # Wait for results to ensure the page loaded. We don't log any values.
        for expected_result in expected_results:
          result_re_str = browser_console_log_re % re.escape(expected_result)
          result_re = re.compile(result_re_str)
          m = self.WaitForLogMatchOrPackageCrash(
              result_re, browser_package, url)
          if not m:
            return False

      self.adb.StartActivity(package=None, activity=None,
                             action="android.intent.action.MAIN",
                             category="android.intent.category.HOME",
                             wait_for_completion=True)

      memory_usage = self.adb.GetMemoryUsage(browser_package)
      PrintPerfResult('pss' + trace_tag, 'pss',
                      [memory_usage['Pss']], 'kb')
      PrintPerfResult('private_dirty' + trace_tag, 'private_dirty',
                      [memory_usage['Private_Dirty']], 'kb')
      if 'Nvidia' in memory_usage:
        PrintPerfResult('nvidia' + trace_tag, 'nvidia',
                        [memory_usage['Nvidia']], 'kb')

      time.sleep(5)

      # TODO(tonyg, husky): factor out this common code! (across all methods)
      memory_usage = self.adb.GetMemoryUsage(browser_package)
      PrintPerfResult('pss' + trace_tag, 'pss_after_5_secs',
                      [memory_usage['Pss']], 'kb')
      PrintPerfResult('private_dirty' + trace_tag, 'private_dirty_after_5_secs',
                      [memory_usage['Private_Dirty']], 'kb')
      if 'Nvidia' in memory_usage:
        PrintPerfResult('nvidia' + trace_tag, 'nvidia_after_5_secs',
                        [memory_usage['Nvidia']], 'kb')
    except pexpect.TIMEOUT:
      PerfTestRunner.OutputFailure(
          'Timed out after %d seconds while waiting for page to load' % timeout)
      return False
    finally:
      self.adb.CloseApplication(browser_package)
    return True

  def _RunPerfTest(self, browser_package, browser_activity,
                   browser_console_log_re, url, expected_results,
                   trace_tag='', browser_extras=None, timeout=30):
    """Runs a JavaScript based performance test.

    The results are printed to the console in a format suitable for the perfbot.

    Args:
      browser_package: The package of the browser to start (e.g.
          'com.google.android.apps.chrome').
      browser_activity: The activity of the browser to start (e.g. '.Main' or
          'com.google.android.apps.chrome.Main').
      browser_console_log_re: Regular expression string which identifies
          console.log output in adb logcat. Must contain a %s placeholder for
          the log_marker.
      url: The URL of the JavaScript performance test. The caller is responsible
          for ensuring this URL is accessible on the phone (either by copying
          locally or starting an HTTP server + forwarder).
      expected_results: A list of tuple of (log_marker, chart_name, trace_name,
          units). log_marker is usually shown as the graph name, with the
          exception of SURFACE_FPS_MONITOR_START/STOP which are used to control
          the monitor of page flip count of Surface.
      trace_tag: An optional tag string to append to all trace_names.
      browser_extras: Extra data to pass to the browser Intent.
      timeout: The browser is killed after this many seconds of inactivity.

    Returns:
      List of results if test ran successfully. None upon failure.
    """
    self.adb.StartMonitoringLogcat(timeout=timeout)
    self.adb.ClearApplicationState(browser_package)
    error_re = re.compile(ACTIVITY_CRASH_RE % re.escape(browser_package))

    results = []
    io_stats_before = self.adb.GetIoStats()
    self.StartupBrowser(browser_package, browser_activity,
                        token=str(random.randint(100000, 999999)), url=url,
                        browser_extras=browser_extras)
    try:
      for log_marker, chart_name, trace_name, units in expected_results:
        result_re_str = browser_console_log_re % re.escape(log_marker)
        result_re = re.compile(result_re_str)
        m = self.WaitForLogMatchOrPackageCrash(result_re, browser_package, url)
        if not m:
          return None
        # For certain tests, the result is a list enclosed in braces, as in:
        #  '{3.134553, 40389443}'; remove these if we find them before
        # splitting, otherwise we'll get an error when converting result[0]
        # to a float below. Same for angle brackets which also happen.
        result = m.group(1)
        if len(result) > 2:
          if result[0] == '{' and result[-1] == '}':
            result = result[1:-1]
          elif result[0] == '[' and result[-1] == ']':
            result = result[1:-1]
        result = result.split(',')
        results.append(float(result[0]))
        if log_marker == SURFACE_FPS_MONITOR_START:
          surface_before = self.adb.GetSurfaceStats()
        elif log_marker == SURFACE_FPS_MONITOR_STOP:
          surface_after = self.adb.GetSurfaceStats()
          td = surface_after['timestamp'] - surface_before['timestamp']
          seconds = td.seconds + td.microseconds / 1e6
          print 'SurfaceMonitorTime: %fsecs' % seconds
          surface_fps = (surface_after['page_flip_count'] -
                         surface_before['page_flip_count']) / seconds
          PrintPerfResult('avg_surface_fps', 'avg_surface_fps' + trace_tag,
                          [int(round(surface_fps))], 'fps')
        else:
          PrintPerfResult(chart_name, trace_name + trace_tag, result, units)
      memory_usage = self.adb.GetMemoryUsage(browser_package)
      PrintPerfResult('pss_final_t', 'pss_final_t' + trace_tag,
                      [memory_usage['Pss']], 'kb')
      PrintPerfResult('private_dirty_final_t',
                      'private_dirty_final_t' + trace_tag,
                      [memory_usage['Private_Dirty']], 'kb')
      if 'Nvidia' in memory_usage:
        PrintPerfResult('nvidia_final_t', 'nvidia_final_t' + trace_tag,
                        [memory_usage['Nvidia']], 'kb')
      io_stats_after = self.adb.GetIoStats()
      for stat in io_stats_after:
        PrintPerfResult(stat, stat + trace_tag,
                        [io_stats_after[stat] - io_stats_before[stat]],
                        stat.split('_')[1])
    except pexpect.TIMEOUT:
      PerfTestRunner.OutputFailure(
          'Timed out after %d seconds while waiting for %s' % (timeout,
                                                               result_re_str))
      return None
    finally:
      self.adb.CloseApplication(browser_package)
    return results
