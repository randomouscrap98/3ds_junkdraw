#include "convert.h"

#include "draw.h"
#include "log.h"

#include <string.h>
#include <stdio.h>


char * convert_00_01(char * data_begin, char * data_end, size_t max_size) {
  // First, scan FORWARD and count the number of strokes. We also fix
  // any 2-byte page numbers in-place (the ones we care about are 1-byte).
  // We can't rely on existing scan functions because they are the new version. 
  char * scan = data_begin;
  u32 badstrokes = 0;
  u32 convstrokes = 0;

  if (*scan != ALIGNMENT_00) {
    LOGDBG("SCAN ERROR: OUT OF ALIGNMENT!, linear scanning for next stroke\n");
    char * tempptr = memchr(scan, ALIGNMENT_00, data_end - scan);
    if (tempptr == NULL) {
      LOGDBG(
          "SCAN ERROR: NO ALIGNMENT CHARS! EXITING\n");
      return NULL;
    } else {
      LOGDBG("SCAN SKIP: fast-forwarding %d characters to next alignment\n",
             tempptr - scan);
      scan = tempptr;
    }
  }

  u16 stroke_page;
  u8 varwidth;

  while (scan < data_end)
  {
    scan++; // skip alignment char

    // TODO: will crash if last character is the alignment char, or if there
    // just aren't enough characters to read up the page.
    stroke_page = varwidth_to_int(scan, &varwidth);
    if(varwidth > 2) {
      LOGDBG("PAGE TOO HIGH! CAN'T CONVERT!");
      return NULL;
    }
    else if(varwidth == 2) { // 2 is the new size, let's just write the new value
      int_to_chars(stroke_page, 2, scan); // FIXES 2-BYTE PAGE IN PLACE!
      convstrokes++;
    }
    else if(varwidth == 1) { // 2 is ok, 1 is NOT
      badstrokes++;
    }

    // Move scanptr to the next stroke, always
    scan = memchr(scan, ALIGNMENT_00, (data_end - scan));

    // If no more strokes are found, we're at the end anyway
    if (scan == NULL)
      break;
  }

  LOGDBG("%ld bad strokes, %ld modded", badstrokes, convstrokes);

  u32 shift = FHEADER_LEN_01 + badstrokes;
  char * new_end = data_end + shift;
  if((new_end - data_begin) > max_size) {
    LOGDBG("ERR: NEW FILE TOO LARGE");
    return NULL;
  }

  // Next, scan BACKWARDS through bytes, moving them "badstrokes" + header amount
  // forwards, then if we run into a stroke alignment, check the page (again) to
  // see if it's bad, and if it is, write the page back and decrement the shift amount
  scan = data_end;
  while(scan >= data_begin) {
    if(*scan == ALIGNMENT_00) {
      stroke_page = varwidth_to_int(scan + 1, &varwidth);
      if(varwidth == 1) {
        int_to_chars(stroke_page, DRAWDATA_PAGEBYTES, scan + 1);
        shift--;
        if(shift < FHEADER_LEN_01) {
          LOGDBG("PRGERR: BACKWARDS SCAN TOO SHIFTED");
          return NULL;
        }
      }
    }
    scan[shift] = scan[0];
    scan--;
  }

  // If everything is valid, write out the new header
  memset(data_begin, '_', FHEADER_LEN_01);
  sprintf(data_begin, MAGICSTRING_01 "01");

  return data_end;
}
