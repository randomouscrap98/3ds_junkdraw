#include "edit.h"
#include "utils.h"
#include "datacontainer.h"

#include <string.h>

// Copy the entire contents of given page to the given destination page. Returns the new end
int edit_copy_page(DataContainer * dc, const page_t sourcepage, const page_t destpage) {

  if(sourcepage == destpage) {
    LOGDBG("WARN: skipping useless copy");
    return 1;
  }

  DataScannerResult result;
  DataScanner ds = datacontainer_get_scanner(dc);
  ds.page = sourcepage;
  u32 numcopied = 0;
  char * oldend = dc->end;

  // Scan through, find strokes on given page (only up to old end)
  while(datascanner_next_loop(&ds, &result)) {
    DataScannerResult newcopy;
    if (datascannerresult_copyresult(&result, dc, &newcopy)) {
      size_t filled = datacontainer_filled(dc);
      size_t len = (result.data_end - result.data_start); // length includes meta
      LOGDBG("Not enough space to copy page! Len: %zu Fill: %zu Copied: %d", 
             len, filled, numcopied);
      dc->end = oldend; // restore end as if nothing happened
      return 1;
    }
    datascannerresult_overwritepage(&newcopy, destpage);
    numcopied++;
  }

  LOGDBG("Copied %ld strokes from pg %d to %d", numcopied, sourcepage + 1, destpage + 1);

  return 0;
}

// Swap the entire contents of the given two pages
int edit_swap_pages(DataContainer * dc, const page_t sourcepage, const page_t destpage) {
  DataScannerResult result;
  DataScanner ds;
  u32 numtouched = 0;

  if(sourcepage == destpage) {
    LOGDBG("WARN: skipping useless self-swap");
    return 1;
  }

  // First, set all pages in sourcepage to a non-valid temp page. 12 bits = 4096,
  // just assume we've reserved some set of the last for this
  ds = datacontainer_get_scanner(dc);
  ds.page = sourcepage;
  while(datascanner_next_loop(&ds, &result)) {
    datascannerresult_overwritepage(&result, JDDC_PAGE_TMP);
    numtouched++;
  }

  // Now, reset and update the destpages to sourcepage
  ds = datacontainer_get_scanner(dc);
  ds.page = destpage;
  while(datascanner_next_loop(&ds, &result)) {
    datascannerresult_overwritepage(&result, sourcepage);
    numtouched++;
  }

  // Then go back to old source and write with dest
  ds = datacontainer_get_scanner(dc);
  ds.page = JDDC_PAGE_TMP;
  while(datascanner_next_loop(&ds, &result)) {
    datascannerresult_overwritepage(&result, destpage);
  }

  LOGDBG("Swapped %ld strokes between pg %d and %d", numtouched, sourcepage + 1, destpage + 1);
  return 0;
}

int edit_delete_page(DataContainer * dc, const page_t page) {
  size_t reclaim = 0;
  u32 delstrokes = 0;

  DataScannerResult result;
  DataScanner ds = datacontainer_get_scanner(dc);

  // Scan for ALL pages
  while (datascanner_next_loop(&ds, &result)) {
    if(result.page == page) {
      // No data to move, just increase reclaim amount
      reclaim += (result.data_end - result.data_start);
      delstrokes++;
    }
    else if(reclaim > 0) {
      // Move this stroke back by the reclaim amount
      size_t stroke_len = result.data_end - result.data_start;
      for(size_t i = 0; i < stroke_len; i++) {
        result.data_start[i - reclaim] = result.data_start[i];
      }
    }
  }

  // move the container's end back
  dc->end -= reclaim;

  LOGDBG("Deleted %ld strokes for pg %d", delstrokes, page + 1);
  
  return 0;
}

// Move given page into the slot occupied by destpage, pushing all pages to the right
// (including destpage). This is effectively removing sourcepage and inserting it to
// the left of destpage
int edit_move_page(DataContainer * dc, const page_t sourcepage, const page_t destpage) {
  DataScannerResult result;
  DataScanner ds;
  u32 numtouched = 0;

  if(sourcepage == destpage) {
    LOGDBG("WARN: skipping useless self-swap");
    return 1;
  }

  int direction = sourcepage < destpage ? 1 : -1;

  // First, set all pages in sourcepage to a non-valid temp page. 12 bits = 4096,
  // just assume we've reserved some set of the last for this
  ds = datacontainer_get_scanner(dc);
  ds.page = sourcepage;
  while(datascanner_next_loop(&ds, &result)) {
    datascannerresult_overwritepage(&result, JDDC_PAGE_TMP);
    numtouched++;
  }

  // Move from source page to destpage, shifting pages to fill holes
  for(u16 page = sourcepage; page != destpage; page += direction) {
    ds = datacontainer_get_scanner(dc);
    ds.page = page + direction; // page away from hole towards dest
    while(datascanner_next_loop(&ds, &result)) {
      datascannerresult_overwritepage(&result, page);
      numtouched++;
    }
  }

  // Then go back to old source and write with dest
  ds = datacontainer_get_scanner(dc);
  ds.page = JDDC_PAGE_TMP;
  while(datascanner_next_loop(&ds, &result)) {
    datascannerresult_overwritepage(&result, destpage);
  }

  LOGDBG("Moved %ld strokes between pg %d and %d", numtouched, sourcepage + 1, destpage + 1);
  return 0;
}
