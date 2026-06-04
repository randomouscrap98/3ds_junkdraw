#include "draw.h"
#include "color.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

// JUST needed for M_PI??
#include <citro3d.h>

// 2026-05-23: UPDATED FOR VERSION 0.5 FILE VERSION 01
//             IT IS FULLY INCOMPATIBLE WITH PREVIOUS VERS

// ------- GENERAL UTILS ---------

#define DCV_NOVARIMAXSCAN


// ------------------------
// -- DATA CONVERT UTILS --
// ------------------------

// Container needs at least 'chars' amount of space to store this int
char *int_to_chars(u32 num, const u8 chars, char *container) {
  // WARN: clamping rather than ignoring! Hope this is ok
  num = DCV_CLAMP(num, 0, DCV_MAXVAL(chars));

  // Place each converted character, Little Endian
  for (int i = 0; i < chars; i++)
    container[i] = DCV_START + ((num >> (i * DCV_BITSPER)) & DCV_MAXVAL(1));

  // Return the next place you can start placing characters (so you can
  // continue reusing this function)
  return container + chars;
}

// Container should always be the start of where you want to read the int.
// The container should have at least count available. You can increment
// container by count afterwards (always)
u32 chars_to_int(const char *container, const u8 count) {
  u32 result = 0;
  for (int i = 0; i < count; i++)
    result += ((container[i] - DCV_START) << (i * DCV_BITSPER));
  return result;
}

// Some optimized reads
#define CHARS_TO_INT_1(container) ((container)[0] - DCV_START)
#define CHARS_TO_INT_2(container) (((container)[0] - DCV_START) + \
  (((container)[1] - DCV_START) << DCV_BITSPER))

// A dumb form of 2's compliment that doesn't carry the leading 1's
s32 special_to_signed(u32 special) {
  if (special & 1)
    return ((special >> 1) * -1) - 1;
  else
    return special >> 1;
}

u32 signed_to_special(s32 value) {
  if (value >= 0)
    return value << 1;
  else
    return ((value << 1) * -1) - 1;
}

// Container needs at LEAST 8 bytes free to store a variable width int
char *int_to_varwidth(u32 value, char *container) {
  u8 c = 0;
  u8 i = 0;

  do {
    c = value & DCV_VARIMAXVAL(1);
    value = value >> DCV_VARIBITSPER;
    if (value)
      c |= DCV_VARISTEP; // Continue on, set the uppermost bit
    container[i++] = DCV_START + c;
  } while (value);

  if (i >= DCV_VARIMAXSCAN)
    LOGDBG("WARN: variable width create too long: %d\n", i);

  // Return the NEXT place you can place values (just like the other func)
  return container + i;
}

// Read a variable width value from the given container. Stops if it goes too
// far though, which may give bad values
u32 varwidth_to_int(const char *container, u8 *read_count) {
  u8 c = 0;
  u8 i = 0;
  u32 result = 0;

  do {
    c = container[i] - DCV_START;
    result += (c & DCV_VARIMAXVAL(1)) << (DCV_VARIBITSPER * i);
    i++;
  }
#ifndef DCV_NOVARIMAXSCAN
  while (c & DCV_VARISTEP &&
         (i < DCV_VARIMAXSCAN)); // Keep going while the high bit is set
  if (i >= DCV_VARIMAXSCAN)
    LOGDBG("WARN: variable width read too long: %d\n", i);
#else
  while (c & DCV_VARISTEP); // Keep going while the high bit is set
#endif

  *read_count = i;

  return result;
}

// ------------------------
// -- SIMPLE LINE UTILS --
// ------------------------

void pixaligned_func(int16_t x0, int16_t y0, int16_t x1, int16_t y1, u16 width, 
                     u32 color, rectangle_func rect_f) {
  // Honestly, taken from wikipedia https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  float ofs = (width / 2.0f) - 0.5f;
  x0 = floor(x0 - ofs);
  x1 = floor(x1 - ofs);
  y0 = floor(y0 - ofs);
  y1 = floor(y1 - ofs);
  int16_t dx = abs(x1 - x0);
  int16_t sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0);
  int16_t sy = y0 < y1 ? 1 : -1;
  int16_t error = dx + dy;

  while(1) {
    (*rect_f)(x0, y0, width, color);
    int16_t e2 = 2 * error;
    if (e2 >= dy) {
      if (x0 == x1) break;
      error = error + dy;
      x0 = x0 + sx;
    }
    if (e2 <= dx) {
      if (y0 == y1) break;
      error = error + dx;
      y0 = y0 + sy;
    }
  }
}

