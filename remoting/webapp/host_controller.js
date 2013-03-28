// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/** @constructor */
remoting.HostController = function() {
  /** @type {remoting.HostPlugin} @private */
  this.plugin_ = remoting.HostSession.createPlugin();
  /** @type {HTMLElement} @private */
  this.container_ = document.getElementById('daemon-plugin-container');
  this.container_.appendChild(this.plugin_);
  /** @type {string?} */
  this.localHostId_ = null;
  /** @param {string} version */
  var printVersion = function(version) {
    if (version == '') {
      console.log('Host not installed.');
    } else {
      console.log('Host version: ' + version);
    }
  };
  /** @type {boolean} @private */
  this.pluginSupported_ = true;
  try {
    this.plugin_.getDaemonVersion(printVersion);
  } catch (err) {
    console.log('Host version not available.');
    this.pluginSupported_ = false;
  }
};

// Note that the values in the enums below are copied from
// daemon_controller.h and must be kept in sync.
/** @enum {number} */
remoting.HostController.State = {
  NOT_IMPLEMENTED: -1,
  NOT_INSTALLED: 0,
  INSTALLING: 1,
  STOPPED: 2,
  STARTING: 3,
  STARTED: 4,
  STOPPING: 5,
  UNKNOWN: 6
};

/** @enum {number} */
remoting.HostController.AsyncResult = {
  OK: 0,
  FAILED: 1,
  CANCELLED: 2,
  FAILED_DIRECTORY: 3
};

/**
 * Checks if the host is installed on the local computer.
 *
 * TODO(sergeyu): Make this method asynchronous or just remove it and use
 * getLocalHostStateAndId() instead.
 *
 * @return {boolean} True if the host is installed.
 */
remoting.HostController.prototype.isInstalled = function() {
  var state = this.plugin_.daemonState;
  return typeof(state) != 'undefined' &&
    state != remoting.HostController.State.NOT_INSTALLED &&
    state != remoting.HostController.State.INSTALLING;
}

/**
 * Checks whether or not the host plugin is valid.
 *
 * @return {boolean} True if the plugin is supported and loaded; false
 *     otherwise.
 */
remoting.HostController.prototype.isPluginSupported = function() {
  return this.pluginSupported_;
};

/**
 * @param {function(boolean, boolean, boolean):void} callback Callback to be
 *     called when done.
 */
remoting.HostController.prototype.getConsent = function(callback) {
  this.plugin_.getUsageStatsConsent(callback);
};

/**
 * Registers and starts the host.
 * @param {string} hostPin Host PIN.
 * @param {boolean} consent The user's consent to crash dump reporting.
 * @param {function(remoting.HostController.AsyncResult):void} callback
 * callback Callback to be called when done.
 * @return {void} Nothing.
 */
remoting.HostController.prototype.start = function(hostPin, consent, callback) {
  /** @type {remoting.HostController} */
  var that = this;
  var hostName = this.plugin_.getHostName();

  /** @return {string} */
  function generateUuid() {
    var random = new Uint16Array(8);
    window.crypto.getRandomValues(random);
    /** @type {Array.<string>} */
    var e = new Array();
    for (var i = 0; i < 8; i++) {
      e[i] = (/** @type {number} */random[i] + 0x10000).
          toString(16).substring(1);
    }
    return e[0] + e[1] + '-' + e[2] + "-" + e[3] + '-' +
        e[4] + '-' + e[5] + e[6] + e[7];
  };

  var newHostId = generateUuid();

  /** @param {function(remoting.HostController.AsyncResult):void} callback
   *  @param {remoting.HostController.AsyncResult} result
   *  @param {string} hostName
   *  @param {string} publicKey */
  function onStarted(callback, result, hostName, publicKey) {
    if (result == remoting.HostController.AsyncResult.OK) {
      that.localHostId_ = newHostId;
      remoting.hostList.onLocalHostStarted(hostName, newHostId, publicKey);
    } else {
      that.localHostId_ = null;
      // Unregister the host if we failed to start it.
      remoting.HostList.unregisterHostById(newHostId);
    }
    callback(result);
  };

  /** @param {string} publicKey
   *  @param {string} privateKey
   *  @param {XMLHttpRequest} xhr */
  function onRegistered(publicKey, privateKey, xhr) {
    var success = (xhr.status == 200);

    if (success) {
      var hostSecretHash =
          that.plugin_.getPinHash(newHostId, hostPin);
      var hostConfig = JSON.stringify({
          xmpp_login: remoting.oauth2.getCachedEmail(),
          oauth_refresh_token: remoting.oauth2.exportRefreshToken(),
          host_id: newHostId,
          host_name: hostName,
          host_secret_hash: hostSecretHash,
          private_key: privateKey
      });
      /** @param {remoting.HostController.AsyncResult} result */
      var onStartDaemon = function(result) {
        onStarted(callback, result, hostName, publicKey);
      };
      that.plugin_.startDaemon(hostConfig, consent, onStartDaemon);
    } else {
      console.log('Failed to register the host. Status: ' + xhr.status +
                  ' response: ' + xhr.responseText);
      callback(remoting.HostController.AsyncResult.FAILED_DIRECTORY);
    }
  };

  /**
   * @param {string} privateKey
   * @param {string} publicKey
   * @param {string} oauthToken
   */
  function doRegisterHost(privateKey, publicKey, oauthToken) {
    var headers = {
      'Authorization': 'OAuth ' + oauthToken,
      'Content-type' : 'application/json; charset=UTF-8'
    };

    var newHostDetails = { data: {
       hostId: newHostId,
       hostName: hostName,
       publicKey: publicKey
    } };
    remoting.xhr.post(
        'https://www.googleapis.com/chromoting/v1/@me/hosts/',
        /** @param {XMLHttpRequest} xhr */
        function (xhr) { onRegistered(publicKey, privateKey, xhr); },
        JSON.stringify(newHostDetails),
        headers);
  };

  /** @param {string} privateKey
   *  @param {string} publicKey */
  function onKeyGenerated(privateKey, publicKey) {
    remoting.oauth2.callWithToken(
        /** @param {string} oauthToken */
        function(oauthToken) {
          doRegisterHost(privateKey, publicKey, oauthToken);
        },
        /** @param {remoting.Error} error */
        function(error) {
          // TODO(jamiewalch): Have a more specific error code here?
          callback(remoting.HostController.AsyncResult.FAILED);
        });
  };

  this.plugin_.generateKeyPair(onKeyGenerated);
};

