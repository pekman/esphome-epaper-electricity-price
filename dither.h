#pragma once

#include <cmath>
#include <climits>

#include <esphome.h>

#include "dithermask.h"


static bool apply_dither_mask(int x, int y, uint8_t value) {
  uint8_t threshold =
    DITHER_MASK[y % DITHER_MASK_HEIGHT][x % DITHER_MASK_WIDTH];
  return value > threshold;
}


class BlackRedBars {
  esphome::display::Display& display;

  const esphome::Color color_red;
  const esphome::Color color_black;

  const int gradient_top;
  const int gradient_bottom;
  const int base_y;
  const int y_limit;
  const int bar_width;

public:
  BlackRedBars(
    float gradient_top, float gradient_bottom,
    int base_y, int y_limit,
    int bar_width)
    : display(id(epaper))
    , color_red(id(red))
    , color_black(esphome::display::COLOR_ON)
    , gradient_top(std::isfinite(gradient_top)
                   ? (int) std::round(gradient_top)
                   : INT_MAX)
    , gradient_bottom(std::isfinite(gradient_bottom)
                      ? (int) std::round(gradient_bottom)
                      : INT_MAX)
    , base_y(base_y)
    , y_limit(y_limit)
    , bar_width(bar_width)
  {}

  void draw_bar(int x0, int h, bool red, bool grayed_out) {
    int y0;
    if (h == 0) {
      y0 = base_y;
      h = 1;
    }
    else if (h < 0) {
      h = std::min(-h, y_limit - (base_y + 1));
      y0 = base_y + h;
    }
    else { // h > 0
      y0 = base_y - 1;
    }

    for (int y = y0; y >= 0 && y > y0 - h; --y) {
      uint8_t redness =
        y >= gradient_bottom ? 0 :
        y <= gradient_top ? 0xff :
        0xff - ((y - gradient_top)*0xff) / (gradient_bottom - gradient_top);
      ESP_LOGVV("dither", "y=%d: redness=%u", y, redness);

      for (int x = x0; x < x0 + bar_width; ++x)
        // if grayed_out, draw every 2nd pixel
        if (!grayed_out || (x & 1) ^ (y & 1))
          display.draw_pixel_at(
            x, y,
            red ? color_red :
            apply_dither_mask(x, y, redness) ? color_red : color_black);
    }
  }
};
