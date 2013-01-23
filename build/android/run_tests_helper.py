# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper functions common to native, java and python test runners."""

import contextlib
import fcntl
import httplib
import logging
import optparse
import os
import re
import socket
import subprocess
import sys
import traceback

import cmd_helper
import constants


CHROME_PACKAGE = 'com.google.android.apps.chrome'
CHROME_ACTIVITY = 'com.google.android.apps.chrome.Main'
CHROME_TESTS_PACKAGE = 'com.google.android.apps.chrome.tests'
MUSIC_PACKAGE = 'com.google.android.music'
MUSIC_ACTIVITY = 'com.android.music.activitymanagement.TopLevelActivity'

LEGACY_BROWSER_PACKAGE = 'com.google.android.browser'
LEGACY_BROWSER_ACTIVITY = 'com.android.browser.BrowserActivity'

FORWARDER_PATH = constants.TEST_EXECUTABLE_DIR + '/forwarder'

CHROME_DIR = os.path.abspath(os.path.join(sys.path[0], '..', '..'))

# Ports arrangement for various test servers used in Clank.
# Lighttpd server will attempt to use 9000 as default port, if unavailable it
# will find a free port from 8001 - 8999.
LIGHTTPD_DEFAULT_PORT = 9000
LIGHTTPD_RANDOM_PORT_FIRST = 8001
LIGHTTPD_RANDOM_PORT_LAST = 8999
# Ports 8001-9000 may be used for lighttpd test servers. The net test server is
# started from 9001.
# Reserve 30 ports for test servers should be enough for running related tests
# on shards.
TEST_SERVER_PORT_FIRST = 9001
TEST_SERVER_PORT_LAST = 9030
TEST_SYNC_SERVER_PORT = 9031

# A file to record next valid port of test server.
TEST_SERVER_PORT_FILE = '/tmp/test_server_port'
TEST_SERVER_PORT_LOCKFILE = '/tmp/test_server_port.lock'

#The following two methods are used to allocate the port source for various
# types of test servers. Because some net relates tests can be run on shards
# at same time, it's important to have a mechanism to allocate the port process
# safe. In here, we implement the safe port allocation by leveraging flock.
def ResetTestServerPortAllocation():
  """Reset the port allocation to start from TEST_SERVER_PORT_FIRST.

  Returns:
    Returns True if reset successes. Otherwise returns False.
  """
  try:
    with open(TEST_SERVER_PORT_FILE, 'w') as fp:
      fp.write('%d' % TEST_SERVER_PORT_FIRST)
    if os.path.exists(TEST_SERVER_PORT_LOCKFILE):
      os.unlink(TEST_SERVER_PORT_LOCKFILE)
    return True
  except Exception as e:
    logging.error(e)
  return False

def AllocateTestServerPort():
  """Allocate a port incrementally.

  Returns:
    Returns a valid port which should be in between TEST_SERVER_PORT_FIRST and
    TEST_SERVER_PORT_LAST. Returning 0 means no more valid port can be used.
  """
  port = 0
  try:
    fp_lock = open(TEST_SERVER_PORT_LOCKFILE, 'w');
    fcntl.flock(fp_lock, fcntl.LOCK_EX)
    # Get current valid port and calculate next valid port.
    assert os.path.exists(TEST_SERVER_PORT_FILE)
    with open(TEST_SERVER_PORT_FILE, 'r+') as fp:
      port = int(fp.read())
      if port > TEST_SERVER_PORT_LAST or port < TEST_SERVER_PORT_FIRST:
        port = 0
      else:
        fp.seek(0, os.SEEK_SET)
        fp.write('%d' % (port + 1))
  except Exception as e:
    logging.info(e)
  finally:
    if fp_lock:
      fcntl.flock(fp_lock, fcntl.LOCK_UN)
      fp_lock.close()
  logging.info('Allocate port %d for test server.' % port)
  return port

def ReportBuildbotLink(label, url):
  """Adds a link with name |label| linking to |url| to current buildbot step.

  Args:
    label: A string with the name of the label.
    url: A string of the URL.
  """
  print '@@@STEP_LINK@%s@%s@@@' % (label, url)


def ReportBuildbotMsg(msg):
  """Appends |msg| to the current buildbot step text.

  Args:
    msg: String to be appended.
  """
  print '@@@STEP_TEXT@%s@@@' % msg

def ReportBuildbotError():
  """Marks the current step as failed."""
  print '@@@STEP_FAILURE@@@'


def GetExpectations(file_name):
  """Returns a list of test names in the |file_name| test expectations file."""
  if not file_name or not os.path.exists(file_name):
    return []
  return [x for x in [x.strip() for x in file(file_name).readlines()]
          if x and x[0] != '#']


