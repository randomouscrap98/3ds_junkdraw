#ifndef __HEADER_JUNKDRAW_EDIT__
#define __HEADER_JUNKDRAW_EDIT__

#include "datacontainer.h"

int edit_copy_page(DataContainer * dc, const page_t sourcepage, const page_t destpage);
int edit_swap_pages(DataContainer * dc, const page_t sourcepage, const page_t destpage);
int edit_delete_page(DataContainer * dc, const page_t page);
int edit_move_page(DataContainer * dc, const page_t sourcepage, const page_t destpage);

#endif
