# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os

from telemetry import discover
from telemetry import page_action

_page_action_classes = discover.Discover(os.path.dirname(__file__),
                                         'action',
                                         page_action.PageAction,
                                         import_error_should_raise=True)

def GetAllClasses():
  return list(_page_action_classes.values())

def FindClassWithName(name):
  return _page_action_classes.get(name)

def RegisterClassForTest(name, clazz):
  _page_action_classes[name] = clazz
