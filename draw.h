#pragma once

#include <string>

#include <esphome.h>


template<typename T>
static void draw(T& it) {

  const int BAR_WIDTH = 5;
  const int GRAPH_MARGIN_TOP = 6;  // space for topmost axis label

  // This should be divisible by as many of 2, 3, 4, and 5 as
  // possible (and also 7 and 9 as a secondary objective)
  const int GRAPH_YGRID_HEIGHT = 100;

  const int HOUR_INDICATOR_HEIGHT = 4;

  const int GRAPH_WIDTH = 48*BAR_WIDTH;
  const int GRAPH_HEIGHT = GRAPH_YGRID_HEIGHT + GRAPH_MARGIN_TOP;
  static const int screen_width = it.get_width();
  static const int graph_margin_left = (screen_width - GRAPH_WIDTH)/2;
  static const int screen_height = it.get_height();
  static const int graph_margin_bottom = screen_height - GRAPH_HEIGHT;

  esphome::font::Font* font = &id(main_font);
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
      ESP_LOGW(
        "electricity_price_display",
        "No data available for today. Data starts at %s",
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
    ESP_LOGW("electricity_price_display", "No data!");
    it.image(
      screen_width / 2, screen_height / 2,
      &id(no_data_icon),
      ImageAlign::CENTER);
    return;
  }

  const auto yticks = pleasing_ticks(
    // Use space above top y-gridline.
    // Calculate tick placement using a scaled top value.
    std::ceil((max_price * GRAPH_YGRID_HEIGHT) / GRAPH_HEIGHT));
  const auto max_ygrid_val = yticks[0];

  // draw bars
  for (int hour=0, left_x = graph_margin_left;
       prices_it != prices_end;
       ++hour, ++prices_it, left_x += BAR_WIDTH)
    {
      float height_f = std::round(
        GRAPH_YGRID_HEIGHT * *prices_it / max_ygrid_val);
      ESP_LOGD("electricity_price_display", "hour %02d: height = %f",
               hour, height_f);
      int height = std::isfinite(height_f) ? height_f : 0;

      // draw bar
      if (height != 0) {
        int x0 = left_x + 1;
        int w = BAR_WIDTH - 1;

        int y0, h;
        if (height > 0) {
          y0 = GRAPH_HEIGHT - height;
          h = height;
        }
        else {  // height < 0
          y0 = GRAPH_HEIGHT,
            h = std::min(-height, graph_margin_bottom);
        }

        if (hour >= now.hour)
          it.filled_rectangle(
            x0, y0, w, h,
            hour == now.hour ? color_red : display::COLOR_ON);
        else
          // draw dithered gray rectangle
          for (int y = y0; y < y0+h; ++y)
            for (int x = x0; x < x0+w; ++x)
              if ((x & 1) ^ (y & 1))  // every 2nd pixel
                it.draw_pixel_at(x, y, display::COLOR_ON);
      }

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
    int x = graph_margin_left + hour*BAR_WIDTH;
    for (int y=0; y < GRAPH_HEIGHT; y += 3)
      it.draw_pixel_at(x, y);
    it.vertical_line(x, screen_height - graph_margin_bottom , 3);
    it.print(
      x, screen_height - graph_margin_bottom  + 4,
      font, TextAlign::TOP_CENTER,
      label);
  }

  // y-axis grid
  for (auto tick_val : yticks) {
    int y = GRAPH_HEIGHT -
      (GRAPH_YGRID_HEIGHT * tick_val) / max_ygrid_val;
    auto label_str = std::to_string(tick_val);
    const char* label = label_str.c_str();

    for (int x = graph_margin_left;
         x < GRAPH_WIDTH + graph_margin_left;
         x += 3)
      it.draw_pixel_at(x, y);
    it.horizontal_line(graph_margin_left - 4, y, 4);
    it.print(
      graph_margin_left - 4 - 1, y,
      font, TextAlign::CENTER_RIGHT,
      label);
    it.horizontal_line(graph_margin_left + GRAPH_WIDTH, y, 4);
    it.print(
      graph_margin_left + GRAPH_WIDTH + 4 + 1, y,
      font, TextAlign::CENTER_LEFT,
      label);
  }
  // gridline 0 without text or solid tick lines
  // (text would overlap with x-axis labels)
  for (int x = graph_margin_left - 4;
       x < GRAPH_WIDTH + graph_margin_left + 4;
       x += 3)
    it.draw_pixel_at(x, GRAPH_HEIGHT);
}