inline void pixaligned_linefunc(const struct FullLine *line, rectangle_func rect_f) {
  pixaligned_func(
      line->x1, line->y1, line->x2, line->y2, line->width, line->color, rect_f);
}

// Draw the collection of lines given, starting at the given line and ending
// before the other given line (first inclusive, last exclusive)
// Assumes you're already on the appropriate page you want and all that
void pixaligned_linepackfunc(const struct LinePackage *linepack, u16 pack_start,
                             u16 pack_end, rectangle_func rect_f) {
  if (pack_end > linepack->line_count)
    pack_end = linepack->line_count;

  u32 color = rgba16_to_rgba32c(linepack->color);

  for (u16 i = pack_start; i < pack_end; i++) {
    pixaligned_func(
        linepack->lines[i].x1, linepack->lines[i].y1,
        linepack->lines[i].x2, linepack->lines[i].y2,
        linepack->width, color, rect_f);
  }
}

void init_linepackage(struct LinePackage *package) {
  // Keep this size a constant so that other implementors can have max size
  // guarantees
  package->lines = malloc(sizeof(struct SimpleLine) * MAX_STROKE_LINES);
  package->max_lines = MAX_STROKE_LINES;
}

void free_linepackage(struct LinePackage *package) { free(package->lines); }

void convert_to_fullline(const struct LinePackage *package, u16 line_index,
                         struct FullLine *result) {
  convert_to_fullline_precolor(
      package, line_index, rgba16_to_rgba32c(package->color), result);
}

void convert_to_fullline_precolor(const struct LinePackage *package, u16 line_index,
                                  u32 color, struct FullLine *result) {
  result->color = color;
  result->layer = package->layer;
  result->style = package->style;
  result->width = package->width;
  result->x1 = package->lines[line_index].x1;
  result->x2 = package->lines[line_index].x2;
  result->y1 = package->lines[line_index].y1;
  result->y2 = package->lines[line_index].y2;
}

// A true macro, as in just dump code into the function later. Used ONLY for
// convert_lines, hence "CVL"
#define CVL_LINECHECK                                                          \
  if (container_size - (ptr - container) < DCV_VARIMAXSCAN) {                  \
    LOGDBG("ERROR: ran out of space in line conversion: original size: %ld\n", \
           container_size);                                                    \
    return NULL;                                                               \
  }

// Convert lines into data, but if it doesn't fit, return a null pointer.
// Returned pointer points to end + 1 in container
char *convert_linepack_to_data(struct LinePackage *lines, char *container,
                               u32 container_size) {
  char *ptr = container;

  if (lines->line_count < 1) {
    LOGDBG("WARN: NO LINES TO CONVERT!\n");
    return NULL;
  }

  // This is a check that will need to be performed a lot
  CVL_LINECHECK

  // NOTE: the data is JUST the info for the lines, it's not "wrapped" into a
  // package or anything. So for instance, the length of the data is not
  // included at the start, as it may be stored in the final product

  // 1 byte style/layer, 1 byte width, 3 bytes color
  // 3 bits of line style, 1 bit (for now) of layers, 2 unused
  ptr = int_to_chars((lines->style & 0x7) | (lines->layer << 3), 1, ptr);
  // 6 bits of line width (minus 1)
  ptr = int_to_chars(lines->width - 1, 1, ptr);
  // 16 bits of color (2 unused)
  ptr = int_to_chars(lines->color, 3, ptr);

  CVL_LINECHECK

  // Now for strokes, we store the first point, then move along the rest of the
  // points doing an offset storage
  if (lines->style == LINESTYLE_STROKE) {
    // Dump first point, save point data for later
    u16 x = lines->lines[0].x1;
    u16 y = lines->lines[0].y1;
    ptr = int_to_chars(x, 2, ptr);
    ptr = int_to_chars(y, 2, ptr);

    // Now compute distances between this point and previous, store those as
    // variable width values. This can save a significant amount for most
    // types of drawing.
    for (u16 i = 0; i < lines->line_count; i++) {
      CVL_LINECHECK

      if (x == lines->lines[i].x2 && y == lines->lines[i].y2)
        continue; // Don't need to store stationary lines

      ptr = int_to_varwidth(signed_to_special(lines->lines[i].x2 - x), ptr);
      ptr = int_to_varwidth(signed_to_special(lines->lines[i].y2 - y), ptr);

      x = lines->lines[i].x2;
      y = lines->lines[i].y2;
    }
  } else if (lines->style == LINESTYLE_COLLECTION) {
    // A very simple storage: each line is just two points, stored
    // as-is with no variance. VERY fast
    for (u16 i = 0; i < lines->line_count; i++) {
      if (container_size - (ptr - container) < 8) {
        LOGDBG("ERROR: ran out of space in line conversion: original size: %ld\n",
               container_size);
        return NULL;
      }

      ptr = int_to_chars(lines->lines[i].x1, 2, ptr);
      ptr = int_to_chars(lines->lines[i].y1, 2, ptr);
      ptr = int_to_chars(lines->lines[i].x2, 2, ptr);
      ptr = int_to_chars(lines->lines[i].y2, 2, ptr);
    }
  } else {
    // We DON'T support this!
    LOGDBG("ERR: L2D UNSUPPORTED STROKE: %d\n", lines->style);
    return NULL;
  }

  return ptr;
}

