# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import telemetry
from telemetry import util

def _WaitForFormToLoad(form_id, tab):
  def IsFormLoaded():
    return tab.runtime.Evaluate(
        'document.querySelector("#%s")!== null' % form_id)

  # Wait until the form is submitted and the page completes loading.
  util.WaitFor(lambda: IsFormLoaded(), 60) # pylint: disable=W0108

def _SubmitFormAndWait(form_id, tab):
  js = 'document.getElementById("%s").submit();' % form_id
  tab.runtime.Execute(js)

  def IsLoginStillHappening():
    return tab.runtime.Evaluate(
        'document.querySelector("#%s")!== null' % form_id)

  # Wait until the form is submitted and the page completes loading.
  util.WaitFor(lambda: not IsLoginStillHappening(), 60)

class FormBasedCredentialsBackend(object):
  def __init__(self):
    self._logged_in = False

  @property
  def credentials_type(self):
    raise NotImplementedError()

  @property
  def url(self):
    raise NotImplementedError()

  @property
  def form_id(self):
    raise NotImplementedError()

  @property
  def login_input_id(self):
    raise NotImplementedError()

  @property
  def password_input_id(self):
    raise NotImplementedError()

  def LoginNeeded(self, tab, config):
    """Logs in to a test account.

    Raises:
      RuntimeError: if could not get credential information.
    """
    if self._logged_in:
      return True

    if 'username' not in config or 'password' not in config:
      message = ('Credentials for "%s" must include username and password.' %
                 self.credentials_type)
      raise RuntimeError(message)

    logging.debug('Logging into %s account...' % self.credentials_type)

    try:
      logging.info('Loading %s...', self.url)
      tab.page.Navigate(self.url)
      _WaitForFormToLoad(self.form_id, tab)
      tab.WaitForDocumentReadyStateToBeInteractiveOrBetter()
      logging.info('Loaded page: %s', self.url)

      email_id = 'document.getElementById("%s").value = "%s"; ' % (
          self.login_input_id, config['username'])
      password = 'document.getElementById("%s").value = "%s"; ' % (
          self.password_input_id, config['password'])
      tab.runtime.Execute(email_id)
      tab.runtime.Execute(password)

      _SubmitFormAndWait(self.form_id, tab)

      self._logged_in = True
      return True
    except telemetry.TimeoutException:
      logging.warning('Timed out while loading: %s', self.url)
      return False

  def LoginNoLongerNeeded(self, tab): # pylint: disable=W0613
    assert self._logged_in
