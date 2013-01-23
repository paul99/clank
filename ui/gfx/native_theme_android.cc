// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/native_theme_android.h"

#include <limits>

#include "base/basictypes.h"
#include "base/logging.h"
#include "grit/gfx_resources.h"
#include "third_party/skia/include/effects/SkGradientShader.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include "ui/gfx/skia_util.h"

namespace gfx {

static const unsigned int kButtonLength = 14;
static const unsigned int kScrollbarWidth = 15;
static const unsigned int kThumbInactiveColor = 0xeaeaea;
static const unsigned int kTrackColor= 0xd3d3d3;

// These are the default dimensions of radio buttons and checkboxes.
static const int kCheckboxAndRadioWidth = 13;
static const int kCheckboxAndRadioHeight = 13;

// These sizes match the sizes in Chromium Win.
static const int kSliderThumbWidth = 11;
static const int kSliderThumbHeight = 21;

static const SkColor kSliderTrackBackgroundColor =
    SkColorSetRGB(0xe3, 0xdd, 0xd8);
static const SkColor kSliderThumbLightGrey = SkColorSetRGB(0xf4, 0xf2, 0xef);
static const SkColor kSliderThumbDarkGrey = SkColorSetRGB(0xea, 0xe5, 0xe0);
static const SkColor kSliderThumbBorderDarkGrey =
    SkColorSetRGB(0x9d, 0x96, 0x8e);

static const SkColor kCheckboxTinyColor = SK_ColorGRAY;
static const SkColor kCheckboxShadowColor = SkColorSetARGB(0x15, 0, 0, 0);
static const SkColor kCheckboxShadowHoveredColor =
    SkColorSetARGB(0x1F, 0, 0, 0);
static const SkColor kCheckboxGradientColors[] = {
    SkColorSetRGB(0xed, 0xed, 0xed),
    SkColorSetRGB(0xde, 0xde, 0xde) };
static const SkColor kCheckboxGradientPressedColors[] = {
    SkColorSetRGB(0xe7, 0xe7, 0xe7),
    SkColorSetRGB(0xd7, 0xd7, 0xd7) };
static const SkColor kCheckboxGradientHoveredColors[] = {
    SkColorSetRGB(0xf0, 0xf0, 0xf0),
    SkColorSetRGB(0xe0, 0xe0, 0xe0) };
static const SkColor kCheckboxGradientDisabledColors[] = {
    SkColorSetARGB(0xB3, 0xed, 0xed, 0xed),
    SkColorSetARGB(0xB3, 0xed, 0xed, 0xed) };
static const SkColor kCheckboxBorderColor = SkColorSetARGB(0x40, 0, 0, 0);
static const SkColor kCheckboxBorderHoveredColor =
    SkColorSetARGB(0x4D, 0, 0, 0);
static const SkColor kCheckboxBorderDisabledColor =
    SkColorSetARGB(0x30, 0, 0, 0);
static const SkColor kCheckboxStrokeColor = SkColorSetARGB(0xB3, 0, 0, 0);
static const SkColor kCheckboxStrokeDisabledColor =
    SkColorSetARGB(0x86, 0, 0, 0);
static const SkColor kRadioDotColor = SkColorSetRGB(0x66, 0x66, 0x66);
static const SkColor kRadioDotDisabledColor =
    SkColorSetARGB(0xC0, 0x66, 0x66, 0x66);

// Get lightness adjusted color.
static SkColor BrightenColor(const color_utils::HSL& hsl,
                             SkAlpha alpha,
                             double lightness_amount) {
  color_utils::HSL adjusted = hsl;
  adjusted.l += lightness_amount;
  if (adjusted.l > 1.0)
    adjusted.l = 1.0;
  if (adjusted.l < 0.0)
    adjusted.l = 0.0;

  return color_utils::HSLToSkColor(adjusted, alpha);
}

// static
NativeThemeAndroid* NativeThemeAndroid::instance() {
  CR_DEFINE_STATIC_LOCAL(NativeThemeAndroid, s_native_theme, ());
  return &s_native_theme;
}

gfx::Size NativeThemeAndroid::GetPartSize(Part part) const {
  switch (part) {
    case SCROLLBAR_DOWN_ARROW:
    case SCROLLBAR_UP_ARROW:
      return gfx::Size(kScrollbarWidth, kButtonLength);
    case SCROLLBAR_LEFT_ARROW:
    case SCROLLBAR_RIGHT_ARROW:
      return gfx::Size(kButtonLength, kScrollbarWidth);
    case CHECKBOX:
    case RADIO:
      return gfx::Size(kCheckboxAndRadioWidth, kCheckboxAndRadioHeight);
    case SLIDER_THUMB:
      // These sizes match the sizes in Chromium Win.
      return gfx::Size(kSliderThumbWidth, kSliderThumbHeight);
    case INNER_SPIN_BUTTON:
      return gfx::Size(kScrollbarWidth, 0);
    case PUSH_BUTTON:
    case TEXTFIELD:
    case MENU_LIST:
    case SLIDER_TRACK:
    case PROGRESS_BAR:
      return gfx::Size();  // No default size.
  }
  return gfx::Size();
}

void NativeThemeAndroid::Paint(SkCanvas* canvas,
                               Part part,
                               State state,
                               const gfx::Rect& rect,
                               const ExtraParams& extra) {
  switch (part) {
    case SCROLLBAR_DOWN_ARROW:
    case SCROLLBAR_UP_ARROW:
    case SCROLLBAR_LEFT_ARROW:
    case SCROLLBAR_RIGHT_ARROW:
      PaintArrowButton(canvas, rect, part, state);
      break;
    case CHECKBOX:
      PaintCheckbox(canvas, state, rect, extra.button);
      break;
    case RADIO:
      PaintRadio(canvas, state, rect, extra.button);
      break;
    case PUSH_BUTTON:
      PaintButton(canvas, state, rect, extra.button);
      break;
    case TEXTFIELD:
      PaintTextField(canvas, state, rect, extra.text_field);
      break;
    case MENU_LIST:
      PaintMenuList(canvas, state, rect, extra.menu_list);
      break;
    case SLIDER_TRACK:
      PaintSliderTrack(canvas, state, rect, extra.slider);
      break;
    case SLIDER_THUMB:
      PaintSliderThumb(canvas, state, rect, extra.slider);
      break;
    case INNER_SPIN_BUTTON:
      PaintInnerSpinButton(canvas, state, rect, extra.inner_spin);
      break;
    case PROGRESS_BAR:
      PaintProgressBar(canvas, state, rect, extra.progress_bar);
      break;
    default:
      NOTREACHED();
  }
}

NativeThemeAndroid::NativeThemeAndroid() {
}

NativeThemeAndroid::~NativeThemeAndroid() {
}

void NativeThemeAndroid::PaintArrowButton(SkCanvas* canvas,
                                          const gfx::Rect& rect,
                                          Part direction,
                                          State state) {
  int widthMiddle;
  int lengthMiddle;
  SkPaint paint;
  if (direction == SCROLLBAR_UP_ARROW || direction == SCROLLBAR_DOWN_ARROW) {
    widthMiddle = rect.width() / 2 + 1;
    lengthMiddle = rect.height() / 2 + 1;
  } else {
    lengthMiddle = rect.width() / 2 + 1;
    widthMiddle = rect.height() / 2 + 1;
  }

  // Calculate button color.
  SkScalar trackHSV[3];
  SkColorToHSV(kTrackColor, trackHSV);
  SkColor buttonColor = SaturateAndBrighten(trackHSV, 0, 0.2);
  SkColor backgroundColor = buttonColor;
  if (state == PRESSED) {
    SkScalar buttonHSV[3];
    SkColorToHSV(buttonColor, buttonHSV);
    buttonColor = SaturateAndBrighten(buttonHSV, 0, -0.1);
  } else if (state == HOVERED) {
    SkScalar buttonHSV[3];
    SkColorToHSV(buttonColor, buttonHSV);
    buttonColor = SaturateAndBrighten(buttonHSV, 0, 0.05);
  }

  SkIRect skrect;
  skrect.set(rect.x(), rect.y(), rect.x() + rect.width(), rect.y()
      + rect.height());
  // Paint the background (the area visible behind the rounded corners).
  paint.setColor(backgroundColor);
  canvas->drawIRect(skrect, paint);

  // Paint the button's outline and fill the middle
  SkPath outline;
  switch (direction) {
    case SCROLLBAR_UP_ARROW:
      outline.moveTo(rect.x() + 0.5, rect.y() + rect.height() + 0.5);
      outline.rLineTo(0, -(rect.height() - 2));
      outline.rLineTo(2, -2);
      outline.rLineTo(rect.width() - 5, 0);
      outline.rLineTo(2, 2);
      outline.rLineTo(0, rect.height() - 2);
      break;
    case SCROLLBAR_DOWN_ARROW:
      outline.moveTo(rect.x() + 0.5, rect.y() - 0.5);
      outline.rLineTo(0, rect.height() - 2);
      outline.rLineTo(2, 2);
      outline.rLineTo(rect.width() - 5, 0);
      outline.rLineTo(2, -2);
      outline.rLineTo(0, -(rect.height() - 2));
      break;
    case SCROLLBAR_RIGHT_ARROW:
      outline.moveTo(rect.x() - 0.5, rect.y() + 0.5);
      outline.rLineTo(rect.width() - 2, 0);
      outline.rLineTo(2, 2);
      outline.rLineTo(0, rect.height() - 5);
      outline.rLineTo(-2, 2);
      outline.rLineTo(-(rect.width() - 2), 0);
      break;
    case SCROLLBAR_LEFT_ARROW:
      outline.moveTo(rect.x() + rect.width() + 0.5, rect.y() + 0.5);
      outline.rLineTo(-(rect.width() - 2), 0);
      outline.rLineTo(-2, 2);
      outline.rLineTo(0, rect.height() - 5);
      outline.rLineTo(2, 2);
      outline.rLineTo(rect.width() - 2, 0);
      break;
    default:
      break;
  }
  outline.close();

  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(buttonColor);
  canvas->drawPath(outline, paint);

  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  SkScalar thumbHSV[3];
  SkColorToHSV(kThumbInactiveColor, thumbHSV);
  paint.setColor(OutlineColor(trackHSV, thumbHSV));
  canvas->drawPath(outline, paint);

  // If the button is disabled or read-only, the arrow is drawn with the
  // outline color.
  if (state != DISABLED)
    paint.setColor(SK_ColorBLACK);

  paint.setAntiAlias(false);
  paint.setStyle(SkPaint::kFill_Style);

  SkPath path;
  // The constants in this block of code are hand-tailored to produce good
  // looking arrows without anti-aliasing.
  switch (direction) {
    case SCROLLBAR_UP_ARROW:
      path.moveTo(rect.x() + widthMiddle - 4, rect.y() + lengthMiddle + 2);
      path.rLineTo(7, 0);
      path.rLineTo(-4, -4);
      break;
    case SCROLLBAR_DOWN_ARROW:
      path.moveTo(rect.x() + widthMiddle - 4, rect.y() + lengthMiddle - 3);
      path.rLineTo(7, 0);
      path.rLineTo(-4, 4);
      break;
    case SCROLLBAR_RIGHT_ARROW:
      path.moveTo(rect.x() + lengthMiddle - 3, rect.y() + widthMiddle - 4);
      path.rLineTo(0, 7);
      path.rLineTo(4, -4);
      break;
    case SCROLLBAR_LEFT_ARROW:
      path.moveTo(rect.x() + lengthMiddle + 1, rect.y() + widthMiddle - 5);
      path.rLineTo(0, 9);
      path.rLineTo(-4, -4);
      break;
    default:
      break;
  }
  path.close();

  canvas->drawPath(path, paint);
}

// Draws the common elements of checkboxes and radio buttons.
// Returns the rectangle within which any additional decorations should be
// drawn, or empty if none.
SkRect NativeThemeAndroid::PaintCheckboxRadioCommon(
    SkCanvas* canvas,
    State state,
    const gfx::Rect& rect,
    const SkScalar borderRadius) const {

  SkRect skrect = gfx::RectToSkRect(rect);

  // Use the largest square that fits inside the provided rectangle.
  // No other browser seems to support non-square widget, so accidentally
  // having non-square sizes is common (eg. amazon and webkit dev tools).
  if (skrect.width() != skrect.height()) {
    SkScalar size = SkMinScalar(skrect.width(), skrect.height());
    skrect.inset((skrect.width() - size) / 2, (skrect.height() - size) / 2);
  }

  // If the rectangle is too small then paint only a rectangle.  We don't want
  // to have to worry about '- 1' and '+ 1' calculations below having overflow
  // or underflow.
  if (skrect.width() <= 2) {
    SkPaint paint;
    paint.setColor(kCheckboxTinyColor);
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(skrect, paint);
    // Too small to draw anything more.
    return SkRect::MakeEmpty();
  }

  // Make room for the drop shadow.
  skrect.iset(skrect.left(), skrect.top(),
              skrect.right() - 1, skrect.bottom() - 1);

  // Draw the drop shadow below the widget.
  if (state != PRESSED) {
    SkPaint paint;
    paint.setAntiAlias(true);
    SkRect shadowRect = skrect;
    shadowRect.offset(0, 1);
    if (state == HOVERED)
      paint.setColor(kCheckboxShadowHoveredColor);
    else
      paint.setColor(kCheckboxShadowColor);
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRoundRect(shadowRect, borderRadius, borderRadius, paint);
  }

  // Draw the gradient-filled rectangle
  SkPoint gradient_bounds[3];
  gradient_bounds[0].set(skrect.left(), skrect.top());
  gradient_bounds[1].set(skrect.left(), skrect.top() + skrect.height() * 0.38);
  gradient_bounds[2].set(skrect.left(), skrect.bottom());
  const SkColor* startEndColors;
  if (state == PRESSED)
    startEndColors = kCheckboxGradientPressedColors;
  else if (state == HOVERED)
    startEndColors = kCheckboxGradientHoveredColors;
  else if (state == DISABLED)
    startEndColors = kCheckboxGradientDisabledColors;
  else /* kNormal */
    startEndColors = kCheckboxGradientColors;
  SkColor colors[3] = {startEndColors[0], startEndColors[0], startEndColors[1]};
  SkShader* shader = SkGradientShader::CreateLinear(
      gradient_bounds, colors, NULL, 3, SkShader::kClamp_TileMode, NULL);
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setShader(shader);
  shader->unref();
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRoundRect(skrect, borderRadius, borderRadius, paint);
  paint.setShader(NULL);

  // Draw the border.
  if (state == HOVERED)
    paint.setColor(kCheckboxBorderHoveredColor);
  else if (state == DISABLED)
    paint.setColor(kCheckboxBorderDisabledColor);
  else
    paint.setColor(kCheckboxBorderColor);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(SkIntToScalar(1));
  skrect.inset(SkFloatToScalar(.5f), SkFloatToScalar(.5f));
  canvas->drawRoundRect(skrect, borderRadius, borderRadius, paint);

  // Return the rectangle excluding the drop shadow for drawing any additional
  // decorations.
  return skrect;
}

void NativeThemeAndroid::PaintCheckbox(SkCanvas* canvas,
                                       State state,
                                       const gfx::Rect& rect,
                                       const ButtonExtraParams& button) {
  SkRect skrect = PaintCheckboxRadioCommon(canvas, state, rect,
                                           SkIntToScalar(2));
  if (!skrect.isEmpty()) {
    // Draw the checkmark / dash.
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    if (state == DISABLED)
      paint.setColor(kCheckboxStrokeDisabledColor);
    else
      paint.setColor(kCheckboxStrokeColor);
    if (button.indeterminate) {
      SkPath dash;
      dash.moveTo(skrect.left() + skrect.width() * 0.16,
                  (skrect.top() + skrect.bottom()) / 2);
      dash.rLineTo(skrect.width() * 0.68, 0);
      paint.setStrokeWidth(SkFloatToScalar(skrect.height() * 0.08));
      canvas->drawPath(dash, paint);
    } else if (button.checked) {
      SkPath check;
      check.moveTo(skrect.left() + skrect.width() * 0.2,
                   skrect.top() + skrect.height() * 0.5);
      check.rLineTo(skrect.width() * 0.2, skrect.height() * 0.2);
      paint.setStrokeWidth(SkFloatToScalar(skrect.height() * 0.23));
      check.lineTo(skrect.right() - skrect.width() * 0.2,
                   skrect.top() + skrect.height() * 0.2);
      canvas->drawPath(check, paint);
    }
  }
}

void NativeThemeAndroid::PaintRadio(SkCanvas* canvas,
                                    State state,
                                    const gfx::Rect& rect,
                                    const ButtonExtraParams& button) {
  // Most of a radio button is the same as a checkbox, except the the rounded
  // square is a circle (i.e. border radius >= 100%).
  const SkScalar radius = SkFloatToScalar(
      static_cast<float>(std::max(rect.width(), rect.height())) / 2);
  SkRect skrect = PaintCheckboxRadioCommon(canvas, state, rect, radius);
  if (!skrect.isEmpty() && button.checked) {
    // Draw the dot.
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    if (state == DISABLED)
      paint.setColor(kRadioDotDisabledColor);
    else
      paint.setColor(kRadioDotColor);
    skrect.inset(skrect.width() * 0.25, skrect.height() * 0.25);
    // Use drawRoundedRect instead of drawOval to be completely consistent
    // with the border in PaintCheckboxRadioNewCommon.
    canvas->drawRoundRect(skrect, radius, radius, paint);
  }
}

void NativeThemeAndroid::PaintButton(SkCanvas* canvas,
                                     State state,
                                     const gfx::Rect& rect,
                                     const ButtonExtraParams& button) {
  SkPaint paint;
  SkRect skrect;
  int kRight = rect.right();
  int kBottom = rect.bottom();
  SkColor base_color = button.background_color;

  color_utils::HSL base_hsl;
  color_utils::SkColorToHSL(base_color, &base_hsl);

  // Our standard gradient is from 0xdd to 0xf8. This is the amount of
  // increased luminance between those values.
  SkColor light_color(BrightenColor(base_hsl, SkColorGetA(base_color), 0.105));

  // If the button is too small, fallback to drawing a single, solid color
  if (rect.width() < 5 || rect.height() < 5) {
    paint.setColor(base_color);
    skrect.set(rect.x(), rect.y(), kRight, kBottom);
    canvas->drawRect(skrect, paint);
    return;
  }

  if (button.has_border) {
    int kBorderAlpha = state == HOVERED ? 0x80 : 0x55;
    paint.setARGB(kBorderAlpha, 0, 0, 0);
    canvas->drawLine(rect.x() + 1, rect.y(), kRight - 1, rect.y(), paint);
    canvas->drawLine(kRight - 1, rect.y() + 1, kRight - 1, kBottom - 1, paint);
    canvas->drawLine(rect.x() + 1, kBottom - 1, kRight - 1, kBottom - 1, paint);
    canvas->drawLine(rect.x(), rect.y() + 1, rect.x(), kBottom - 1, paint);
  }

  paint.setColor(SK_ColorBLACK);
  int kLightEnd = state == PRESSED ? 1 : 0;
  int kDarkEnd = !kLightEnd;
  SkPoint gradient_bounds[2];
  gradient_bounds[kLightEnd].set(SkIntToScalar(rect.x()),
                                 SkIntToScalar(rect.y()));
  gradient_bounds[kDarkEnd].set(SkIntToScalar(rect.x()),
                                SkIntToScalar(kBottom - 1));
  SkColor colors[2];
  colors[0] = light_color;
  colors[1] = base_color;

  SkShader* shader = SkGradientShader::CreateLinear(
      gradient_bounds, colors, NULL, 2, SkShader::kClamp_TileMode, NULL);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setShader(shader);
  shader->unref();

  if (button.has_border) {
    skrect.set(rect.x() + 1, rect.y() + 1, kRight - 1, kBottom - 1);
  } else {
    skrect.set(rect.x(), rect.y(), kRight, kBottom);
  }
  canvas->drawRect(skrect, paint);
  paint.setShader(NULL);

  if (button.has_border) {
    paint.setColor(BrightenColor(base_hsl, SkColorGetA(base_color), -0.0588));
    canvas->drawPoint(rect.x() + 1, rect.y() + 1, paint);
    canvas->drawPoint(kRight - 2, rect.y() + 1, paint);
    canvas->drawPoint(rect.x() + 1, kBottom - 2, paint);
    canvas->drawPoint(kRight - 2, kBottom - 2, paint);
  }
}

void NativeThemeAndroid::PaintTextField(SkCanvas* canvas,
                                        State state,
                                        const gfx::Rect& rect,
                                        const TextFieldExtraParams& text) {
  // The following drawing code simulates the user-agent css border for
  // text area and text input so that we do not break layout tests. Once we
  // have decided the desired looks, we should update the code here and
  // the layout test expectations.
  SkRect bounds;
  bounds.set(rect.x(), rect.y(), rect.right() - 1, rect.bottom() - 1);

  SkPaint fill_paint;
  fill_paint.setStyle(SkPaint::kFill_Style);
  fill_paint.setColor(text.background_color);
  canvas->drawRect(bounds, fill_paint);

  if (text.is_text_area) {
    // Draw text area border: 1px solid black
    SkPaint stroke_paint;
    fill_paint.setStyle(SkPaint::kStroke_Style);
    fill_paint.setColor(SK_ColorBLACK);
    canvas->drawRect(bounds, fill_paint);
  } else {
    // Draw text input and listbox inset border
    //   Text Input: 2px inset #eee
    //   Listbox: 1px inset #808080
    SkColor kLightColor = text.is_listbox ?
        SkColorSetRGB(0x80, 0x80, 0x80) : SkColorSetRGB(0xee, 0xee, 0xee);
    SkColor kDarkColor = text.is_listbox ?
        SkColorSetRGB(0x2c, 0x2c, 0x2c) : SkColorSetRGB(0x9a, 0x9a, 0x9a);
    int kBorderWidth = text.is_listbox ? 1 : 2;

    SkPaint dark_paint;
    dark_paint.setAntiAlias(true);
    dark_paint.setStyle(SkPaint::kFill_Style);
    dark_paint.setColor(kDarkColor);

    SkPaint light_paint;
    light_paint.setAntiAlias(true);
    light_paint.setStyle(SkPaint::kFill_Style);
    light_paint.setColor(kLightColor);

    int left = rect.x();
    int top = rect.y();
    int right = rect.right();
    int bottom = rect.bottom();

    SkPath path;
    path.incReserve(4);

    // Top
    path.moveTo(SkIntToScalar(left), SkIntToScalar(top));
    path.lineTo(SkIntToScalar(left + kBorderWidth),
                SkIntToScalar(top + kBorderWidth));
    path.lineTo(SkIntToScalar(right - kBorderWidth),
                SkIntToScalar(top + kBorderWidth));
    path.lineTo(SkIntToScalar(right), SkIntToScalar(top));
    canvas->drawPath(path, dark_paint);

    // Bottom
    path.reset();
    path.moveTo(SkIntToScalar(left + kBorderWidth),
                SkIntToScalar(bottom - kBorderWidth));
    path.lineTo(SkIntToScalar(left), SkIntToScalar(bottom));
    path.lineTo(SkIntToScalar(right), SkIntToScalar(bottom));
    path.lineTo(SkIntToScalar(right - kBorderWidth),
                SkIntToScalar(bottom - kBorderWidth));
    canvas->drawPath(path, light_paint);

    // Left
    path.reset();
    path.moveTo(SkIntToScalar(left), SkIntToScalar(top));
    path.lineTo(SkIntToScalar(left), SkIntToScalar(bottom));
    path.lineTo(SkIntToScalar(left + kBorderWidth),
                SkIntToScalar(bottom - kBorderWidth));
    path.lineTo(SkIntToScalar(left + kBorderWidth),
                SkIntToScalar(top + kBorderWidth));
    canvas->drawPath(path, dark_paint);

    // Right
    path.reset();
    path.moveTo(SkIntToScalar(right - kBorderWidth),
                SkIntToScalar(top + kBorderWidth));
    path.lineTo(SkIntToScalar(right - kBorderWidth), SkIntToScalar(bottom));
    path.lineTo(SkIntToScalar(right), SkIntToScalar(bottom));
    path.lineTo(SkIntToScalar(right), SkIntToScalar(top));
    canvas->drawPath(path, light_paint);
  }
}

void NativeThemeAndroid::PaintMenuList(SkCanvas* canvas,
                                       State state,
                                       const gfx::Rect& rect,
                                       const MenuListExtraParams& menu_list) {
  // If a border radius is specified, we let the WebCore paint the background
  // and the border of the control.
  if (!menu_list.has_border_radius) {
    ButtonExtraParams button = { 0 };
    button.background_color = menu_list.background_color;
    button.has_border = menu_list.has_border;
    PaintButton(canvas, state, rect, button);
  }

  SkPaint paint;
  paint.setColor(SK_ColorBLACK);
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);