// A true macro, as in just dump code into the function later. Used ONLY for
// convert_data, hence "CVD"
#define CVD_LINECHECK(x, msg)                                                  \
  if ((data_end - endptr) < x) {                                               \
    LOGDBG("ERROR: Not enough data to parse line! %s\n", msg);                 \
    return NULL;                                                               \
  }

// Package needs to have a 'lines' array already assigned with enough space to
// hold the largest stroke. Data needs to start precisely where you want it.
// Parsed data is all stored in the given package. A partial parse may result
// in a partially modified package. Converts a single chunk of line data.
// Returns the next position in the data to start reading a chunk
char *convert_data_to_linepack(struct LinePackage *package, char *data,
                               char *data_end) {
  char *endptr = data;
  package->line_count = 0;

  // If data is so short that it can't even parse the preamble, quit
  CVD_LINECHECK(5, "PREAMBLE")

  u32 temp = CHARS_TO_INT_1(endptr);
  package->style = temp & 0x7;
  package->layer = (temp >> 3) & 0x1;
  package->width = CHARS_TO_INT_1(endptr + 1) + 1;
  package->color = chars_to_int(endptr + 2, 3);
  endptr += 5;

  if (package->style == LINESTYLE_STROKE) {
    CVD_LINECHECK(4, "STROKE FIRST POINT")

    // First point is regular simple 4 byte data point.
    u16 x = CHARS_TO_INT_2(endptr);
    u16 y = CHARS_TO_INT_2(endptr + 2);
    endptr += 4;

    u8 scanned = 0;

    // This could be VERY VERY UNSAFE!!
    while (endptr < data_end) {
      struct SimpleLine *line = package->lines + package->line_count;

      // Store current end as first point
      line->x1 = x;
      line->y1 = y;
      // Read next endpoint
      x = x + special_to_signed(varwidth_to_int(endptr, &scanned));
      endptr += scanned;
      y = y + special_to_signed(varwidth_to_int(endptr, &scanned));
      endptr += scanned;
      // The end of us is the next endpoint
      line->x2 = x;
      line->y2 = y;

      // We added another line
      package->line_count++;

      if (package->line_count > package->max_lines) {
        LOGDBG("ERR: got a stroke that's too long!");
        return NULL;
      }
    }

    // The special case where there's no additional strokes
    if (package->line_count == 0) {
      package->lines[0].x1 = package->lines[0].x2 = x;
      package->lines[0].y1 = package->lines[0].y2 = y;
      package->line_count++;
    }
  } else if (package->style == LINESTYLE_COLLECTION) {
    while (endptr < data_end) {
      CVD_LINECHECK(8, "COLLECTION: LINE")
      struct SimpleLine *line = package->lines + package->line_count;
      line->x1 = CHARS_TO_INT_2(endptr);
      line->y1 = CHARS_TO_INT_2(endptr + 2);
      line->x2 = CHARS_TO_INT_2(endptr + 4);
      line->y2 = CHARS_TO_INT_2(endptr + 6);
      endptr += 8;
      package->line_count++;
      if (package->line_count > package->max_lines) {
        LOGDBG("ERR: got a stroke that's too long!");
        return NULL;
      }
    }
  } else {
    // We DON'T support this!
    LOGDBG("ERR: D2L UNSUPPORTED STROKE: %d\n", package->style);
    return NULL;
  }

  return endptr;
}

