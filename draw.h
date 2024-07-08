#pragma once

#include <string>

#include <esphome.h>

#include "ticks.h"
#include "dither.h"


// true if price is above warning threshold, regardless of whether
// price warning is enabled
bool price_at_warning_level = false;


inline void on_price_warning_switch_change() {
  if (price_at_warning_level)
    id(epaper).update();
}


template<typename T>
static void draw(T& it) {

  const int BAR_WIDTH = 4;
  const int GRAPH_MARGIN_TOP = 6;  // space for topmost axis label

  // This should be divisible by as many of 2, 3, 4, and 5 as
  // possible (and also 7 and 9 as a secondary objective)
  const int GRAPH_YGRID_HEIGHT = 100;

  const int HOUR_INDICATOR_HEIGHT = 4;

  const int CUR_PRICE_TOP = 10;
  const int CUR_PRICE_WIDTH = 48;

  // this should be in font[].glyphs in the yaml file:
  const char DECIMAL_SEPARATOR = ',';

  const int GRAPH_WIDTH = 48*BAR_WIDTH;
  const int GRAPH_HEIGHT = GRAPH_YGRID_HEIGHT + GRAPH_MARGIN_TOP;
  static const int screen_width = it.get_width();
  static const int graph_left =
    CUR_PRICE_WIDTH + (screen_width - CUR_PRICE_WIDTH - GRAPH_WIDTH)/2;
  static const int screen_height = it.get_height();
  static const int graph_margin_bottom = screen_height - GRAPH_HEIGHT;

  esphome::font::Font* font = &id(main_font);
  esphome::font::Font* price_font = &id(cur_price_font);
  const Color& color_red = id(red);

  const auto& prices = id(hourly_prices);
  auto prices_it = prices.cbegin();
  const auto prices_end = prices.cend();

  // prices contains values for 2 days. If we're at 2nd day, skip 1st
  // day. If we're even further ahead, then we have no usable data.
  ESPTime now = id(homeassistant_time).now();
  ESPTime& start_date = id(prices_start_date);
  if (now.year != start_date.year ||
      now.month != start_date.month ||
      now.day_of_month != start_date.day_of_month)
  {
    ESPTime next_day_from_start(start_date);
    next_day_from_start.increment_day();

    if (now.year == next_day_from_start.year &&
        now.month == next_day_from_start.month &&
        now.day_of_month == next_day_from_start.day_of_month)
    {
      prices_it += 24;
    }
    else {
      prices_it = prices_end;
      ESP_LOGW("draw", "No data available for today. Data starts at %s",
               start_date.strftime(std::string("%F %T%z")).c_str());
    }
  }

  float max_price = -INFINITY;
  // Calculate max value NaN-safely.
  // If there are no non-NaN values, max_price = -INFINITY.
  for (auto it = prices_it; it != prices_end; ++it)
    if (*it > max_price)
      max_price = *it;

  // show alert icon if no actual values
  if (!std::isfinite(max_price)) {
    ESP_LOGW("draw", "No data!");
    it.image(
      screen_width / 2, screen_height / 2,
      &id(no_data_icon),
      ImageAlign::CENTER);
    ESP_LOGD("draw", "Finished drawing.");
    return;
  }

  // print current price
  {
    float price = now.hour >= 0 && now.hour < 24
      ? prices_it[now.hour]
      : NAN;  // this shouldn't happen, but let's not crash if it does
    char str[16];
    snprintf(
      str, sizeof(str), "%.*f",
      price < 10.0f && price > -10.0f ? 1 : 0,
      price);
    for (char& c : str) {
      if (c == '\0')
        break;
      if (c == '.') {
        c = DECIMAL_SEPARATOR;
        break;
      }
    }
    it.print(
      0, CUR_PRICE_TOP,
      price_font, TextAlign::TOP_LEFT,
      str);
    it.print(
      0, CUR_PRICE_TOP + price_font->get_height() + 3,
      font, TextAlign::TOP_LEFT,
      u8"c\u200A/\u200AkWh");  // U+200A = hair space

    // Check if price at warning level. Show warning if warnings
    // enabled. Use gradient values. If within gradient, show black
    // icon. If above gradient, show red icon.
    price_at_warning_level = price >= id(gradient_bottom).state;
    if (price_at_warning_level && id(price_warning_switch).state) {
      esphome::image::Image* img = &id(price_alert_icon);
      bool center = img->get_width() < CUR_PRICE_WIDTH;
      it.image(
        center ? CUR_PRICE_WIDTH/2 : 0,
        screen_height - 1,
        img,
        center ? ImageAlign::BOTTOM_CENTER : ImageAlign::BOTTOM_LEFT,
        price < id(gradient_top).state
        ? esphome::display::COLOR_ON
        : color_red);
    }
  }

  // calculate and draw graph

  const auto yticks = pleasing_ticks(
    // Use space above top y-gridline.
    // Calculate tick placement using a scaled top value.
    std::ceil((max_price * GRAPH_YGRID_HEIGHT) / GRAPH_HEIGHT));
  const auto max_ygrid_val = yticks[0];

  float pixels_per_cent = float(GRAPH_YGRID_HEIGHT) / float(max_ygrid_val);
  float gradient_top_px =
    GRAPH_HEIGHT - (id(gradient_top).state * pixels_per_cent);
  float gradient_bottom_px =
    GRAPH_HEIGHT - (id(gradient_bottom).state * pixels_per_cent);
  ESP_LOGD(
    "draw", "gradient: %g...%g c => %g...%g px",
    id(gradient_bottom).state,
    id(gradient_top).state,
    gradient_bottom_px,
    gradient_top_px);

  BlackRedBars dithered_bar_drawer(
    gradient_top_px, gradient_bottom_px,
    GRAPH_HEIGHT,  // base y
    screen_height,  // y limit
    BAR_WIDTH - 1);  // bar width

  // draw bars
  for (int hour=0, left_x = graph_left;
       prices_it != prices_end;
       ++hour, ++prices_it, left_x += BAR_WIDTH)
    {
      float height = std::round(
        GRAPH_YGRID_HEIGHT * *prices_it / max_ygrid_val);
      ESP_LOGV("draw", "hour %02d: value = %g, height = %g px",
               hour, *prices_it, height);

      // draw bar
      if (std::isfinite(height))
        dithered_bar_drawer.draw_bar(
          left_x + 1, height,
          hour == now.hour,  // red if current hour
          hour < now.hour);  // greyed out if in the past

      // draw current hour indicator
      if (hour == now.hour) {
        int center_x = left_x + BAR_WIDTH/2;
        for (int y=1; y < screen_height; y += 2)
          it.draw_pixel_at(center_x, y, color_red);

        // draw triangle at bottom and also at top if it doesn't
        // overlap with price bar
        for (int y = screen_height - HOUR_INDICATOR_HEIGHT, x = center_x, w = 1;
             y < screen_height;
             ++y, --x, w += 2)
          it.horizontal_line(x, y, w, color_red);
        if (GRAPH_HEIGHT - height > HOUR_INDICATOR_HEIGHT)
          for (int y = HOUR_INDICATOR_HEIGHT - 1, x = center_x, w = 1;
               y >= 0;
               --y, --x, w += 2)
            it.horizontal_line(x, y, w, color_red);
      }
    }

  struct axis_label { int pos; const char* label; };

  // x-axis grid
  for (auto [hour, label] : (axis_label[]) {
    {0, "00"}, {6, "06"}, {12, "12"}, {18, "18"},
    {24, "00"}, {30, "06"}, {36, "12"}, {42, "18"},
    {48, "00"}
  }) {
    int x = graph_left + hour*BAR_WIDTH;
    for (int y=0; y < GRAPH_HEIGHT; y += 3)
      it.draw_pixel_at(x, y);
    it.vertical_line(x, screen_height - graph_margin_bottom, 5);
    it.print(
      x, screen_height - graph_margin_bottom + 4,
      font, TextAlign::TOP_CENTER,
      label);
  }

  // y-axis grid
  for (auto tick_val : yticks) {
    int y = GRAPH_HEIGHT -
      (GRAPH_YGRID_HEIGHT * tick_val) / max_ygrid_val;
    auto label_str = std::to_string(tick_val);
    const char* label = label_str.c_str();

    for (int x = graph_left; x < graph_left + GRAPH_WIDTH; x += 3)
      it.draw_pixel_at(x, y);
    it.horizontal_line(graph_left - 4, y, 4);
    it.print(
      graph_left - 4 - 2, y,
      font, TextAlign::CENTER_RIGHT,
      label);
    it.horizontal_line(graph_left + GRAPH_WIDTH, y, 4);
    it.print(
      graph_left + GRAPH_WIDTH + 4 + 2, y,
      font, TextAlign::CENTER_LEFT,
      label);
  }
  // gridline 0 without text or solid tick lines
  // (text would overlap with x-axis labels)
  for (int x = graph_left - 4; x < graph_left + GRAPH_WIDTH + 4; x += 3)
    it.draw_pixel_at(x, GRAPH_HEIGHT);

  ESP_LOGD("draw", "Finished drawing.");
}
