// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains various hacks needed to inform JSCompiler of various
// WebKit- and Chrome-specific properties and methods. It is used only with
// JSCompiler to verify the type-correctness of our code.

/** @type {HTMLElement} */
Document.prototype.activeElement;

/** @type {Array.<HTMLElement>} */
Document.prototype.all;

/** @type {function(string): void} */
Document.prototype.execCommand = function(command) {};

/** @return {void} Nothing. */
Document.prototype.webkitCancelFullScreen = function() {};

/** @type {boolean} */
Document.prototype.webkitIsFullScreen;

/** @type {boolean} */
Document.prototype.webkitHidden;

/** @type {number} */
Element.ALLOW_KEYBOARD_INPUT;

/** @param {number} flags
/** @return {void} Nothing. */
Element.prototype.webkitRequestFullScreen = function(flags) {};

/** @type {{getRandomValues: function((Uint16Array|Uint8Array)):void}} */
Window.prototype.crypto;

/** @constructor
    @extends {HTMLElement} */
var HTMLEmbedElement = function() { };

/** @type {number} */
HTMLEmbedElement.prototype.height;

/** @type {number} */
HTMLEmbedElement.prototype.width;

/** @type {Window} */
HTMLIFrameElement.prototype.contentWindow;

/** @constructor */
var MutationRecord = function() {};

/** @type {string} */
MutationRecord.prototype.attributeName;

/** @type {Element} */
MutationRecord.prototype.target;

/** @type {string} */
MutationRecord.prototype.type;

/** @constructor
    @param {function(Array.<MutationRecord>):void} callback */
var WebKitMutationObserver = function(callback) {};

/** @param {Element} element
    @param {Object} options */
WebKitMutationObserver.prototype.observe = function(element, options) {};

// TODO(jamiewalch): Flesh this out with the correct type when we're a v2 app.
/** @type {remoting.MockStorage} */
remoting.storage.local = null;

/** @type {Object} */
chrome.app.runtime = {
  /** @type {Object} */
  onLaunched: {
    /** @param {function():void} callback */
    addListener: function(callback) {}
  }
};

/** @type {Object} */
chrome.app.window = {
  /**
   * @param {string} name
   * @param {Object} parameters
   */
  create: function(name, parameters) {}
};

/** @type {Object} */
chrome.experimental = {};

/** @type {Object} */
chrome.experimental.identity = {
  /**
   * @param {Object.<string>} parameters
   * @param {function(string):void} callback
   */
  getAuthToken: function(parameters, callback) {}
};

/** @type {Object} */
chrome.runtime = {
  /** @type {Object} */
  lastError: {
    /** @type {string} */
    message: ''
  }
};

/** @type {Object} */
chrome.tabs;

/** @param {function(chrome.Tab):void} callback */
chrome.tabs.getCurrent = function(callback) {}

/** @constructor */
chrome.Tab = function() {
  /** @type {boolean} */
  this.pinned = false;
  /** @type {number} */
  this.windowId = 0;
};

/** @type {Object} */
chrome.windows;

/** @param {number} id
 *  @param {Object?} getInfo
 *  @param {function(chrome.Window):void} callback */
chrome.windows.get = function(id, getInfo, callback) {}

/** @constructor */
chrome.Window = function() {
  /** @type {string} */
  this.state = '';
  /** @type {string} */
  this.type = '';
};
