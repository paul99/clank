#!/usr/bin/python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os
import sys

import cmd_helper
import shutil
import tempfile
from test_package import TestPackage


class TestPackageApk(TestPackage):
  """A helper class for running APK-based native tests.

  Args:
    adb: ADB interface the tests are using.
    device: Device to run the tests.
    test_suite: A specific test suite to run, empty to run all.
    timeout: Timeout for each test.
    rebaseline: Whether or not to run tests in isolation and update the filter.
    performance_test: Whether or not performance test(s).
    cleanup_test_files: Whether or not to cleanup test files on device.
    tool: Name of the Valgrind tool.
    dump_debug_info: A debug_info object.
  """

  APK_DATA_DIR = '/data/user/0/com.google.android.apps.chrome.native_tests/files/'

  def __init__(self, adb, device, test_suite, timeout, rebaseline,
               performance_test, cleanup_test_files, tool,
               dump_debug_info):
    TestPackage.__init__(self, adb, device, test_suite, timeout,
                         rebaseline, performance_test, cleanup_test_files,
                         tool, dump_debug_info)

  def _CreateTestRunnerScript(self, options):
    tool_wrapper = self.tool.GetTestWrapper()
    if tool_wrapper:
      raise RuntimeError("TestPackageApk does not support custom wrappers.")
    command_line_file = tempfile.NamedTemporaryFile()
    # GTest expects argv[0] to be the executable path.
    command_line_file.write(self.test_suite_basename + ' ' + options)
    command_line_file.flush()
    self.adb.PushIfNeeded(command_line_file.name,
                          TestPackageApk.APK_DATA_DIR +
                          'chrome-native-tests-command-line')

  def _GetGTestReturnCode(self):
    return None

  def GetAllTests(self):
    """Returns a list of all tests available in the test suite."""
    self._CreateTestRunnerScript('--gtest_list_tests')
    self.adb.RunShellCommand('am start -n '
                             'com.google.android.apps.chrome.native_tests/'
                             'android.app.NativeActivity')
    stdout_file = tempfile.NamedTemporaryFile()
    ret = []
    self.adb.Adb().Pull(TestPackageApk.APK_DATA_DIR + 'stdout.txt',
                        stdout_file.name)
    ret = self._ParseGTestListTests(stdout_file)
    return ret

  def CreateTestRunnerScript(self, gtest_filter, test_arguments):
     self._CreateTestRunnerScript('--gtest_filter=%s %s' % (gtest_filter,
                                                           test_arguments))

  def RunTestsAndListResults(self):
    self.adb.StartMonitoringLogcat(clear=True, logfile=sys.stdout)
    self.adb.RunShellCommand('am start -n '
                             'com.google.android.apps.chrome.native_tests/'
                             'com.google.android.apps.chrome.native_tests.ChromeNativeTestActivity')
    return self._WatchTestOutput(self.adb.GetMonitoredLogCat())

  def StripAndCopyExecutable(self):
    # TODO(bulach): the build system currently only builds lib$TEST_SUITE.so
    # or $TEST_SUITE executable so that we can have it under a flag and keep
    # both coexisting for the the time being. This means that we're
    # packaging the APK up here. Move this down to the build system once we're
    # done with moving the tests to APK.
    android_product_out = os.environ['ANDROID_PRODUCT_OUT']
    shutil.copy(os.path.join(self.test_suite_dirname,
                             'lib.target',
                             'lib' + self.test_suite_basename + '.so'),
                os.path.join(android_product_out,
                             'obj/lib/libnative_tests.so'))
    cmd_helper.RunCmd(['clank/build/clank_mmm', 'base/test/android'])
    self.adb.Adb().SendCommand('install -r ' +
                               os.path.join(android_product_out,
                                            'data/app/ChromeNativeTests.apk'),
                               timeout_time=60*5)