// WARN: this is not the last page WITH MARKS, it is the last page that the
// user touched. This is VERY DIFFERENT!!!
u32 last_used_page(char *data, u32 length) {
  if (length < 2) {
    LOGTRACE("DATA TOO SMALL, assuming last page 0");
    return 0; // Just safety
  }
  // Simple: start at one before the end and search backwards for the '.'
  for (char *pos = data + length - 2; pos >= data; pos--) {
    if (*pos == '.') {
      return chars_to_int(pos + 1, DRAWDATA_PAGEBYTES);
    }
  }
  LOGTRACE("NO DATA FOUND, assuming last page 0");
  return 0;
}

// WARN: This is the last page that has anything on it! It's slower!!
u32 last_total_page(char *data, char * data_end) {
  // if (data_end - data < 2) {
  //   LOGDBG("DATA TOO SMALL, assuming last page 0");
  //   return 0; // Just safety
  // }
  u16 maxpage = 0;
  char * ptr = data;
  while((ptr = memchr(ptr, DRAWDATA_ALIGNMENT, data_end - ptr))) {
    ptr++;
    u16 page = chars_to_int(ptr, DRAWDATA_PAGEBYTES);
    if(page > maxpage) {
      maxpage = page;
    }
  }
  return maxpage;
}

char *write_to_datamem(char *stroke_data, char *stroke_end, u16 page, char *mem,
                       char *mem_end) {
  u32 stroke_size = stroke_end - stroke_data;
  u32 test_size = 1 + DRAWDATA_PAGEBYTES + stroke_size;
  u32 mem_free = MAX_DRAW_DATA - (mem_end - mem);
  char *new_end = mem_end;

  // Oops, the size of the data is more than the leftover space in draw_data!
  if (test_size > mem_free) {
    LOGDBG("ERR: Couldn't store lines! Required: %ld, has: %ld\n", test_size,
           mem_free);
  } else {
    // FIRST, write the stroke identifier
    *new_end = DRAWDATA_ALIGNMENT;
    new_end++;

    // Next, store the page
    new_end = int_to_chars(page, DRAWDATA_PAGEBYTES, new_end);

    // Finally, memcopy the whole stroke chunk
    memcpy(new_end, stroke_data, sizeof(char) * stroke_size);
    new_end += stroke_size;

    // no need to free or anything, we're reusing the stroke buffer
#ifdef DEBUG_DATAPRINT
    *new_end = '\0'; // Will this be an issue??
    printf("\x1b[20;1HS: %-5ld MF: %-7ld\n%-300.300s", stroke_size, mem_free,
           mem_end);
#endif
  }

  return new_end;
}

// Scan through memory until either we reach the end, the max scan is reached,
// or we actually find the first occurence of a stroke on a page that we want.
// Return the place where we stopped scanning (so you can call it repeatedly).
// Doesn't actually parse the stroke, just looks for the next one.
char *datamem_scanstroke(char *start, char *end, const u32 max_scan,
                         const u16 page, char **stroke_start) {
  char *tempptr;
  char *scanptr = start;
  *stroke_start = NULL;

  if (scanptr >= end) {
    LOGDBG("WARN: called scanstroke at or past end of data! Diff: %d\n",
           end - scanptr);
    return end;
  }

  // Perform a pre-check to realign ourselves if we're not aligned
  if (*scanptr != DRAWDATA_ALIGNMENT) {
    LOGDBG("SCAN ERROR: OUT OF ALIGNMENT!, linear scanning for next stroke\n");
    tempptr = memchr(scanptr, DRAWDATA_ALIGNMENT, end - scanptr);

    if (tempptr == NULL) {
      LOGDBG(
          "SCAN ERROR: NO MORE ALIGNMENT CHARS! Skipping all the way to end\n");
      return end;
    } else {
      LOGDBG("SCAN SKIP: fast-forwarding %d characters to next alignment\n",
             tempptr - scanptr);
      scanptr = tempptr;
    }
  }

  u16 stroke_page;
  char *stop = scanptr + DCV_MIN(end - scanptr, max_scan);

  while (scanptr < stop)
  {
    // Skip the alignment character (TODO: assuming it's 1 byte)
    scanptr++;

    // TODO: will crash if last character is the alignment char, or if there
    // just aren't enough characters to read up the page.
    stroke_page = CHARS_TO_INT_2(scanptr);
    tempptr = scanptr + DRAWDATA_PAGEBYTES; // tmpptr points at the stroke start

    // Move scanptr to the next stroke, always
    scanptr = memchr(scanptr, DRAWDATA_ALIGNMENT, (end - scanptr));

    // If no more strokes are found, we're at the end
    if (scanptr == NULL)
      scanptr = end;

    if (stroke_page == page) {
      *stroke_start = tempptr;
      break;
    }
  }

  return scanptr;
}

