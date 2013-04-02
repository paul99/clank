# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import unittest

from telemetry import page

class TestPage(unittest.TestCase):
  def testGetUrlBaseDirAndFileForAbsolutePath(self):
    apage = page.Page('file:///somedir/otherdir/file.html',
                      None, # In this test, we don't need a page set.
                      base_dir='basedir')
    dirname, filename = apage.url_base_dir_and_file
    self.assertEqual(dirname, 'basedir/somedir/otherdir')
    self.assertEqual(filename, 'file.html')

  def testGetUrlBaseDirAndFileForRelativePath(self):
    apage = page.Page('file:///../../otherdir/file.html',
                      None, # In this test, we don't need a page set.
                      base_dir='basedir')
    dirname, filename = apage.url_base_dir_and_file
    self.assertEqual(dirname, 'basedir/../../otherdir')
    self.assertEqual(filename, 'file.html')

  def testGetUrlBaseDirAndFileForUrlBaseDir(self):
    apage = page.Page('file:///../../somedir/otherdir/file.html',
                      None, # In this test, we don't need a page set.
                      base_dir='basedir')
    setattr(apage, 'url_base_dir', 'file:///../../somedir/')
    dirname, filename = apage.url_base_dir_and_file
    self.assertEqual(dirname, 'basedir/../../somedir/')
    self.assertEqual(filename, 'otherdir/file.html')

  def testDisplayUrlForHttp(self):
    self.assertEquals(page.Page('http://www.foo.com/', None).display_url,
                      'www.foo.com/')

  def testDisplayUrlForFile(self):
    self.assertEquals(
        page.Page('file:///../../otherdir/file.html', None).display_url,
        'file.html')