  SkPath path;
  path.moveTo(menu_list.arrow_x, menu_list.arrow_y - 3);
  path.rLineTo(6, 0);
  path.rLineTo(-3, 6);
  path.close();
  canvas->drawPath(path, paint);
}

void NativeThemeAndroid::PaintSliderTrack(SkCanvas* canvas,
                                          State state,
                                          const gfx::Rect& rect,
                                          const SliderExtraParams& slider) {
  int kMidX = rect.x() + rect.width() / 2;
  int kMidY = rect.y() + rect.height() / 2;

  SkPaint paint;
  paint.setColor(kSliderTrackBackgroundColor);

  SkRect skrect;
  if (slider.vertical) {
    skrect.set(std::max(rect.x(), kMidX - 2),
               rect.y(),
               std::min(rect.right(), kMidX + 2),
               rect.bottom());
  } else {
    skrect.set(rect.x(),
               std::max(rect.y(), kMidY - 2),
               rect.right(),
               std::min(rect.bottom(), kMidY + 2));
  }
  canvas->drawRect(skrect, paint);
}

void NativeThemeAndroid::PaintSliderThumb(SkCanvas* canvas,
                                          State state,
                                          const gfx::Rect& rect,
                                          const SliderExtraParams& slider) {
  bool hovered = (state == HOVERED) || slider.in_drag;
  int kMidX = rect.x() + rect.width() / 2;
  int kMidY = rect.y() + rect.height() / 2;

  SkPaint paint;
  paint.setColor(hovered ? SK_ColorWHITE : kSliderThumbLightGrey);

  SkIRect skrect;
  if (slider.vertical)
    skrect.set(rect.x(), rect.y(), kMidX + 1, rect.bottom());
  else
    skrect.set(rect.x(), rect.y(), rect.right(), kMidY + 1);

  canvas->drawIRect(skrect, paint);

  paint.setColor(hovered ? kSliderThumbLightGrey : kSliderThumbDarkGrey);

  if (slider.vertical)
    skrect.set(kMidX + 1, rect.y(), rect.right(), rect.bottom());
  else
    skrect.set(rect.x(), kMidY + 1, rect.right(), rect.bottom());

  canvas->drawIRect(skrect, paint);

  paint.setColor(kSliderThumbBorderDarkGrey);
  DrawBox(canvas, rect, paint);

  if (rect.height() > 10 && rect.width() > 10) {
    DrawHorizLine(canvas, kMidX - 2, kMidX + 2, kMidY, paint);
    DrawHorizLine(canvas, kMidX - 2, kMidX + 2, kMidY - 3, paint);
    DrawHorizLine(canvas, kMidX - 2, kMidX + 2, kMidY + 3, paint);
  }
}

