#include "datacontainer.h"
#include "utils.h"

#include <string.h>
// #include "3ds/allocator/linear.h"

// No limit to variable width scan (it's faster)
#define JDDC_NOVARIMAXSCAN

// ==========================================
//    INTERNAL CONVERSION FUNCTIONS
// ==========================================

// Container needs at least 'chars' amount of space to store this int
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

void fileheader_default(DataHeader * header) {
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
    LOGTRACE("DATA TOO SMALL, assuming last page 0");
    return 0; // Just safety
  }
  // Simple: start at one before the end and search backwards for the '.'
  for (char *pos = dc->start + length - 2; pos >= dc->start; pos--) {
    if (*pos == '.') {
      return chars_to_int(pos + 1, JDDC_PAGEBYTES);
    }
  }
  LOGTRACE("NO DATA FOUND, assuming last page 0");
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
  result.next_alignment = NULL;

  if (ds->current >= ds->parent->end) {
    LOGDBG("WARN: scanner run at or past end of data! Diff: %d\n",
           ds->parent->end - ds->current);
    result.next_alignment = ds->parent->end;
    return result;
  }

  // Perform a pre-check to realign ourselves if we're not aligned
  if (*ds->current != JDDC_ALIGNMENT) {
    LOGDBG("SCAN ERROR: OUT OF ALIGNMENT!, linear scanning for next stroke\n");
    char * tempptr = memchr(ds->current, JDDC_ALIGNMENT, ds->parent->end - ds->current);

    if (tempptr == NULL) {
      LOGDBG("SCAN ERROR: NO MORE ALIGNMENT CHARS! Skipping all the way to end\n");
      result.next_alignment = ds->parent->end;
      return result;
    } else {
      LOGDBG("SCAN SKIP: fast-forwarding %d characters to next alignment\n",
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
    // Skip the alignment character (TODO: assuming it's 1 byte)
    ds->current++;

    // TODO: will crash if last character is the alignment char, or if there
    // just aren't enough characters to read up the page.
    page_t stroke_page = JDDC_CHARS_TO_INT_2(ds->current);
    char * tempptr = ds->current + JDDC_PAGEBYTES; // tmpptr points at the stroke start

    // Move scanptr to the next stroke, always
    ds->current = memchr(ds->current, JDDC_ALIGNMENT, (ds->parent->end - ds->current));

    // If no more strokes are found, we're at the end
    if (ds->current == NULL)
      ds->current = ds->parent->end;

    if (stroke_page == ds->page || ds->page < 0) {
      result.stroke_start = tempptr;
      break;
    }
  }

  result.next_alignment = ds->current;
  return result;
}
