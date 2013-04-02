# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os

from telemetry.page import Page
from telemetry import scrolling_action
from telemetry import tab_test_case

class ScrollingActionTest(tab_test_case.TabTestCase):
  def CreateAndNavigateToPageFromUnittestDataDir(
    self, filename, page_attributes):
    unittest_data_dir = os.path.join(os.path.dirname(__file__),
                                     '..', 'unittest_data')
    self._browser.SetHTTPServerDirectory(unittest_data_dir)
    page = Page(
      self._browser.http_server.UrlOf(filename),
      None, # In this test, we don't need a page set.
      attributes=page_attributes)

    self._tab.Navigate(page.url)
    self._tab.WaitForDocumentReadyStateToBeComplete()

    return page

  def testScrollingAction(self):
    page = self.CreateAndNavigateToPageFromUnittestDataDir(
        "blank.html",
        page_attributes={"smoothness": {
          "action": "scrolling_action"
          }})
    # Make page bigger than window so it's scrollable.
    self._tab.ExecuteJavaScript("""document.body.style.height =
                              (2 * window.innerHeight + 1) + 'px';""")

    self.assertEquals(
        self._tab.EvaluateJavaScript('document.body.scrollTop'), 0)

    i = scrolling_action.ScrollingAction()
    i.WillRunAction(page, self._tab)

    self._tab.ExecuteJavaScript("""
        window.__scrollingAction.beginMeasuringHook = function() {
            window.__didBeginMeasuring = true;
        };
        window.__scrollingAction.endMeasuringHook = function() {
            window.__didEndMeasuring = true;
        };""")
    i.RunAction(page, self._tab)

    self.assertTrue(self._tab.EvaluateJavaScript('window.__didBeginMeasuring'))
    self.assertTrue(self._tab.EvaluateJavaScript('window.__didEndMeasuring'))

    # Allow for roundoff error in scaled viewport.
    scroll_position = self._tab.EvaluateJavaScript(
        'document.body.scrollTop + window.innerHeight')
    scroll_height = self._tab.EvaluateJavaScript('document.body.scrollHeight')
    difference = scroll_position - scroll_height
    self.assertTrue(abs(difference) <= 1)

  def testBoundingClientRect(self):
    self.CreateAndNavigateToPageFromUnittestDataDir('blank.html', {})
    with open(
      os.path.join(os.path.dirname(__file__),
                   'scrolling_action.js')) as f:
      js = f.read()
      self._tab.ExecuteJavaScript(js)

    # Verify that the rect returned by getBoundingVisibleRect() in
    # scroll.js is completely contained within the viewport. Scroll
    # events dispatched by the benchmarks use the center of this rect
    # as their location, and this location needs to be within the
    # viewport bounds to correctly decide between main-thread and
    # impl-thread scrolling. If the scrollable area were not clipped
    # to the viewport bounds, then the instance used here (the scrollable
    # area being more than twice as tall as the viewport) would
    # result in a scroll location outside of the viewport bounds.
    self._tab.ExecuteJavaScript("""document.body.style.height =
                           (2 * window.innerHeight + 1) + 'px';""")

    rect_bottom = int(self._tab.EvaluateJavaScript("""
        __ScrollingAction_GetBoundingVisibleRect(document.body).top +
        __ScrollingAction_GetBoundingVisibleRect(document.body).height"""))
    rect_right = int(self._tab.EvaluateJavaScript("""
        __ScrollingAction_GetBoundingVisibleRect(document.body).left +
        __ScrollingAction_GetBoundingVisibleRect(document.body).width"""))
    viewport_width = int(self._tab.EvaluateJavaScript('window.innerWidth'))
    viewport_height = int(self._tab.EvaluateJavaScript('window.innerHeight'))

    self.assertTrue(rect_bottom <= viewport_height)
    self.assertTrue(rect_right <= viewport_width)
