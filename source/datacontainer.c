#include "datacontainer.h"
#include "utils.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

// No limit to variable width scan (it's faster)
#define JDDC_NOVARIMAXSCAN


// ==========================================
//    INTERNAL CONVERSION FUNCTIONS
// ==========================================

// Some optimized reads
#define JDDC_CHARS_TO_INT_1(container) ((container)[0] - JDDC_START)
#define JDDC_CHARS_TO_INT_2(container) (((container)[0] - JDDC_START) + \
  (((container)[1] - JDDC_START) << JDDC_BITSPER))


// Returns the new end. Container needs at least 'chars' amount of space to store this int
static inline char *int_to_chars(u32 num, const u8 chars, char *container) {
  // WARN: clamping rather than ignoring! Hope this is ok
  num = JD_CLAMP(num, 0, JDDC_MAXVAL(chars));

  // Place each converted character, Little Endian
  for (int i = 0; i < chars; i++)
    container[i] = JDDC_START + ((num >> (i * JDDC_BITSPER)) & JDDC_MAXVAL(1));

  // Return the next place you can start placing characters (so you can
  // continue reusing this function)
  return container + chars;
}

// Container should always be the start of where you want to read the int.
// The container should have at least count available. You can increment
// container by count afterwards (always)
static inline u32 chars_to_int(const char *container, const u8 count) {
  u32 result = 0;
  for (int i = 0; i < count; i++)
    result += ((container[i] - JDDC_START) << (i * JDDC_BITSPER));
  return result;
}

// A dumb form of 2's compliment that doesn't carry the leading 1's
static inline s32 special_to_signed(u32 special) {
  if (special & 1)
    return ((special >> 1) * -1) - 1;
  else
    return special >> 1;
}

static inline u32 signed_to_special(s32 value) {
  if (value >= 0)
    return value << 1;
  else
    return ((value << 1) * -1) - 1;
}

// Container needs at LEAST 8 bytes free to store a variable width int
static inline char *int_to_varwidth(u32 value, char *container) {
  u8 c = 0;
  u8 i = 0;

  do {
    c = value & JDDC_VARIMAXVAL(1);
    value = value >> JDDC_VARIBITSPER;
    if (value)
      c |= JDDC_VARISTEP; // Continue on, set the uppermost bit
    container[i++] = JDDC_START + c;
  } while (value);

#ifndef JDDC_NOVARIMAXSCAN
  if (i >= JDDC_VARIMAXSCAN)
    LOGDBG("WARN: variable width create too long: %d\n", i);
#endif

  // Return the NEXT place you can place values (just like the other func)
  return container + i;
}

// Read a variable width value from the given container. Stops if it goes too
// far though, which may give bad values
static inline u32 varwidth_to_int(const char *container, u8 *read_count) {
  u8 c = 0;
  u8 i = 0;
  u32 result = 0;

  do {
    c = container[i] - JDDC_START;
    result += (c & JDDC_VARIMAXVAL(1)) << (JDDC_VARIBITSPER * i);
    i++;
  }
#ifndef JDDC_NOVARIMAXSCAN
  // Keep going while the high bit is set
  while (c & JDDC_VARISTEP && (i < JDDC_VARIMAXSCAN)); 
  if (i >= JDDC_VARIMAXSCAN)
    LOGDBG("WARN: variable width read too long: %d\n", i); 
#else
  while (c & JDDC_VARISTEP); // Keep going while the high bit is set
#endif

  *read_count = i;

  return result;
}


// ==========================================
//            DATA CONTAINER
// ==========================================

int datacontainer_init(DataContainer * dc, size_t capacity) {
  dc->capacity = capacity;
  dc->sequence = 0;
  dc->container = linearAlloc(capacity);
  if(!dc->container) {
    return 1;
  }
  dc->start = dc->end = dc->container + JDDC_FHEADER_LEN;
  // Just in case the user forgets: let's assign a default header.
  DataHeader dh;
  dataheader_default(&dh);
  datacontainer_setheader(dc, &dh);
  // Maybe not necessary but yeah...
  dc->start[0] = 0;
  return 0;
}

void datacontainer_free(DataContainer * dc) {
  linearFree(dc->container);
}

size_t datacontainer_length(DataContainer * dc) {
  return dc->end - dc->start;
}

void dataheader_default(DataHeader * header) {
  // No need to set these as constants; if you want the defaults,
  // just call this function
  header->resolution_id = 0;
  header->layer_count = 2;
  header->onion_count = 0;
  header->bgcolor = 0xFFFF;
}

