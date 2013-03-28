# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import sys

# Get chrome/test/functional scripts into our path.
# TODO(tonyg): Move webpagereplay.py to a common location.
sys.path.append(
    os.path.abspath(
        os.path.join(os.path.dirname(__file__),
                     '../../../chrome/test/functional')))
import webpagereplay  # pylint: disable=F0401

CHROME_FLAGS = webpagereplay.CHROME_FLAGS

class ReplayServer(object):
  def __init__(self, browser_backend, path, is_record_mode):
    self._browser_backend = browser_backend
    self._forwarder = None
    self._web_page_replay = None
    self._is_record_mode = is_record_mode

    # Note: This can cause flake if server doesn't shut down properly and keeps
    # ports tied up. See crbug.com/157459.
    self._forwarder = browser_backend.CreateForwarder(
        (webpagereplay.HTTP_PORT, webpagereplay.HTTP_PORT),
        (webpagereplay.HTTPS_PORT, webpagereplay.HTTPS_PORT))

    options = []
    if self._is_record_mode:
      options.append('--record')
    if not browser_backend.options.wpr_make_javascript_deterministic:
      options.append('--inject_scripts=')
    self._web_page_replay = webpagereplay.ReplayServer(path, options)
    self._web_page_replay.StartServer()

  def __enter__(self):
    return self

  def __exit__(self, *args):
    self.Close()

  def Close(self):
    if self._forwarder:
      self._forwarder.Close()
      self._forwarder = None
    if self._web_page_replay:
      self._web_page_replay.StopServer()
      self._web_page_replay = None
