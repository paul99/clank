// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/menu/menu_config.h"

#include "grit/ui_resources.h"
#include "ui/base/layout.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/views/controls/menu/menu_image_util.h"

namespace views {

#if !defined(OS_WIN)
void MenuConfig::Init(const ui::NativeTheme* theme) {
  InitAura();
}
#endif

void MenuConfig::InitAura() {
  ui::NativeTheme* theme = ui::NativeThemeAura::instance();
  text_color = theme->GetSystemColor(
      ui::NativeTheme::kColorId_EnabledMenuItemForegroundColor);
  menu_horizontal_border_size = 0;
  menu_vertical_border_size = 0;
  submenu_horizontal_inset = 1;
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  arrow_to_edge_padding = 20;
  icon_to_label_padding = 4;
  arrow_width = rb.GetImageNamed(IDR_MENU_ARROW).ToImageSkia()->width();
  const gfx::ImageSkia* check = GetMenuCheckImage();
  // Add 4 to force some padding between check and label.
  check_width = check->width() + 4;
  check_height = check->height();
  item_left_margin = 4;
  item_min_height = 29;
  separator_height = 15;
  separator_spacing_height = 7;
  separator_lower_height = 8;
  separator_upper_height = 8;
  font = rb.GetFont(ResourceBundle::BaseFont);
  label_to_arrow_padding = 20;
  label_to_accelerator_padding = 20;
  always_use_icon_to_label_padding = true;
  align_arrow_and_shortcut = true;
  offset_context_menus = true;
}

#if !defined(OS_WIN)
// static
const MenuConfig& MenuConfig::instance(const ui::NativeTheme* theme) {
  static MenuConfig* views_instance = NULL;
  if (!views_instance)
    views_instance = new MenuConfig(ui::NativeTheme::instance());
  return *views_instance;
}
#endif

}  // namespace views
