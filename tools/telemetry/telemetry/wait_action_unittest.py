# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import time

from telemetry import wait_action
from telemetry import tab_test_case

class WaitActionTest(tab_test_case.TabTestCase):
  def testWaitAction(self):
    unittest_data_dir = os.path.join(os.path.dirname(__file__),
                                     '..', 'unittest_data')
    self._browser.SetHTTPServerDirectory(unittest_data_dir)
    self._tab.Navigate(
      self._browser.http_server.UrlOf('blank.html'))
    self._tab.WaitForDocumentReadyStateToBeComplete()
    self.assertEquals(
        self._tab.EvaluateJavaScript('document.location.pathname;'),
        '/blank.html')

    i = wait_action.WaitAction({ 'duration' : 1 })

    start_time = time.time()
    i.RunAction({}, self._tab)
    self.assertAlmostEqual(time.time() - start_time, 1, places=2)