void datacontainer_setheader(DataContainer * dc, DataHeader * dh) {
  // Dump a default header with no metadata set
  memcpy(dc->container, JDDC_FHEADER_BASE, JDDC_FHEADER_LEN);
  // Now we can write the metadata into the empty space
  u8 byte10 = (dh->resolution_id & 0x3) | (((dh->layer_count - 1) & 0x7) << 2);
  u8 byte11 = (dh->onion_count & 0x7);
  u16 byte12_14 = dh->bgcolor;
  int_to_chars(byte10, 1, dc->container + 10);
  int_to_chars(byte11, 1, dc->container + 11);
  int_to_chars(byte12_14, 3, dc->container + 12);
}

void datacontainer_getheader(DataContainer * dc, DataHeader * dh) {
  dataheader_default(dh);
  if(dc->container[10] != '_') {
    u8 byte10 = JDDC_CHARS_TO_INT_1(dc->container + 10);
    dh->resolution_id = byte10 & 0x3;
    dh->layer_count = 1 + ((byte10 >> 2) & 0x7);
  }
  if(dc->container[11] != '_') {
    u8 byte11 = JDDC_CHARS_TO_INT_1(dc->container + 11);
    dh->onion_count = byte11 & 0x7;
  }
  if(dc->container[12] != '_' && dc->container[13] != '_' && 
     dc->container[14] != '_') {
    dh->bgcolor = chars_to_int(dc->container + 12, 3);
  }
}

// WARN: this is not the last page WITH MARKS, it is the last page that the
// user touched. This is VERY DIFFERENT!!!
page_t datacontainer_last_used_page(DataContainer * dc) {
  size_t length = datacontainer_length(dc);
  if (length < 2) {
    LOGTRC("DATA TOO SMALL, assuming last page 0");
    return 0; // Just safety
  }
  // Simple: start at one before the end and search backwards for the '.'
  for (char *pos = dc->start + length - 2; pos >= dc->start; pos--) {
    if (*pos == '.') {
      return chars_to_int(pos + 1, JDDC_PAGEBYTES);
    }
  }
  LOGTRC("NO DATA FOUND, assuming last page 0");
  return 0;
}

// WARN: This is the last page that has anything on it! It's slower!!
page_t datacontainer_last_total_page(DataContainer * dc) {
  page_t maxpage = 0;
  char * ptr = dc->start;
  while((ptr = memchr(ptr, JDDC_ALIGNMENT, dc->end - ptr))) {
    ptr++;
    page_t page = chars_to_int(ptr, JDDC_PAGEBYTES);
    if(page > maxpage) {
      maxpage = page;
    }
  }
  return maxpage;
}

int datacontainer_enough(DataContainer * dc, size_t added_space) {
  return dc->capacity - (dc->end - dc->container) >= added_space;
}

// Get the TOTAL fill, including the header
size_t datacontainer_filled(DataContainer * dc) {
  return dc->end - dc->container;
}

// ==========================================
//              LINE CONTAINER
// ==========================================

int linecontainer_init_stroke(LineContainer * lc) {
  memset(lc, 0, sizeof(LineContainer));
  lc->capacity = JDDC_MAXSTROKELINES;
  lc->lines = malloc(sizeof(LineSegment) * lc->capacity);
  if(!lc->lines) {
    return 1;
  }
  return 0;
}

void linecontainer_free(LineContainer * lc) {
  free(lc->lines);
}

#define JDDC_LINECHECK(dc, v) { \
  if (!datacontainer_enough(dc, (v))) { \
    LOGERR("OUT OF SPACE, can't store stroke data!\n"); \
    return 1; \
  } \
}

// Internal function to store the line header. If it fails, dc->end is left in a shifted state
static inline int datacontainer_addline_header(DataContainer * dc, LineContainer * lc) {
  JDDC_LINECHECK(dc, JDDC_STROKEHEADERBYTES);

  *dc->end = JDDC_ALIGNMENT; 
  dc->end += 1;
  // 1 byte style/layer, 1 byte width, 3 bytes color
  // 3 bits of line style, 3 bits of layers
  // 6 bits of line width (minus 1)
  // 16 bits of color (2 unused)
  dc->end = int_to_chars(lc->page, JDDC_PAGEBYTES, dc->end); 
  dc->end = int_to_chars((lc->style & 0x7) | (lc->layer << 3), 1, dc->end);
  dc->end = int_to_chars(lc->width - 1, 1, dc->end);
  dc->end = int_to_chars(lc->color, 3, dc->end);

  return 0;
}

