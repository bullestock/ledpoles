/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Pixel structure
 */

#ifndef _pixel_hpp_
#define _pixel_hpp_

#include <stdint.h>

struct pixel {
  uint8_t g;
  uint8_t r;
  uint8_t b;
  uint8_t& operator[](int i)
  {
    switch (i) {
    case 0:
      return g;
    case 1:
      return r;
    default:
      return b;
    }
  }
};

#endif // _pixel_hpp_