void NativeThemeAndroid::PaintInnerSpinButton(
    SkCanvas* canvas,
    State state,
    const gfx::Rect& rect,
    const InnerSpinButtonExtraParams& spin_button) {
  if (spin_button.read_only)
    state = DISABLED;

  State north_state = state;
  State south_state = state;
  if (spin_button.spin_up)
    south_state = south_state != DISABLED ? NORMAL : DISABLED;
  else
    north_state = north_state != DISABLED ? NORMAL : DISABLED;

  gfx::Rect half = rect;
  half.set_height(rect.height() / 2);
  PaintArrowButton(canvas, half, SCROLLBAR_UP_ARROW, north_state);

  half.set_y(rect.y() + rect.height() / 2);
  PaintArrowButton(canvas, half, SCROLLBAR_DOWN_ARROW, south_state);
}

void NativeThemeAndroid::PaintProgressBar(
    SkCanvas* canvas,
    State state,
    const gfx::Rect& rect,
    const ProgressBarExtraParams& progress_bar) {
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  SkBitmap* bar_image = rb.GetBitmapNamed(IDR_PROGRESS_BAR);
  SkBitmap* left_border_image = rb.GetBitmapNamed(IDR_PROGRESS_BORDER_LEFT);
  SkBitmap* right_border_image = rb.GetBitmapNamed(IDR_PROGRESS_BORDER_RIGHT);

  double tile_scale = static_cast<double>(rect.height()) /
      bar_image->height();

  int new_tile_width = static_cast<int>(bar_image->width() * tile_scale);
  double tile_scale_x = static_cast<double>(new_tile_width) /
      bar_image->width();

  DrawTiledImage(canvas, *bar_image, 0, 0, tile_scale_x, tile_scale,
      rect.x(), rect.y(), rect.width(), rect.height());

  if (progress_bar.value_rect_width) {
    SkBitmap* value_image = rb.GetBitmapNamed(IDR_PROGRESS_VALUE);

    new_tile_width = static_cast<int>(value_image->width() * tile_scale);
    tile_scale_x = static_cast<double>(new_tile_width) /
        value_image->width();

    DrawTiledImage(canvas, *value_image, 0, 0, tile_scale_x, tile_scale,
        progress_bar.value_rect_x,
        progress_bar.value_rect_y,
        progress_bar.value_rect_width,
        progress_bar.value_rect_height);
  }

  int dest_left_border_width = static_cast<int>(left_border_image->width() *
      tile_scale);
  SkRect dest_rect = {
      SkIntToScalar(rect.x()),
      SkIntToScalar(rect.y()),
      SkIntToScalar(rect.x() + dest_left_border_width),
      SkIntToScalar(rect.bottom())
  };
  canvas->drawBitmapRect(*left_border_image, NULL, dest_rect);

  int dest_right_border_width = static_cast<int>(right_border_image->width() *
      tile_scale);
  dest_rect.set(SkIntToScalar(rect.right() - dest_right_border_width),
      SkIntToScalar(rect.y()),
      SkIntToScalar(rect.right()),
      SkIntToScalar(rect.bottom()));
  canvas->drawBitmapRect(*right_border_image, NULL, dest_rect);
}