/**
 * Stop the daemon process.
 * @param {function(remoting.HostController.AsyncResult):void} callback
 *     Callback to be called when finished.
 * @return {void} Nothing.
 */
remoting.HostController.prototype.stop = function(callback) {
  /** @type {remoting.HostController} */
  var that = this;

  /** @param {remoting.HostController.AsyncResult} result */
  function onStopped(result) {
    if (result == remoting.HostController.AsyncResult.OK &&
        that.localHostId_) {
      remoting.HostList.unregisterHostById(that.localHostId_);
    }
    callback(result);
  };
  this.plugin_.stopDaemon(onStopped);
};

/**
 * Parse a stringified host configuration and return it as a dictionary if it
 * is well-formed and contains both host_id and xmpp_login keys.  null is
 * returned if either key is missing, or if the configuration is corrupt.
 * @param {string} configStr The host configuration, JSON encoded to a string.
 * @return {Object.<string,string>|null} The host configuration.
 */
function parseHostConfig_(configStr) {
  var config = /** @type {Object.<string,string>} */ jsonParseSafe(configStr);
  if (config &&
      typeof config['host_id'] == 'string' &&
      typeof config['xmpp_login'] == 'string') {
    return config;
  } else {
    // {} means that host is not configured; '' means that the config file could
    // not be read.
    // TODO(jamiewalch): '' is expected if the host isn't installed, but should
    // be reported as an error otherwise. Fix this once we have an event-based
    // daemon state mechanism.
    if (configStr != '{}' && configStr != '') {
      console.error('Invalid getDaemonConfig response.');
    }
  }
  return null;
}

/**
 * @param {string} newPin The new PIN to set
 * @param {function(remoting.HostController.AsyncResult):void} callback
 *     Callback to be called when finished.
 * @return {void} Nothing.
 */
remoting.HostController.prototype.updatePin = function(newPin, callback) {
  /** @type {remoting.HostController} */
  var that = this;

  /** @param {string} configStr */
  function onConfig(configStr) {
    var config = parseHostConfig_(configStr);
    if (!config) {
      callback(remoting.HostController.AsyncResult.FAILED);
      return;
    }
    var hostId = config['host_id'];
    var newConfig = JSON.stringify({
        host_secret_hash: that.plugin_.getPinHash(hostId, newPin)
      });
    that.plugin_.updateDaemonConfig(newConfig, callback);
  };

  // TODO(sergeyu): When crbug.com/121518 is fixed: replace this call
  // with an upriveleged version if that is necessary.
  this.plugin_.getDaemonConfig(onConfig);
};

/**
 * Update the internal state so that the local host can be correctly filtered
 * out of the host list.
 *
 * @param {function(remoting.HostController.State, string?):void} onDone
 *     Completion callback.
 */
remoting.HostController.prototype.getLocalHostStateAndId = function(onDone) {
  /** @type {remoting.HostController} */
  var that = this;
  /** @param {string} configStr */
  function onConfig(configStr) {
    var config = parseHostConfig_(configStr);
    if (config) {
      that.localHostId_ = config['host_id'];
    } else {
      that.localHostId_ = null;
    }

    var state = that.plugin_.daemonState;
    if (typeof(state) == 'undefined') {
      // If the plug-in can't be instantiated, for example on ChromeOS, then
      // return something sensible.
      state = remoting.HostController.State.NOT_IMPLEMENTED;
    }

    onDone(state, that.localHostId_);
  };
  try {
    this.plugin_.getDaemonConfig(onConfig);
  } catch (err) {
    onDone(remoting.HostController.State.NOT_IMPLEMENTED, null);
  }
};

/** @type {remoting.HostController} */
remoting.hostController = null;
