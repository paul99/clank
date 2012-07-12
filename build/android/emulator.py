#!/usr/bin/env python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides an interface to start and stop Android emulator.

Assumes system environment ANDROID_NDK_ROOT has been set.

  Emulator: The class provides the methods to launch/shutdown the emulator with
            the android virtual device named 'buildbot' .
"""

import logging
import os
import signal
import subprocess
import sys
import time

import android_commands

# adb_interface.py is under ../../third_party/android/testrunner/
sys.path.append(os.path.join(os.path.abspath(os.path.dirname(__file__)), '..',
   '..', 'third_party', 'android', 'testrunner'))
import adb_interface
import cmd_helper
import errors
import run_command

class EmulatorLaunchException(Exception):
  """Emulator failed to launch."""
  pass

def _KillAllEmulators():
  """Kill all running emulators that look like ones we started.

  There are odd 'sticky' cases where there can be no emulator process
  running but a device slot is taken.  A little bot trouble and and
  we're out of room forever.
  """
  emulators = android_commands.GetEmulators()
  if not emulators:
    return
  for emu_name in emulators:
    cmd_helper.GetCmdOutput(['adb', '-s', emu_name, 'emu', 'kill'])
  logging.info('Emulator killing is async; give a few seconds for all to die.')
  for i in range(5):
    if not android_commands.GetEmulators():
      return
    time.sleep(1)


class PortPool(object):
  """Pool for emulator port starting position that changes over time."""
  _port_min = 5554
  _port_max = 5585
  _port_current_index = 0

  @classmethod
  def port_range(cls):
    """Return a range of valid ports for emulator use.

    The port must be an even number between 5554 and 5584.  Sometimes
    a killed emulator "hangs on" to a port long enough to prevent
    relaunch.  This is especially true on slow machines (like a bot).
    Cycling through a port start position helps make us resilient."""
    ports = range(cls._port_min, cls._port_max, 2)
    n = cls._port_current_index
    cls._port_current_index = (n + 1) % len(ports)
    return ports[n:] + ports[:n]


def _GetAvailablePort():
  """Returns an available TCP port for the console."""
  used_ports = []
  emulators = android_commands.GetEmulators()
  for emulator in emulators:
    used_ports.append(emulator.split('-')[1])
  for port in PortPool.port_range():
    if str(port) not in used_ports:
      return port


class Emulator(object):
  """Provides the methods to lanuch/shutdown the emulator.

  The emulator has the android virtual device named 'buildbot'.

  The emulator could use any even TCP port between 5554 and 5584 for the
  console communication, and this port will be part of the device name like
  'emulator-5554'. Assume it is always True, as the device name is the id of
  emulator managed in this class.

  Attributes:
    emulator: Path of Android's emulator tool.
    popen: Popen object of the running emulator process.
    device: Device name of this emulator.
  """

  # Signals we listen for to kill the emulator on
  _SIGNALS = (signal.SIGINT, signal.SIGHUP)

  # Time to wait for an emulator launch, in seconds.  This includes
  # the time to launch the emulator and a wait-for-device command.
  _LAUNCH_TIMEOUT = 120

  # Timeout interval of wait-for-device command before bouncing to a a
  # process life check.
  _WAITFORDEVICE_TIMEOUT = 5

  # Time to wait for a "wait for boot complete" (property set on device).
  _WAITFORBOOT_TIMEOUT = 300

  def __init__(self, fast_and_loose=False):
    """Init an Emulator.

    Args:
      fast_and_loose: Loosen up the rules for reliable running for speed.
        Intended for quick testing or re-testing.

    """
    try:
      android_sdk_root = os.environ['ANDROID_SDK_ROOT']
    except KeyError:
      logging.critical('The ANDROID_SDK_ROOT must be set to run the test on '
                       'emulator.')
      raise
    self.emulator = os.path.join(android_sdk_root, 'tools', 'emulator')
    self.popen = None
    self.device = None
    self.fast_and_loose = fast_and_loose

  def _DeviceName(self):
    """Return our device name."""
    port = _GetAvailablePort()
    return ('emulator-%d' % port, port)

  def Launch(self):
    """Launches the emulator and waits for package manager to startup.

    If fails, an exception will be raised.
    """
    _KillAllEmulators()  # just to be sure
    if not self.fast_and_loose:
      self._AggressiveImageCleanup()
    (self.device, port) = self._DeviceName()
    emulator_command = [
        self.emulator,
        # Speed up emulator launch by 40%.  Really.
        '-no-boot-anim',
        # The default /data size is 64M.
        # That's not enough for 4 unit test bundles and their data.
        '-partition-size', '1024',
        # Use a familiar name and port.
        '-avd', 'buildbot',
        '-port', str(port)]
    if not self.fast_and_loose:
      emulator_command.extend([
          # Wipe the data.  We've seen cases where an emulator
          # gets 'stuck' if we don't do this (every thousand runs or
          # so).
          '-wipe-data',
          ])
    # This will not return until device's package manager starts up or an
    # exception is raised.
    adb = android_commands.AndroidCommands(self.device)
    adb.WaitForDevicePm()      


  def Shutdown(self):
    """Shuts down the process started by launch."""
    if self.popen:
      self.popen.poll()
      if self.popen.returncode == None:
        self.popen.kill()
      self.popen = None

  def _ShutdownOnSignal(self, signum, frame):
    logging.critical('emulator _ShutdownOnSignal')
    for sig in self._SIGNALS:
      signal.signal(sig, signal.SIG_DFL)
    self.Shutdown()
    raise KeyboardInterrupt  # print a stack

  def _InstallKillHandler(self):
    """Install a handler to kill the emulator when we exit unexpectedly."""
    for sig in self._SIGNALS:
      signal.signal(sig, self._ShutdownOnSignal)

def main(argv):
  Emulator().launch()


if __name__ == '__main__':
  main(sys.argv)
