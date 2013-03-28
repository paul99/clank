# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import os
import socket
import subprocess

from telemetry import browser_backend
from telemetry import util

class CrOSBrowserBackend(browser_backend.BrowserBackend):
  def __init__(self, browser_type, options, is_content_shell, cri):
    super(CrOSBrowserBackend, self).__init__(is_content_shell, options)
    # Initialize fields so that an explosion during init doesn't break in Close.
    self._options = options
    assert not is_content_shell
    self._cri = cri
    self._browser_type = browser_type

    self._remote_debugging_port = self._cri.GetRemotePort()
    self._login_ext_dir = '/tmp/chromeos_login_ext'

    # Ensure the UI is running and logged out.
    self._RestartUI()

    # Delete test@test.test's cryptohome vault (user data directory).
    if not options.dont_override_profile:
      logging.info('Deleting user\'s cryptohome vault (the user data dir)')
      self._cri.GetCmdOutput(
          ['cryptohome', '--action=remove', '--force', '--user=test@test.test'])

    # Push a dummy login extension to the device.
    # This extension automatically logs in as test@test.test
    logging.info('Copying dummy login extension to the device')
    cri.PushFile(
        os.path.join(os.path.dirname(__file__), 'chromeos_login_ext'), '/tmp/')
    cri.GetCmdOutput(['chown', '-R', 'chronos:chronos', self._login_ext_dir])

    # Restart Chrome with the login extension and remote debugging.
    logging.info('Restarting Chrome with flags and login')
    args = ['dbus-send', '--system', '--type=method_call',
            '--dest=org.chromium.SessionManager',
            '/org/chromium/SessionManager',
            'org.chromium.SessionManagerInterface.EnableChromeTesting',
            'boolean:true',
            'array:string:"%s"' % '","'.join(self.GetBrowserStartupArgs())]
    cri.GetCmdOutput(args)

    # Find a free local port.
    tmp = socket.socket()
    tmp.bind(('', 0))
    self._port = tmp.getsockname()[1]
    tmp.close()

    # Forward the remote debugging port.
    logging.info('Forwarding remote debugging port')
    self._forwarder = SSHForwarder(
      cri, 'L', (self._port, self._remote_debugging_port))

    # Wait for the browser to come up.
    logging.info('Waiting for browser to be ready')
    try:
      self._WaitForBrowserToComeUp()
      self._PostBrowserStartupInitialization()
    except:
      import traceback
      traceback.print_exc()
      self.Close()
      raise

    # Make sure there's a tab open.
    if len(self.tabs) == 0:
      self.tabs.New()

    logging.info('Browser is up!')

  def GetBrowserStartupArgs(self):
    args = super(CrOSBrowserBackend, self).GetBrowserStartupArgs()

    args.extend([
            '--allow-webui-compositing',
            '--aura-host-window-use-fullscreen',
            '--enable-smooth-scrolling',
            '--enable-threaded-compositing',
            '--enable-per-tile-painting',
            '--enable-gpu-sandboxing',
            '--force-compositing-mode',
            '--remote-debugging-port=%i' % self._remote_debugging_port,
            '--auth-ext-path=%s' % self._login_ext_dir,
            '--start-maximized'])

    return args

  def __del__(self):
    self.Close()

  def Close(self):
    self._RestartUI() # Logs out.

    if self._forwarder:
      self._forwarder.Close()
      self._forwarder = None

    if self._login_ext_dir:
      self._cri.RmRF(self._login_ext_dir)
      self._login_ext_dir = None

    self._cri = None

  def IsBrowserRunning(self):
    # On ChromeOS, there should always be a browser running.
    for _, process in self._cri.ListProcesses():
      if process.startswith('/opt/google/chrome/chrome'):
        return True
    return False

  def GetStandardOutput(self):
    return 'Cannot get standard output on CrOS'

  def CreateForwarder(self, *ports):
    assert self._cri
    return SSHForwarder(self._cri, 'R', *ports)

  def _RestartUI(self):
    if self._cri:
      logging.info('(Re)starting the ui (logs the user out)')
      if self._cri.IsServiceRunning('ui'):
        self._cri.GetCmdOutput(['restart', 'ui'])
      else:
        self._cri.GetCmdOutput(['start', 'ui'])


class SSHForwarder(object):
  def __init__(self, cri, forwarding_flag, *ports):
    self._proc = None
    self._host_port = ports[0][0]

    port_pairs = []

    for port in ports:
      if port[1] is None:
        port_pairs.append((port[0], cri.GetRemotePort()))
      else:
        port_pairs.append(port)

    if forwarding_flag == 'R':
      self._device_port = port_pairs[0][0]
    else:
      self._device_port = port_pairs[0][1]

    self._proc = subprocess.Popen(
      cri.FormSSHCommandLine(
        ['sleep', '999999999'],
        ['-%s%i:localhost:%i' % (forwarding_flag, from_port, to_port)
        for from_port, to_port in port_pairs]),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      stdin=subprocess.PIPE,
      shell=False)

    util.WaitFor(lambda: cri.IsHTTPServerRunningOnPort(self._device_port), 60)

  @property
  def url(self):
    assert self._proc
    return 'http://localhost:%i' % self._host_port

  def Close(self):
    if self._proc:
      self._proc.kill()
      self._proc = None