bool NativeThemeAndroid::IntersectsClipRectInt(SkCanvas* canvas,
                                               int x,
                                               int y,
                                               int w,
                                               int h) {
  SkRect clip;
  return canvas->getClipBounds(&clip) &&
      clip.intersect(SkIntToScalar(x), SkIntToScalar(y), SkIntToScalar(x + w),
                     SkIntToScalar(y + h));
}

void NativeThemeAndroid::DrawBitmapInt(SkCanvas* canvas,
                                       const SkBitmap& bitmap,
                                       int src_x,
                                       int src_y,
                                       int src_w,
                                       int src_h,
                                       int dest_x,
                                       int dest_y,
                                       int dest_w,
                                       int dest_h) {
  DLOG_ASSERT(src_x + src_w < std::numeric_limits<int16_t>::max() &&
              src_y + src_h < std::numeric_limits<int16_t>::max());
  if (src_w <= 0 || src_h <= 0 || dest_w <= 0 || dest_h <= 0) {
    NOTREACHED() << "Attempting to draw bitmap to/from an empty rect!";
    return;
  }

  if (!IntersectsClipRectInt(canvas, dest_x, dest_y, dest_w, dest_h))
    return;

  SkRect dest_rect = { SkIntToScalar(dest_x),
                       SkIntToScalar(dest_y),
                       SkIntToScalar(dest_x + dest_w),
                       SkIntToScalar(dest_y + dest_h) };

  if (src_w == dest_w && src_h == dest_h) {
    // Workaround for apparent bug in Skia that causes image to occasionally
    // shift.
    SkIRect src_rect = { src_x, src_y, src_x + src_w, src_y + src_h };
    canvas->drawBitmapRect(bitmap, &src_rect, dest_rect);
    return;
  }

  // Make a bitmap shader that contains the bitmap we want to draw. This is
  // basically what SkCanvas.drawBitmap does internally, but it gives us
  // more control over quality and will use the mipmap in the source image if
  // it has one, whereas drawBitmap won't.
  SkShader* shader = SkShader::CreateBitmapShader(bitmap,
                                                  SkShader::kRepeat_TileMode,
                                                  SkShader::kRepeat_TileMode);
  SkMatrix shader_scale;
  shader_scale.setScale(SkFloatToScalar(static_cast<float>(dest_w) / src_w),
                        SkFloatToScalar(static_cast<float>(dest_h) / src_h));
  shader_scale.preTranslate(SkIntToScalar(-src_x), SkIntToScalar(-src_y));
  shader_scale.postTranslate(SkIntToScalar(dest_x), SkIntToScalar(dest_y));
  shader->setLocalMatrix(shader_scale);

  // The rect will be filled by the bitmap.
  SkPaint p;
  p.setFilterBitmap(true);
  p.setShader(shader);
  shader->unref();
  canvas->drawRect(dest_rect, p);
}

