#!/usr/bin/python2.4
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Produces localized strings.xml files for android.

In cases where a "android" type output file is requested in a grd, a
android_xml.MessagesFormatter will process the <message> tags and produce a
valid strings.xml that is properly localized with the specified language.

For example if the follwing output tag were to be included in a grd file
<outputs>
  ...
  <output filename="values-es/strings.xml" type="android" lang="es" />
  ...
</outputs>

For a grd file with the following messages:

  <message name="IDS_HELLO" desc="Simple greeting">Hello</message>
  <message name="IDS_WORLD" desc="The world">world</message>

And there existed an appropriate xtb file containing the Spanish translations.
Then the MessagesFormatter would produce:

<?xml version="1.0" encoding="utf-8"?>
<resources xmlns:android="http://schemas.android.com/apk/res/android"
           xmlns:xliff="urn:oasis:names:tc:xliff:document:1.2">
  <string name='hello'>Hola</string>
  <string name='world'>mundo</string>
</resources>

which would be written to values-es/strings.xml and usable by the android
resource framework.
"""

import os
import re
import struct
import types
import xml.sax.saxutils

from grit import util
from grit.format import interface
from grit.format import data_pack
from grit.node import empty
from grit.node import include
from grit.node import message
from grit.node import misc

class MessagesFormatter(interface.ItemFormatter):
  """
  """

  # The android resource name and optional product information are put placed
  # in the grd string name because grd doesn't know about android product
  # information.
  __PATTERN = re.compile(
      '^IDS_(?P<name>[A-Z0-9_]+)(_product_(?P<product>[a-z]*))?$')

  __FORMAT = u"""
<?xml version="1.0" encoding="utf-8"?>
<resources xmlns:android="http://schemas.android.com/apk/res/android"
           xmlns:xliff="urn:oasis:names:tc:xliff:document:1.2">
%s
</resources>
"""

  def Format(self, item, lang='en', begin_item=True, output_dir='.'):
    """Formats the messages into a format useable by Android.

    Overrides node.ItemFormatter.Format. The specified empty.Messages node will
    be formatted in to a strings.xml resource xml string. Note that this script
    assumes that string resource names are lower cased. Also it is assumed that
    the grd file in question was produced using the android2grd tool. Thus not
    all grd tags are supported or comprehensible to this formatter. For
    example, the printf style syntax allowed in grd messages is not resolved.

    Args:
      item: A empty.MessageNode which is the parent of all messages to be
            formatted.
      lang: ISO code used to determine which translations to use. Defaults to
            English.
      begin_item: Ignored. See documentation from node.ItemFormatter.Format.
      output_dir: Ignored.
    Returns
      A string containing the content of the localized android resource file
      (strings.xml) file.
    """
    if not begin_item:
      return ''
    assert isinstance(item, empty.MessagesNode)
    # In most cases we only need a name attribute and string value.
    simple_template = u"<string name='%s'>%s</string>"
    # In a few cases a product attribute is needed.
    product_template = u"<string name='%s' product='%s'>%s</string>"
    nodes = data_pack.DataPack.GetDataNodes(item)
    strings = []
    for node in nodes:
      # Get the unescaped message text as a unicode string.
      value = node.GetDataPackPair(lang, message.BINARY)[1]

      # Replace < > & with &lt; &gt; &amp; to ensure we generate valid XML.
      # Replace ' " with \' \" to conform to Android's string formatting rules
      value = xml.sax.saxutils.escape(value, {"'": "\\'", '"': '\\"'})

      # Wrap the string in double quotes to preserve whitespace.
      value = '"' + value + '"'

      name = node.NameOrOffset()
      match = MessagesFormatter.__PATTERN.match(name)
      if match:
        name = match.group('name').lower()
        product = match.group('product')
        string_resource_el = '';
        if product:
          string_resource_el = product_template % (name, product, value)
        else:
          string_resource_el = simple_template % (name, value)
        strings.append(string_resource_el)
      else:
        raise Exception('Unexpected resource name: %s' % name )
    android_strings_xml = MessagesFormatter.__FORMAT % '\n'.join(strings)
    return android_strings_xml.strip()