// Internal function to store the stroke portion of the line. If it fails,
// dc->end is left in a shifted state.
static inline int datacontainer_addline_stroke(DataContainer * dc, LineContainer * lc) {
  // This is a check that will need to be performed a lot

  // Now for strokes, we store the first point, then move along the rest of the
  // points doing an offset storage
  if (lc->style == JDDC_LINESTYLE_STROKE) {
    JDDC_LINECHECK(dc, 2 * JDDC_COORDBYTES);

    // Dump first point, save point data for later
    coord_t x = lc->lines[0].x1;
    coord_t y = lc->lines[0].y1;
    dc->end = int_to_chars(x, JDDC_COORDBYTES, dc->end);
    dc->end = int_to_chars(y, JDDC_COORDBYTES, dc->end);

    // Now compute distances between this point and previous, store those as
    // variable width values. This can save a significant amount for most
    // types of drawing.
    for (lineidx_t i = 0; i < lc->length; i++) {
      if (x == lc->lines[i].x2 && y == lc->lines[i].y2)
        continue; // Don't need to store stationary lines
      JDDC_LINECHECK(dc, JDDC_VARIMAXSCAN);
      dc->end = int_to_varwidth(signed_to_special(lc->lines[i].x2 - x), dc->end);
      dc->end = int_to_varwidth(signed_to_special(lc->lines[i].y2 - y), dc->end);
      x = lc->lines[i].x2;
      y = lc->lines[i].y2;
    }
  } else if (lc->style == JDDC_LINESTYLE_COLLECTION) {
    // A very simple storage: each line is just two points, stored
    // as-is with no variance. VERY fast
    for (lineidx_t i = 0; i < lc->length; i++) {
      JDDC_LINECHECK(dc, JDDC_COORDBYTES * 4);
      dc->end = int_to_chars(lc->lines[i].x1, JDDC_COORDBYTES, dc->end);
      dc->end = int_to_chars(lc->lines[i].y1, JDDC_COORDBYTES, dc->end);
      dc->end = int_to_chars(lc->lines[i].x2, JDDC_COORDBYTES, dc->end);
      dc->end = int_to_chars(lc->lines[i].y2, JDDC_COORDBYTES, dc->end);
    }
  } else {
    // We DON'T support this!
    LOGERR("L2D UNSUPPORTED STROKE: %d\n", lc->style);
    return 1;
  }

  return 0;
}

int datacontainer_addline(DataContainer * dc, LineContainer * lc) {
  // Just write data into the container until we run out, it's ok to leave stuff behind.
  // Might waste a little time but no worries.
  char * prev_end = dc->end;

  if (lc->length < 1) {
    LOGWRN("NO LINES TO STORE!\n");
    return 1;
  }
  
  if(datacontainer_addline_header(dc, lc) || 
     datacontainer_addline_stroke(dc, lc)) {
    dc->end = prev_end;
    return 1;
  }

  return 0;
}

// ==========================================
//              DATA SCANNER
// ==========================================

DataScanner datacontainer_get_scanner(DataContainer * dc) {
  DataScanner out;
  out.sequence = dc->sequence;
  out.parent = dc;
  out.current = dc->start;
  // Optional limitations on scanner
  out.page = -1;      // Scan for all pages
  out.max_scan = -1;  // No limit
  // out.layer = -1;     // Scan for all layers
  return out;
}

// Scan until either we reach the end, the max scan is reached,
// or we actually find the first occurence of a stroke on a page that we want.
// Doesn't actually parse the stroke, just looks for the next one. 
// result.stroke_start will be null if no stroke is found.
DataScannerResult datascanner_next(DataScanner * ds) {
  // First, see if we're invalidated. If so, reset scanner.
  if(ds->sequence != ds->parent->sequence) {
    ds->sequence = ds->parent->sequence;
    ds->current = ds->parent->start;
  }

  DataScannerResult result;
  result.stroke_start = NULL;
  result.data_start = NULL;
  result.data_end = NULL;
  result.page = -1;

  if (ds->current >= ds->parent->end) {
    LOGWRN("Scanner run at or past end of data! Diff: %d\n",
           ds->parent->end - ds->current);
    result.data_end = ds->parent->end;
    return result;
  }

  // Perform a pre-check to realign ourselves if we're not aligned
  if (*ds->current != JDDC_ALIGNMENT) {
    LOGWRN("SCAN OUT OF ALIGNMENT!, linear scanning for next stroke\n");
    char * tempptr = memchr(ds->current, JDDC_ALIGNMENT, ds->parent->end - ds->current);

    if (tempptr == NULL) {
      LOGWRN("SCAN: NO MORE ALIGNMENT CHARS! Skipping all the way to end\n");
      result.data_end = ds->parent->end;
      return result;
    } else {
      LOGWRN("SCAN: fast-forwarding %d characters to next alignment\n",
             tempptr - ds->current);
      ds->current = tempptr;
    }
  }

  // Stop is either the end (with no limit) or up to the limit or end
  char *stop = ds->max_scan < 0 ? 
    ds->parent->end : 
    ds->current + JD_MIN(ds->parent->end - ds->current, ds->max_scan);

  while (ds->current < stop)
  {
    // Include the alignment character
    result.data_start = ds->current;

    // Skip the alignment character (TODO: assuming it's 1 byte)
    ds->current++;

    // TODO: will crash if last character is the alignment char, or if there
    // just aren't enough characters to read up the page.
    result.page = JDDC_CHARS_TO_INT_2(ds->current);
    char * tempptr = ds->current + JDDC_PAGEBYTES; // tmpptr points at the stroke start

    // Move scanptr to the next stroke, always
    ds->current = memchr(ds->current, JDDC_ALIGNMENT, (ds->parent->end - ds->current));

    // If no more strokes are found, we're at the end
    if (ds->current == NULL) {
      ds->current = ds->parent->end;
    }

    if (result.page == ds->page || ds->page < 0) {
      result.stroke_start = tempptr;
      break;
    }
  }

  result.data_end = ds->current;
  return result;
}