void NativeThemeAndroid::DrawTiledImage(SkCanvas* canvas,
                                        const SkBitmap& bitmap,
                                        int src_x,
                                        int src_y,
                                        double tile_scale_x,
                                        double tile_scale_y,
                                        int dest_x,
                                        int dest_y,
                                        int w,
                                        int h) const {
  SkShader* shader = SkShader::CreateBitmapShader(bitmap,
                                                  SkShader::kRepeat_TileMode,
                                                  SkShader::kRepeat_TileMode);
  if (tile_scale_x != 1.0 || tile_scale_y != 1.0) {
    SkMatrix shader_scale;
    shader_scale.setScale(SkDoubleToScalar(tile_scale_x),
                          SkDoubleToScalar(tile_scale_y));
    shader->setLocalMatrix(shader_scale);
  }

  SkPaint paint;
  paint.setShader(shader);
  paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);

  // CreateBitmapShader returns a Shader with a reference count of one, we
  // need to unref after paint takes ownership of the shader.
  shader->unref();
  canvas->save();
  canvas->translate(SkIntToScalar(dest_x - src_x),
                    SkIntToScalar(dest_y - src_y));
  canvas->clipRect(SkRect::MakeXYWH(src_x, src_y, w, h));
  canvas->drawPaint(paint);
  canvas->restore();
}

