// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Loads a thumbnail using provided url. In CANVAS mode, loaded images
 * are attached as <canvas> element, while in IMAGE mode as <img>.
 * <canvas> renders faster than <img>, however has bigger memory overhead.
 *
 * @param {string} url File URL.
 * @param {ThumbnailLoader.LoaderType} opt_loaderType Canvas or Image loader,
 *     default: IMAGE.
 * @param {Object} opt_metadata Metadata object.
 * @param {string} opt_mediaType Media type.
 * @constructor
 */
function ThumbnailLoader(url, opt_loaderType, opt_metadata, opt_mediaType) {
  this.mediaType_ = opt_mediaType || FileType.getMediaType(url);
  this.loaderType_ = opt_loaderType || ThumbnailLoader.LoaderType.IMAGE;

  if (!opt_metadata) {
    this.thumbnailUrl_ = url;  // Use the URL directly.
    return;
  }

  this.fallbackUrl_ = null;
  this.thumbnailUrl_ = null;
  if (opt_metadata.drive) {
    var apps = opt_metadata.drive.driveApps;
    for (var i = 0; i < apps.length; ++i) {
      if (apps[i].docIcon && apps[i].isPrimary) {
        this.fallbackUrl_ = apps[i].docIcon;
        break;
      }
    }
  }

  if (opt_metadata.thumbnail && opt_metadata.thumbnail.url) {
    this.thumbnailUrl_ = opt_metadata.thumbnail.url;
    this.transform_ = opt_metadata.thumbnail.transform;
  } else if (FileType.isImage(url) &&
      ThumbnailLoader.canUseImageUrl_(opt_metadata)) {
    this.thumbnailUrl_ = url;
    this.transform_ = opt_metadata.media && opt_metadata.media.imageTransform;
  } else if (this.fallbackUrl_) {
    // Use fallback as the primary thumbnail.
    this.thumbnailUrl_ = this.fallbackUrl_;
    this.fallbackUrl_ = null;
  } // else the generic thumbnail based on the media type will be used.
}

/**
 * Files with more pixels won't have thumbnails.
 */
ThumbnailLoader.MAX_PIXEL_COUNT = 1 << 21; // 2 MPix

/**
 * Files of bigger size won't have thumbnails.
 */
ThumbnailLoader.MAX_FILE_SIZE = 1 << 20; // 1 Mb

/**
 * In percents (0.0 - 1.0), how much area can be cropped to fill an image
 * in a container, when loading a thumbnail in FillMode.AUTO mode.
 * The specified 30% value allows to fill 16:9, 3:2 pictures in 4:3 element.
 * @type {number}
 */
ThumbnailLoader.AUTO_FILL_THRESHOLD = 0.3;

/**
 * Type of displaying a thumbnail within a box.
 * @enum
 */
ThumbnailLoader.FillMode = {
  FILL: 0,  // Fill whole box. Image may be cropped.
  FIT: 1,   // Keep aspect ratio, do not crop.
  AUTO: 2   // Try to fill, but if incompatible aspect ratio, then fit.
};

/**
 * Type of element to store the image.
 * @enum
 */
ThumbnailLoader.LoaderType = {
  IMAGE: 0,
  CANVAS: 1
};

/**
 * If an image file does not have an embedded thumbnail we might want to use
 * the image itself as a thumbnail. If the image is too large it hurts
 * the performance a lot so we allow it only for moderately sized files.
 *
 * @param {Object} metadata Metadata object
 * @return {boolean} Whether it is OK to use the image url for a preview.
 * @private
 */
ThumbnailLoader.canUseImageUrl_ = function(metadata) {
  return (metadata.filesystem && metadata.filesystem.size &&
      metadata.filesystem.size <= ThumbnailLoader.MAX_FILE_SIZE) ||
     (metadata.media && metadata.media.width && metadata.media.height &&
      metadata.media.width * metadata.media.height <=
          ThumbnailLoader.MAX_PIXEL_COUNT);
};

/**
 *
 * @param {HTMLElement} box Container element.
 * @param {ThumbnailLoader.FillMode} fillMode Fill mode.
 * @param {function(Image, object} opt_onSuccess Success callback,
 *   accepts the image and the transform.
 * @param {function} opt_onError Error callback.
 * @param {function} opt_onGeneric Callback for generic image used.
 */
ThumbnailLoader.prototype.load = function(
    box, fillMode, opt_onSuccess, opt_onError, opt_onGeneric) {
  if (!this.thumbnailUrl_) {
    // Relevant CSS rules are in file_types.css.
    box.setAttribute('generic-thumbnail', this.mediaType_);
    if (opt_onGeneric) opt_onGeneric();
    return;
  }

  this.canvasUpToDate_ = false;
  this.image_ = new Image();
  this.image_.onload = function() {
    this.attachImage(box, fillMode);
    if (opt_onSuccess)
      opt_onSuccess(this.image_, this.transform_);
  }.bind(this);
  this.image_.onerror = function() {
    if (opt_onError)
      opt_onError();
    if (this.fallbackUrl_) {
      new ThumbnailLoader(this.fallbackUrl_,
                          this.loaderType_,
                          null,
                          this.mediaType_).
          load(box, fillMode, opt_onSuccess);
    } else {
      box.setAttribute('generic-thumbnail', this.mediaType_);
    }
  }.bind(this);

  if (this.image_.src == this.thumbnailUrl_) {
    console.warn('Thumnbnail already loaded: ' + this.thumbnailUrl_);
    return;
  }

  util.loadImage(this.image_, this.thumbnailUrl_);
};