// Useful for loop: scan while strokes are found. Does not throw errors if
// ds is already past the end.
int datascanner_next_loop(DataScanner * ds, DataScannerResult * dsr) {
  if(datascanner_at_end(ds)) return 0;
  *dsr = datascanner_next(ds);
  return dsr->stroke_start != NULL;
}

int datascanner_at_end(DataScanner * ds) {
  return ds->current >= ds->parent->end;
}

void datascannerresult_overwritepage(DataScannerResult * dsr, page_t page) {
  int_to_chars(page, JDDC_PAGEBYTES, dsr->data_start + 1);
}

// A true macro, as in just dump code into the function later. Used ONLY for
// convert_data, hence "CVD"
#define JDDC_READCHECK(dsr, endptr, x, msg) {                               \
  if ((dsr->data_end - endptr) < x) {                                       \
    LOGDBG("ERROR: Not enough data to parse line! %s\n", msg);              \
    return 1;                                                               \
  } \
}

// Use this like it's going to return a value and check to see if you have space
#define JDDC_NEWLINE_OK(lc) \
  lc->lines + lc->length; \
  if (lc->length >= lc->capacity) { \
    LOGDBG("ERR: got a stroke that's too long!"); \
    return 1; \
  } \
  lc->length++; 


int datascannerresult_parseline(DataScannerResult * dsr, LineContainer * lc) {
  lc->length = 0;
  char * endptr = dsr->stroke_start;

  JDDC_READCHECK(dsr, endptr, JDDC_PREAMBLEBYTES, "PREAMBLE");

  // Read the preamble
  u32 temp = JDDC_CHARS_TO_INT_1(endptr);
  lc->style = temp & 0x7;
  lc->layer = (temp >> 3) & 0x7;
  lc->width = JDDC_CHARS_TO_INT_1(endptr + 1) + 1;
  lc->color = chars_to_int(endptr + 2, 3);
  endptr += JDDC_PREAMBLEBYTES;

  if (lc->style == JDDC_LINESTYLE_STROKE) {
    JDDC_READCHECK(dsr, endptr, 2 * JDDC_COORDBYTES, "STROKE FIRST POINT");

    // First point is regular simple 4 byte data point.
    coord_t x = JDDC_CHARS_TO_INT_2(endptr);
    coord_t y = JDDC_CHARS_TO_INT_2(endptr + JDDC_COORDBYTES);
    endptr += 2 * JDDC_COORDBYTES;

    u8 scanned = 0;

    while (endptr < dsr->data_end) {
      LineSegment *line = JDDC_NEWLINE_OK(lc);
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
    }

    // The special case where there's no additional strokes
    if (lc->length == 0) {
      lc->lines[0].x1 = lc->lines[0].x2 = x;
      lc->lines[0].y1 = lc->lines[0].y2 = y;
      lc->length++;
    }
  } else if (lc->style == JDDC_LINESTYLE_COLLECTION) {
    while (endptr < dsr->data_end) {
      JDDC_READCHECK(dsr, endptr, 8, "COLLECTION: LINE")
      LineSegment *line = JDDC_NEWLINE_OK(lc);
      line->x1 = JDDC_CHARS_TO_INT_2(endptr);
      line->y1 = JDDC_CHARS_TO_INT_2(endptr + JDDC_COORDBYTES);
      line->x2 = JDDC_CHARS_TO_INT_2(endptr + JDDC_COORDBYTES * 2);
      line->y2 = JDDC_CHARS_TO_INT_2(endptr + JDDC_COORDBYTES * 3);
      endptr += JDDC_COORDBYTES * 4;
    }
  } else {
    // We DON'T support this!
    LOGDBG("ERR: D2L UNSUPPORTED STROKE: %d\n", lc->style);
    return 1;
  }

  return 0;
}

