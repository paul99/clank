# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Finds android browsers that can be controlled by telemetry."""

import os
import logging as real_logging
import re
import subprocess

from telemetry import adb_commands
from telemetry import android_browser_backend
from telemetry import android_platform
from telemetry import browser
from telemetry import possible_browser

ALL_BROWSER_TYPES = ','.join([
    'android-content-shell',
    'android-chrome',
    'android-jb-system-chrome',
    ])

CHROME_PACKAGE = 'com.google.android.apps.chrome'
CHROME_ACTIVITY = '.Main'
CHROME_COMMAND_LINE = '/data/local/chrome-command-line'
CHROME_DEVTOOLS_REMOTE_PORT = 'localabstract:chrome_devtools_remote'

CHROME_JB_SYSTEM_PACKAGE = 'com.android.chrome'
CHROME_JB_SYSTEM_DEVTOOLS_REMOTE_PORT = 'localabstract:chrome_devtools_remote'

CONTENT_SHELL_PACKAGE = 'org.chromium.content_shell'
CONTENT_SHELL_ACTIVITY = '.ContentShellActivity'
CONTENT_SHELL_COMMAND_LINE = '/data/local/tmp/content-shell-command-line'
CONTENT_SHELL_DEVTOOLS_REMOTE_PORT = (
    'localabstract:content_shell_devtools_remote')

# adb shell pm list packages
# adb
# intents to run (pass -D url for the rest)
#   com.android.chrome/.Main
#   com.google.android.apps.chrome/.Main

class PossibleAndroidBrowser(possible_browser.PossibleBrowser):
  """A launchable android browser instance."""
  def __init__(self, browser_type, options, *args):
    super(PossibleAndroidBrowser, self).__init__(
        browser_type, options)
    self._args = args

  def __repr__(self):
    return 'PossibleAndroidBrowser(browser_type=%s)' % self.browser_type

  def Create(self):
    backend = android_browser_backend.AndroidBrowserBackend(
        self._options, *self._args)
    platform = android_platform.AndroidPlatform(
        self._args[0].Adb(), self._args[1],
        self._args[1] + self._args[4])
    b = browser.Browser(backend, platform)
    backend.SetBrowser(b)
    return b

def FindAllAvailableBrowsers(options, logging=real_logging):
  """Finds all the desktop browsers available on this machine."""
  if not adb_commands.IsAndroidSupported():
    return []

  # See if adb even works.
  try:
    with open(os.devnull, 'w') as devnull:
      proc = subprocess.Popen(['adb', 'devices'],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,
                              stdin=devnull)
      stdout, _ = proc.communicate()
      if re.search(re.escape('????????????\tno permissions'), stdout) != None:
        logging.warn(
            ('adb devices reported a permissions error. Consider '
            'restarting adb as root:'))
        logging.warn('  adb kill-server')
        logging.warn('  sudo `which adb` devices\n\n')
  except OSError:
    logging.info('No adb command found. ' +
             'Will not try searching for Android browsers.')
    return []

  device = None
  if options.android_device:
    devices = [options.android_device]
  else:
    devices = adb_commands.GetAttachedDevices()

  if len(devices) == 0:
    logging.info('No android devices found.')
    return []

  if len(devices) > 1:
    logging.warn('Multiple devices attached. ' +
                 'Please specify a device explicitly.')
    return []

  device = devices[0]

  adb = adb_commands.AdbCommands(device=device)

  # See if adb is root
  if not adb.IsRootEnabled():
    logging.warn('ADB is not root. Please make it root by doing:')
    logging.warn(' adb root')
    return []

  packages = adb.RunShellCommand('pm list packages')
  possible_browsers = []
  if 'package:' + CONTENT_SHELL_PACKAGE in packages:
    b = PossibleAndroidBrowser('android-content-shell',
                               options, adb,
                               CONTENT_SHELL_PACKAGE, True,
                               CONTENT_SHELL_COMMAND_LINE,
                               CONTENT_SHELL_ACTIVITY,
                               CONTENT_SHELL_DEVTOOLS_REMOTE_PORT)
    possible_browsers.append(b)

  if 'package:' + CHROME_PACKAGE in packages:
    b = PossibleAndroidBrowser('android-chrome',
                               options, adb,
                               CHROME_PACKAGE, False,
                               CHROME_COMMAND_LINE,
                               CHROME_ACTIVITY,
                               CHROME_DEVTOOLS_REMOTE_PORT)
    possible_browsers.append(b)

  if 'package:' + CHROME_JB_SYSTEM_PACKAGE in packages:
    b = PossibleAndroidBrowser('android-jb-system-chrome',
                               options, adb,
                               CHROME_JB_SYSTEM_PACKAGE, False,
                               CHROME_COMMAND_LINE,
                               CHROME_ACTIVITY,
                               CHROME_JB_SYSTEM_DEVTOOLS_REMOTE_PORT)
    possible_browsers.append(b)

  # See if the "forwarder" is installed -- we need this to host content locally
  # but make it accessible to the device.
  if len(possible_browsers) and not adb_commands.HasForwarder():
    logging.warn('telemetry detected an android device. However,')
    logging.warn('Chrome\'s port-forwarder app is not available.')
    logging.warn('To build:')
    logging.warn('  make -j16 host_forwarder device_forwarder')
    logging.warn('')
    logging.warn('')
    return []
  return possible_browsers