/**
 * @return {boolean} True if a valid image is loaded.
 */
ThumbnailLoader.prototype.hasValidImage = function() {
  return !!(this.image_ && this.image_.width && this.image_.height);
};

/**
 * @return {boolean} True if the image is rotated 90 degrees left or right.
 * @private
 */
ThumbnailLoader.prototype.isRotated_ = function() {
  return this.transform_ && (this.transform_.rotate90 % 2 == 1);
};

/**
 * @return {number} Image width (corrected for rotation).
 */
ThumbnailLoader.prototype.getWidth = function() {
  return this.isRotated_() ? this.image_.height : this.image_.width;
};

/**
 * @return {number} Image height (corrected for rotation).
 */
ThumbnailLoader.prototype.getHeight = function() {
  return this.isRotated_() ? this.image_.width : this.image_.height;
};

/**
 * Load an image but do not attach it.
 *
 * @param {function(boolean)} callback Callback, parameter is true if the image
 *   has loaded successfully or a stock icon has been used.
 */
ThumbnailLoader.prototype.loadDetachedImage = function(callback) {
  if (!this.thumbnailUrl_) {
    callback(true);
    return;
  }

  this.canvasUpToDate_ = false;
  this.image_ = new Image();
  this.image_.onload = callback.bind(null, true);
  this.image_.onerror = callback.bind(null, false);
  util.loadImage(this.image_, this.thumbnailUrl_);
};

/**
 * Attach the image to a given element.
 * @param {Element} container Parent element.
 * @param {ThumbnailLoader.FillMode} fillMode Fill mode.
 */
ThumbnailLoader.prototype.attachImage = function(container, fillMode) {
  if (!this.hasValidImage()) {
    container.setAttribute('generic-thumbnail', this.mediaType_);
    return;
  }

  var attachableMedia;
  if (this.loaderType_ == ThumbnailLoader.LoaderType.CANVAS) {
    if (!this.canvas_)
      this.canvas_ = container.ownerDocument.createElement('canvas');

    // Copy the image to a canvas if the canvas is outdated.
    if (!this.canvasUpToDate_) {
      this.canvas_.width = this.image_.width;
      this.canvas_.height = this.image_.height;
      var context = this.canvas_.getContext('2d');
      context.drawImage(this.image_, 0, 0);
      this.canvasUpToDate_ = true;
    }

    // Canvas will be attached.
    attachableMedia = this.canvas_;
  } else {
    // Image will be attached.
    attachableMedia = this.image_;
  }

  util.applyTransform(container, this.transform_);
  ThumbnailLoader.centerImage_(
      container, attachableMedia, fillMode, this.isRotated_());
  if (this.image_.parentNode != container) {
    container.textContent = '';
    container.appendChild(attachableMedia);
  }
};

/**
 * Update the image style to fit/fill the container.
 *
 * Using webkit center packing does not align the image properly, so we need
 * to wait until the image loads and its dimensions are known, then manually
 * position it at the center.
 *
 * @param {HTMLElement} box Containing element.
 * @param {Image|HTMLCanvasElement} img Element containing an image.
 * @param {ThumbnailLoader.FillMode} fillMode Fill mode.
 * @param {boolean} rotate True if the image should be rotated 90 degrees.
 * @private
 */
ThumbnailLoader.centerImage_ = function(box, img, fillMode, rotate) {
  var imageWidth = img.width;
  var imageHeight = img.height;

  var fractionX;
  var fractionY;

  var boxWidth = box.clientWidth;
  var boxHeight = box.clientHeight;

  var fill;
  switch (fillMode) {
    case ThumbnailLoader.FillMode.FILL:
      fill = true;
      break;
    case ThumbnailLoader.FillMode.FIT:
      fill = false;
      break;
    case ThumbnailLoader.FillMode.AUTO:
      var imageRatio = imageWidth / imageHeight;
      var boxRatio = 1.0;
      if (boxWidth && boxHeight)
        boxRatio = boxWidth / boxHeight;
      // Cropped area in percents.
      var ratioFactor = boxRatio / imageRatio;
      fill = (ratioFactor >= 1.0 - ThumbnailLoader.AUTO_FILL_THRESHOLD) &&
             (ratioFactor <= 1.0 + ThumbnailLoader.AUTO_FILL_THRESHOLD);
      break;
  }

  if (boxWidth && boxHeight) {
    // When we know the box size we can position the image correctly even
    // in a non-square box.
    var fitScaleX = (rotate ? boxHeight : boxWidth) / imageWidth;
    var fitScaleY = (rotate ? boxWidth : boxHeight) / imageHeight;

    var scale = fill ?
        Math.max(fitScaleX, fitScaleY) :
        Math.min(fitScaleX, fitScaleY);

    scale = Math.min(scale, 1);  // Never overscale.

    fractionX = imageWidth * scale / boxWidth;
    fractionY = imageHeight * scale / boxHeight;
  } else {
    // We do not know the box size so we assume it is square.
    // Compute the image position based only on the image dimensions.
    // First try vertical fit or horizontal fill.
    fractionX = imageWidth / imageHeight;
    fractionY = 1;
    if ((fractionX < 1) == !!fill) {  // Vertical fill or horizontal fit.
      fractionY = 1 / fractionX;
      fractionX = 1;
    }
  }

  function percent(fraction) {
    return (fraction * 100).toFixed(2) + '%';
  }

  img.style.width = percent(fractionX);
  img.style.height = percent(fractionY);
  img.style.left = percent((1 - fractionX) / 2);
  img.style.top = percent((1 - fractionY) / 2);
};