SkColor NativeThemeAndroid::SaturateAndBrighten(
    SkScalar* hsv,
    SkScalar saturate_amount,
    SkScalar brighten_amount) const {
  SkScalar color[3];
  color[0] = hsv[0];
  color[1] = Clamp(hsv[1] + saturate_amount, 0.0, 1.0);
  color[2] = Clamp(hsv[2] + brighten_amount, 0.0, 1.0);
  return SkHSVToColor(color);
}

SkScalar NativeThemeAndroid::Clamp(SkScalar value,
                                   SkScalar min,
                                   SkScalar max) const {
  return std::min(std::max(value, min), max);
}

void NativeThemeAndroid::DrawVertLine(SkCanvas* canvas,
                                      int x,
                                      int y1,
                                      int y2,
                                      const SkPaint& paint) const {
  SkIRect skrect;
  skrect.set(x, y1, x + 1, y2 + 1);
  canvas->drawIRect(skrect, paint);
}

void NativeThemeAndroid::DrawHorizLine(SkCanvas* canvas,
                                       int x1,
                                       int x2,
                                       int y,
                                       const SkPaint& paint) const {
  SkIRect skrect;
  skrect.set(x1, y, x2 + 1, y + 1);
  canvas->drawIRect(skrect, paint);
}

void NativeThemeAndroid::DrawBox(SkCanvas* canvas,
                                 const gfx::Rect& rect,
                                 const SkPaint& paint) const {
  int right = rect.x() + rect.width() - 1;
  int bottom = rect.y() + rect.height() - 1;
  DrawHorizLine(canvas, rect.x(), right, rect.y(), paint);
  DrawVertLine(canvas, right, rect.y(), bottom, paint);
  DrawHorizLine(canvas, rect.x(), right, bottom, paint);
  DrawVertLine(canvas, rect.x(), rect.y(), bottom, paint);
}

SkColor NativeThemeAndroid::OutlineColor(SkScalar* hsv1, SkScalar* hsv2) const {
  SkScalar min_diff = Clamp((hsv1[1] + hsv2[1]) * 1.2, 0.28, 0.5);
  SkScalar diff = Clamp(fabs(hsv1[2] - hsv2[2]) / 2, min_diff, 0.5);

  if (hsv1[2] + hsv2[2] > 1.0)
    diff = -diff;

  return SaturateAndBrighten(hsv2, -0.2, diff);
}

}  // namespace gfx