def SetLogLevel(verbose_count):
  """Sets log level as |verbose_count|."""
  log_level = logging.WARNING  # Default.
  if verbose_count == 1:
    log_level = logging.INFO
  elif verbose_count >= 2:
    log_level = logging.DEBUG
  logging.getLogger().setLevel(log_level)


def CreateTestRunnerOptionParser(usage=None, default_timeout=60):
  """Returns a new OptionParser with arguments applicable to all tests."""
  option_parser = optparse.OptionParser(usage=usage)
  option_parser.add_option('-t', dest='timeout',
                           help='Timeout to wait for each test',
                           type='int',
                           default=default_timeout)
  option_parser.add_option('-c', dest='cleanup_test_files',
                           help='Cleanup test files on the device after run',
                           action='store_true',
                           default=False)
  option_parser.add_option('-v',
                           '--verbose',
                           dest='verbose_count',
                           default=0,
                           action='count',
                           help='Verbose level (multiple times for more)')
  option_parser.add_option('--tool',
                           dest='tool',
                           help='Run the test under a tool '
                           '(use --tool help to list them)')
  return option_parser


def ForwardDevicePorts(adb, ports, host_name='127.0.0.1'):
  """Forwards a TCP port on the device back to the host.

  Works like adb forward, but in reverse.

  Args:
    adb: Instance of AndroidCommands for talking to the device.
    ports: A list of tuples (device_port, host_port) to forward.
    host_name: Optional. Address to forward to, must be addressable from the
               host machine. Usually this is omitted and loopback is used.

  Returns:
    subprocess instance connected to the forwarder process on the device.
  """
  adb.PushIfNeeded(
      os.path.join(CHROME_DIR, 'out', 'Release', 'forwarder'), FORWARDER_PATH)
  forward_string = ['%d:%d:%s' %
                    (device, host, host_name) for device, host in ports]
  logging.info("Forwarding ports: %s" % (forward_string))

  return subprocess.Popen(
      ['adb', '-s', adb._adb.GetSerialNumber(),
       'shell', '%s -D %s' % (FORWARDER_PATH, ' '.join(forward_string))])


def IsHostPortUsed(host_port):
  """Checks whether the specified host port is used or not.

  Args:
    host_port: Port on host we want to check.

  Returns:
    True if the port on host is already used, otherwise returns False.
  """
  port_info = 'localhost:%d' % host_port
  logging.info('Host port: %s' % port_info)
  # TODO(jnd): Find a better way to filter the port.
  re_port = re.compile(re.escape(port_info), re.MULTILINE)
  if re_port.findall(cmd_helper.GetCmdOutput(['lsof', '-i:%d' % host_port])):
    return True
  return False


def IsDevicePortUsed(adb, device_port):
  """Checks whether the specified device port is used or not.

  Args:
    adb: Instance of AndroidCommands for talking to the device.
    device_port: Port on device we want to check.

  Returns:
    True if the port on device is already used, otherwise returns False.
  """
  base_url = '127.0.0.1:%d' % device_port
  netstat_results = adb.RunShellCommand('netstat')
  for single_connect in netstat_results:
    # Column 3 is the local address which we want to check with.
    if single_connect.split()[3] == base_url:
      return True
  return False


def IsHttpServerConnectable(host, port, tries=3, command='GET', path='/',
                            expected_read='', timeout=2):
  """Checks whether the specified http server is ready to serve request or not.

  Args:
    host: Host name of the HTTP server.
    port: Port number of the HTTP server.
    tries: How many times we want to test the connection. The default value is
           3.
    command: The http command we use to connect to HTTP server. The default
             command is 'GET'.
    path: The path we use when connecting to HTTP server. The default path is
          '/'.
    expected_read: The content we expect to read from the response. The default
                   value is ''.
    timeout: Timeout (in seconds) for each http connection. The default is 2s.

  Returns:
    Tuple of (connect status, client error). connect status is a boolean value
    to indicate whether the server is connectable. client_error is the error
    message the server returns when connect status is false.
  """
  assert tries >= 1
  for i in xrange(0, tries):
    client_error = None
    try:
      with contextlib.closing(httplib.HTTPConnection(
          host, port, timeout=timeout)) as http:
        # Output some debug information when we have tried more than 2 times.
        http.set_debuglevel(i >= 2)
        http.request(command, path)
        r = http.getresponse()
        content = r.read()
        if (r.status == 200 and r.reason == 'OK' and content == expected_read):
          return (True, '')
        client_error = ('Bad response: %s %s version %s\n  ' %
                        (r.status, r.reason, r.version) +
                        '\n  '.join([': '.join(h) for h in r.getheaders()]))
    except (httplib.HTTPException, socket.error) as e:
      # Probably too quick connecting: try again.
      exception_error_msgs = traceback.format_exception_only(type(e), e)
      if exception_error_msgs:
        client_error = ''.join(exception_error_msgs)
  # Only returns last client_error.
  return (False, client_error or 'Timeout')
