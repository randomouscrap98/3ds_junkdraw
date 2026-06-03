#include "digits.h"

#include <string.h>

static struct SimpleLine _digits_raw[] = {
  // 0
  {.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 0}, // horizontal
  {.x1 = 0, .y1 = 4, .x2 = 2, .y2 = 4},
  {.x1 = 0, .y1 = 0, .x2 = 0, .y2 = 4}, // vertical
  {.x1 = 2, .y1 = 0, .x2 = 2, .y2 = 4},

  // 1
  {.x1 = 0, .y1 = 0, .x2 = 1, .y2 = 0}, // horizontal
  {.x1 = 0, .y1 = 4, .x2 = 2, .y2 = 4},
  {.x1 = 1, .y1 = 0, .x2 = 1, .y2 = 4}, // vertical

  // 2
  {.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 0}, // horizontal
  {.x1 = 0, .y1 = 2, .x2 = 2, .y2 = 2},
  {.x1 = 0, .y1 = 4, .x2 = 2, .y2 = 4},
  {.x1 = 2, .y1 = 0, .x2 = 2, .y2 = 2}, // vertical
  {.x1 = 0, .y1 = 2, .x2 = 0, .y2 = 4},

  // 3
  {.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 0}, // horizontal
  {.x1 = 0, .y1 = 4, .x2 = 2, .y2 = 4},
  {.x1 = 1, .y1 = 2, .x2 = 2, .y2 = 2},
  {.x1 = 2, .y1 = 0, .x2 = 2, .y2 = 4}, // vertical

  // 4
  {.x1 = 0, .y1 = 2, .x2 = 2, .y2 = 2}, // horizontal
  {.x1 = 2, .y1 = 0, .x2 = 2, .y2 = 4}, // vertical
  {.x1 = 0, .y1 = 0, .x2 = 0, .y2 = 2},

  // 5
  {.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 0}, // horizontal
  {.x1 = 0, .y1 = 2, .x2 = 2, .y2 = 2},
  {.x1 = 0, .y1 = 4, .x2 = 2, .y2 = 4},
  {.x1 = 0, .y1 = 0, .x2 = 0, .y2 = 2}, // vertical
  {.x1 = 2, .y1 = 2, .x2 = 2, .y2 = 4},

  // 6
  {.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 0}, // horizontal
  {.x1 = 0, .y1 = 2, .x2 = 2, .y2 = 2},
  {.x1 = 0, .y1 = 4, .x2 = 2, .y2 = 4},
  {.x1 = 0, .y1 = 0, .x2 = 0, .y2 = 4}, // vertical
  {.x1 = 2, .y1 = 2, .x2 = 2, .y2 = 4},

  // 7
  {.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 0}, // horizontal
  {.x1 = 2, .y1 = 0, .x2 = 2, .y2 = 4}, // vertical

  // 8
  {.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 0}, // horizontal
  {.x1 = 0, .y1 = 2, .x2 = 2, .y2 = 2},
  {.x1 = 0, .y1 = 4, .x2 = 2, .y2 = 4},
  {.x1 = 0, .y1 = 0, .x2 = 0, .y2 = 4}, // vertical
  {.x1 = 2, .y1 = 0, .x2 = 2, .y2 = 4},

  // 9
  {.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 0}, // horizontal
  {.x1 = 0, .y1 = 2, .x2 = 2, .y2 = 2},
  {.x1 = 2, .y1 = 0, .x2 = 2, .y2 = 4}, // vertical
  {.x1 = 0, .y1 = 0, .x2 = 0, .y2 = 2},
};

static struct SimpleLine * _digits[] = {
  _digits_raw,      // 0
  _digits_raw + 4,  // 1
  _digits_raw + 7,  // 2
  _digits_raw + 12,  // 3
  _digits_raw + 16,  // 4
  _digits_raw + 19,  // 5
  _digits_raw + 24,  // 6
  _digits_raw + 29,  // 7
  _digits_raw + 31,  // 8
  _digits_raw + 36,  // 9
  _digits_raw + 40   // END (use N + 1 to figure out # of lines)
};

// Push given string of digits as lines into linepackage.
int push_digits(const char * digits, struct LinePackage *package, u16 x, u16 y) {
  while(*digits) {
    int digit = (*digits) - '0';
    digits++;
    if(digit < 0 || digit > 9) {
      continue;
    }
    int linecount = _digits[digit + 1] - _digits[digit];
    // Do we have space for this digit?
    if(linecount > package->max_lines - package->line_count) {
      return 1;
    }
    // Copy all lines in, after modifying the x and y values
    for(int i = 0; i < linecount; i++) {
      package->lines[package->line_count].x1 = x + _digits[digit]->x1;
      package->lines[package->line_count].y1 = y + _digits[digit]->y1;
      package->lines[package->line_count].x2 = x + _digits[digit]->x2;
      package->lines[package->line_count].y2 = y + _digits[digit]->y2;
      package->line_count++;
    }
    //memcpy(package->lines + package->line_count, digits + digit, 
     //      linecount * sizeof(struct SimpleLine));
    x += 4; // move over
  }
  return 0;
}

