#pragma once

#include <vector>
#include <cmath>


static std::vector<int> pleasing_ticks(int max_val) {
  // Handle max_val <= 1 as a special case. We return ints, which
  // means we can't return fractional values, but there is little
  // point in supporting such small values anyway, so let's just use 1
  // as the only tick.
  if (max_val <= 1)
    return {1};

  float order_of_magnitude = std::floor(std::log10((float)max_val));
  int first_digit = std::ceil(max_val / std::pow(10, order_of_magnitude));
  int top = first_digit * std::pow(10, order_of_magnitude);
  ESP_LOGD("ticks", "first digit = %d  top = %d", first_digit, top);

  switch (first_digit) {
  case 1: case 10:  // 10 8 6 4 2
    // return {top, top/2};  // 10 5
  case 5:  // 5 4 3 2 1
    return {top, (top*4)/5, (top*3)/5, (top*2)/5, top/5};
  case 2:  // 2 1
    return {top, top/2};
  case 3:  // 3 2 1
  case 6:  // 6 4 2
    return {top, (top*2)/3, top/3};
  case 4:  // 4 3 2 1
  case 8:  // 8 6 4 2
    return {top, (top*3)/4, top/2, top/4};

  // These are inconvenient. Let's use some kind of uneven spacing.
  case 7:  // 7 5
    return {top, (top*5)/7};
  case 9:  // 9 5
    return {top, (top*5)/9};

  default:
    // we should never get here
    return {top};
  }
}
