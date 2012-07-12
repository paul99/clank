// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Create the bubble notch arrow for the description bubbles of the
// footer buttons.
(function() {
  var arrowWidth = 8;
  var arrowHeight = 4;
  var ctx = document.getCSSCanvasContext(
      '2d', 'footer-bubble-arrow', arrowWidth, arrowHeight);
  ctx.fillStyle = '#EBEBEB';
  ctx.strokeStyle = '#EBEBEB';
  ctx.beginPath();
  ctx.moveTo(0, 0);
  ctx.lineTo(arrowWidth / 2, arrowHeight);
  ctx.lineTo(arrowWidth, 0);
  ctx.closePath();
  ctx.fill();
  ctx.stroke();

  ctx.strokeStyle = '#A4A4A4';
  ctx.beginPath();
  ctx.moveTo(0, 0);
  ctx.lineTo(arrowWidth / 2, arrowHeight);
  ctx.lineTo(arrowWidth, 0);
  ctx.stroke();
})();

var MIN_SECTION_PADDING = 10;
var MAX_SECTION_LINE_WIDTH = 90;

/**
 * @param {number} width The width of the navigation overlay.
 * @param {number} height The height of the navigation overlay.
 */
function initNavigationOverlay(width, height) {
  var ctx = document.getCSSCanvasContext(
      '2d', 'navigation-clip', width, height);
  ctx.fillStyle = '#AAA';
  ctx.font = '10pt Droid Sans, sans serif';
  ctx.textBaseline = 'top';
  ctx.textAlign = 'center';

  var sectionsText = ['Most Visited', 'Bookmarks', 'Open Tabs'];

  var cellWidth = width / sectionsText.length;
  var sectionLineWidth = Math.min(
      MAX_SECTION_LINE_WIDTH, cellWidth - (2 * MIN_SECTION_PADDING));
  var sectionPadding = (cellWidth - sectionLineWidth) / 2;

  var iconsOnly = false;
  for (var i = 0; i < sectionsText.length; i++) {
    if (ctx.measureText(sectionsText[i]).width > sectionLineWidth) {
      iconsOnly = true;
      break;
    }
  }

  for (var i = 0; i < sectionsText.length; i++) {
    ctx.fillRect(sectionPadding + i * cellWidth, 10, sectionLineWidth, 5);
    if (iconsOnly) {
      // TODO(tedchoc): Implement the icons.
    } else {
      ctx.fillText(sectionsText[i], cellWidth * i + cellWidth / 2, 20);
    }
  }
}