// Copy the entire contents of given page to the given destination page. Returns the new end
char * copy_page(char * start, char * end, const u16 sourcepage, const u16 destpage) {
  char * stroke = NULL;
  char * new_end = end;
  char * ptr = datamem_scanstroke(start, end, MAX_DRAW_DATA, sourcepage, &stroke);
  u32 numcopied = 0;

  // Scan through, find strokes on given page (only up to old end)
  while(stroke) {
    // Write new stroke at end. ptr is pointing at NEXT stroke, and stroke is past
    // the metadata (which we add back in)
    char * real_start = stroke - (1 + DRAWDATA_PAGEBYTES); // Move stroke back to get meta
    u32 len = (ptr - real_start); // length includes meta
    if((new_end + len) - start > MAX_DRAW_DATA) {
      LOGDBG("Not enough space to copy page!");
      return end;
    }
    memcpy(new_end, real_start, len); // copy WHOLE stroke
    int_to_chars(destpage, 2, new_end + 1); // overwrite page with given
    new_end += len;
    numcopied++;
    ptr = datamem_scanstroke(ptr, end, MAX_DRAW_DATA, sourcepage, &stroke);
  }

  LOGDBG("Copied %ld strokes from pg %d to %d", numcopied, sourcepage + 1, destpage + 1);

  return new_end;
}

// Swap the entire contents of the given two pages
void swap_pages(char * start, char * end, const u16 sourcepage, const u16 destpage) {
  char * stroke = NULL;
  char * ptr;
  u32 numtouched = 0;

  if(sourcepage == destpage) {
    LOGDBG("WARN: skipping useless self-swap");
    return;
  }

  // First, set all pages in sourcepage to a non-valid temp page. 12 bits = 4096,
  // just assume we've reserved some set of the last for this
  ptr = datamem_scanstroke(start, end, MAX_DRAW_DATA, sourcepage, &stroke);
  while(stroke) {
    int_to_chars(RSV_PAGE_TMP, DRAWDATA_PAGEBYTES, stroke - DRAWDATA_PAGEBYTES);
    numtouched++;
    if(ptr >= end) break;
    ptr = datamem_scanstroke(ptr, end, MAX_DRAW_DATA, sourcepage, &stroke);
  }

  // Now, reset and update the destpages to sourcepage
  ptr = datamem_scanstroke(start, end, MAX_DRAW_DATA, destpage, &stroke);
  while(stroke) {
    int_to_chars(sourcepage, DRAWDATA_PAGEBYTES, stroke - DRAWDATA_PAGEBYTES);
    numtouched++;
    if(ptr >= end) break;
    ptr = datamem_scanstroke(ptr, end, MAX_DRAW_DATA, destpage, &stroke);
  }

  // Then go back to old source and write with dest
  ptr = datamem_scanstroke(start, end, MAX_DRAW_DATA, RSV_PAGE_TMP, &stroke);
  while(stroke) {
    int_to_chars(destpage, DRAWDATA_PAGEBYTES, stroke - DRAWDATA_PAGEBYTES);
    numtouched++;
    if(ptr >= end) break;
    ptr = datamem_scanstroke(ptr, end, MAX_DRAW_DATA, RSV_PAGE_TMP, &stroke);
  }

  LOGDBG("Swapped %ld strokes between pg %d and %d", numtouched, sourcepage + 1, destpage + 1);
}

