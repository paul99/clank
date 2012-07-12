# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A "Test Server Spawner" that handles killing/stopping per-test test servers.

It's used to accept requests from the device to spawn and kill instances of the
chrome test server on the host.
"""

import BaseHTTPServer
import logging
import json
import os
import sys
import threading
import time
import urlparse

import run_tests_helper

# Path that are needed to import testserver
cr_src = os.path.join(os.path.abspath(os.path.dirname(__file__)), '..', '..')
sys.path.append(os.path.join(cr_src, 'third_party'))
sys.path.append(os.path.join(cr_src, 'third_party', 'tlslite'))
sys.path.append(os.path.join(cr_src, 'third_party', 'pyftpdlib', 'src'))
sys.path.append(os.path.join(cr_src, 'net', 'tools', 'testserver'))
import testserver

_test_servers = []

def GetServerType(server_type_string):
  """Returns the server type to use when starting the test server.

  This function translate the command-line argument into the appropriate
  numerical constant.
  """
  if server_type_string not in testserver.SERVER_TYPES:
    raise NotImplementedError('Unknown server type: %s' % server_type)
  server_type = testserver.SERVER_TYPES[server_type_string]
  if server_type == testserver.SERVER_UDP_ECHO:
    raise Exception('Please do not run UDP echo tests because we do not have '
                    'a UDP forwarder tool.')
  return server_type


class TestServerOptions(object):
  """The class to store options for starting test server.
  """

  def __init__(self, test_server_arguments):
    # All options are processed by following the definitions in testserver.py.
    self.server_type = GetServerType(test_server_arguments['server-type'])
    self.port = test_server_arguments['port']
    self.log_to_console = test_server_arguments['log-to-console']
    self.data_dir = test_server_arguments['doc-root'] or 'chrome/test/data'
    if not os.path.isabs(self.data_dir):
      self.data_dir = os.path.join(cr_src, self.data_dir)
    self.file_root_url = '/files/'
    # Initialize SSL arguments.
    self.record_resume = False
    self.cert = False
    self.policy_keys = None
    self.policy_user = 'user@example.com'
    self.startup_pipe = None
    self.ssl_client_auth = False
    self.ssl_client_ca = []
    self.ssl_bulk_cipher = None
    if test_server_arguments.has_key('root-cert'):
      assert test_server_arguments.has_key('https-cert')
      self.cert = os.path.join(cr_src, test_server_arguments['https-cert'])
      if test_server_arguments.has_key('https-record-resume'):
        self.record_resume = True
      if test_server_arguments.has_key('ssl-client-auth'):
        self.ssl_client_auth = True
      if test_server_arguments.has_key('ssl-client-ca'):
        ca_list = test_server_arguments['ssl-client-ca']
        self.ssl_client_ca = [os.path.join(cr_src, path) for path in ca_list]
      if test_server_arguments.has_key('ssl-bulk-cipher'):
        self.ssl_bulk_cipher = test_server_arguments['ssl-bulk-cipher']


class SpawningServerRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
  """A handler used to process http GET/POST request.
  """

  def SendResponse(self, response_code, response_reason, additional_headers,
                   contents):
    """Generate response according to specified parameters and send it back
    to client.

    Args:
      response_code: number of the response status.
      response_reason: string of reason description of the response.
      additional_headers: dict of additional headers. Each key is the name of
                          the header, each value is the content of the header.
      contents: string of the contents we want to send to client.
    """
    self.send_response(response_code, response_reason)
    self.send_header('Content-type', 'text/html')
    for header_name in additional_headers:
      self.send_header(header_name, additional_headers[header_name])
    self.end_headers()
    self.wfile.write(contents)

  def CheckTestServerStatus(self, expected_status):
    """ Check the live status of the test server by querying whether the port
        of the test server exists or not

    Args:
      expected_status: boolean of expected status.
    Returns:
      Returns True if the status is expected. Otherwise returns False.
    """
    port = self.server.test_server_port
    for timeout in xrange(1, 5):
      if run_tests_helper.IsHostPortUsed(port) == expected_status:
        return True
      time.sleep(timeout)
    return False

  def do_POST(self):
    parsed_path = urlparse.urlparse(self.path)
    action = parsed_path.path
    logging.info('Action for POST method is: %s' % action)
    port = self.server.test_server_port
    if action == '/start':
      logging.info('Handling request to spawn a test webserver')
      content_type = self.headers.getheader('content-type')
      if content_type != 'application/json':
        raise Exception('Bad content-type for start request!')
      content_length = self.headers.getheader('content-length')
      if not content_length:
        content_length = 0
      try:
        content_length = int(content_length)
      except:
        raise Exception('Bad content-length for start request!')
      logging.info(content_length)
      test_server_argument_json = self.rfile.read(content_length)
      logging.info(test_server_argument_json)
      test_server_arguments = json.loads(test_server_argument_json)
      assert test_server_arguments['port'] == port
      self.webserver_thread = threading.Thread(
          target=self.SpawnTestWebServer, args=(test_server_arguments,))
      self.webserver_thread.setDaemon(True)
      self.webserver_thread.start()
      if self.CheckTestServerStatus(True):
        self.SendResponse(200, 'OK', {}, '%s:%d' % ('started', port))
        logging.info('Returned OK!!!')
      else:
        self.SendResponse(400, 'Bad request', {}, '')
        logging.info('Encounter problem!!!')
    else:
      logging.warning("Unknow action: %s" % action)

  def do_GET(self):
    parsed_path = urlparse.urlparse(self.path)
    action = parsed_path.path
    params = urlparse.parse_qs(parsed_path.query, keep_blank_values=1)
    logging.info('Action for GET method is: %s' % action)
    for param in params:
      logging.info('%s=%s' % (param, params[param][0]))
    port = self.server.test_server_port
    message = ''
    expect_exist = False
    if action == '/killserver':
      # There should only ever be one test server at a time. This may do the
      # wrong thing if we try and start multiple test servers.
      logging.info('Handling request to kill a test webserver')
      assert 'port' in params
      assert int(params['port'][0]) == port
      _test_servers.pop().Stop()
      message = 'killed'
    elif action == '/start':
      raise Exception('Do not support start request with GET method.')
    elif action == '/ping':
      # The ping handler is used to check whether the spawner server is ready
      # to serve the requests. We don't need to test the status of the test
      # server when handling ping request.
      self.SendResponse(200, 'OK', {}, '%s:%d' % ('ready', port))
      logging.info('Handled ping request and sent response!!!')
      return
    logging.info('Port of test server: %d' % port)
    # Make sure the status of test server is correct before sending response.
    if self.CheckTestServerStatus(expect_exist):
      self.SendResponse(200, 'OK', {}, '%s:%d' % (message, port))
      logging.info('Returned OK!!!')
    else:
      self.SendResponse(400, 'Bad request', {}, message)
      logging.info('Encounter problem!!!')

  def SpawnTestWebServer(self, test_server_arguments):
    options = TestServerOptions(test_server_arguments)
    logging.info('Listening on %d, type %d, data_dir %s' % (options.port,
        options.server_type, options.data_dir))
    testserver.main(options, None, server_list=_test_servers)
    logging.info('Test-server has died.')


class SpawningServer(object):
  """The class used to start/stop a http server.
  """

  def __init__(self, test_server_spawner_port, test_server_port):
    logging.info('Creating new spawner %d', test_server_spawner_port)
    self.server = testserver.StoppableHTTPServer(('', test_server_spawner_port),
                                                 SpawningServerRequestHandler)
    self.port = test_server_spawner_port
    self.server.test_server_port = test_server_port

  def Listen(self):
    logging.info('Starting test server spawner')
    self.server.serve_forever()

  def Start(self):
    listener_thread = threading.Thread(target=self.Listen)
    listener_thread.setDaemon(True)
    listener_thread.start()
    time.sleep(1)

  def Stop(self):
    self.server.Stop()
