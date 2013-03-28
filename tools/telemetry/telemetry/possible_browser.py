# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
class PossibleBrowser(object):
  """A browser that can be controlled.

  Call Create() to launch the browser and begin manipulating it..
  """

  def __init__(self, browser_type, options):
    self.browser_type = browser_type
    self._options = options

  def __repr__(self):
    return 'PossibleBrowser(browser_type=%s)' % self.browser_type

  @property
  def options(self):
    return self._options

  def Create(self):
    raise NotImplementedError()