char * delete_page(char * start, char * end, const u16 page) {
  char * scan = memchr(start, DRAWDATA_ALIGNMENT, end - start);
  u32 reclaim = 0;
  u32 delstrokes = 0;

  if(scan != start) {
    LOGDBG("PRGERR: DID NOT BEGIN DELETE AT START");
  }

  // Linear scan looking for alignment char. If page matches, add length to 
  while (scan && scan < end) {
    // Peek page; if next is delete, do something special
    if(*scan == DRAWDATA_ALIGNMENT && chars_to_int(scan + 1, DRAWDATA_PAGEBYTES) == page) {
      char * next = memchr(scan + 1, DRAWDATA_ALIGNMENT, end - scan - 1);
      if(next == NULL) {
        next = end;
      }
      reclaim += (next - scan);
      scan = next;
      delstrokes++;
    }
    else {
      if(reclaim > 0) { // This is a normal scan, just move it backwards
        *(scan - reclaim) = *scan;
      }
      scan++;
    }
  }

  char * new_end = end - reclaim;

  // Now that the page is deleted and data moved over, we need to shift
  // over the page metadata
  u16 lastpage = last_total_page(start, new_end);
  LOGTRACE("Last page: %d vs this: %d (%ld)", lastpage, page, new_end - start);

  for(u16 shiftpage = page; shiftpage < lastpage; shiftpage++) {
    // Scan for the next page, shift it back into the hole, it leaves a new hole
    char * stroke;
    scan = datamem_scanstroke(start, new_end, MAX_DRAW_DATA, shiftpage + 1, &stroke);
    while(stroke) {
      int_to_chars(shiftpage, DRAWDATA_PAGEBYTES, stroke - DRAWDATA_PAGEBYTES);
      if(scan >= new_end) break;
      scan = datamem_scanstroke(scan, new_end, MAX_DRAW_DATA, shiftpage + 1, &stroke);
    }
  }

  LOGDBG("Deleted %ld strokes for pg %d", delstrokes, page + 1);
  
  return new_end;
}

// Move given page into the slot occupied by destpage, pushing all pages to the right
// (including destpage). This is effectively removing sourcepage and inserting it to
// the left of destpage
void move_page(char * start, char * end, const u16 sourcepage, const u16 destpage) {
  char * stroke;
  char * ptr;
  u32 numtouched = 0;

  if(sourcepage == destpage) {
    LOGDBG("WARN: skipping useless self-swap");
    return;
  }

  int direction = sourcepage < destpage ? 1 : -1;

  // First, set all pages in sourcepage to a non-valid temp page. 12 bits = 4096,
  // just assume we've reserved some set of the last for this
  ptr = datamem_scanstroke(start, end, MAX_DRAW_DATA, sourcepage, &stroke);
  while(stroke) {
    int_to_chars(RSV_PAGE_TMP, DRAWDATA_PAGEBYTES, stroke - DRAWDATA_PAGEBYTES);
    numtouched++;
    if(ptr >= end) break;
    ptr = datamem_scanstroke(ptr, end, MAX_DRAW_DATA, sourcepage, &stroke);
  }

  // Move from source page to destpage, shifting pages to fill holes
  for(u16 page = sourcepage; page != destpage; page += direction) {
    ptr = datamem_scanstroke(start, end, MAX_DRAW_DATA, page + direction, &stroke);
    while(stroke) {
      int_to_chars(page, DRAWDATA_PAGEBYTES, stroke - DRAWDATA_PAGEBYTES);
      numtouched++;
      if(ptr >= end) break;
      ptr = datamem_scanstroke(ptr, end, MAX_DRAW_DATA, page + direction, &stroke);
    }
  }

  // Then go back to old source and write with dest
  ptr = datamem_scanstroke(start, end, MAX_DRAW_DATA, RSV_PAGE_TMP, &stroke);
  while(stroke) {
    int_to_chars(destpage, DRAWDATA_PAGEBYTES, stroke - DRAWDATA_PAGEBYTES);
    numtouched++;
    if(ptr >= end) break;
    ptr = datamem_scanstroke(ptr, end, MAX_DRAW_DATA, RSV_PAGE_TMP, &stroke);
  }

  LOGDBG("Swapped %ld strokes between pg %d and %d", numtouched, sourcepage + 1, destpage + 1);
}
