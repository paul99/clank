// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('options', function() {
  const ListItem = cr.ui.ListItem;
  const Grid = cr.ui.Grid;
  const ListSingleSelectionModel = cr.ui.ListSingleSelectionModel;

  /**
   * Creates a new profile icon grid item.
   * @param {Object} iconURL The profile icon URL.
   * @constructor
   * @extends {cr.ui.GridItem}
   */
  function ProfilesIconGridItem(iconURL) {
    var el = cr.doc.createElement('span');
    el.iconURL_ = iconURL;
    ProfilesIconGridItem.decorate(el);
    return el;
  }

  /**
   * Decorates an element as a profile grid item.
   * @param {!HTMLElement} el The element to decorate.
   */
  ProfilesIconGridItem.decorate = function(el) {
    el.__proto__ = ProfilesIconGridItem.prototype;
    el.decorate();
  };

  ProfilesIconGridItem.prototype = {
    __proto__: ListItem.prototype,

    /** @inheritDoc */
    decorate: function() {
      ListItem.prototype.decorate.call(this);
      var imageEl = cr.doc.createElement('img');
      imageEl.className = 'profile-icon';
      imageEl.src = this.iconURL_;
      this.appendChild(imageEl);

      this.className = 'profile-icon-grid-item';
    },
  };

  var ProfilesIconGrid = cr.ui.define('grid');

  ProfilesIconGrid.prototype = {
    __proto__: Grid.prototype,

    /** @inheritDoc */
    decorate: function() {
      Grid.prototype.decorate.call(this);
      this.selectionModel = new ListSingleSelectionModel();
    },

    /** @inheritDoc */
    createItem: function(iconURL) {
      return new ProfilesIconGridItem(iconURL);
    },
  };

  return {
    ProfilesIconGrid: ProfilesIconGrid
  };
});

