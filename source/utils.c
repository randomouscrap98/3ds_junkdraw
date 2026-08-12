#include "utils.h"

#include <stdlib.h>

int logbuffer_init(LogBuffer * lb, size_t slots, size_t slot_size) {
  lb->slot_size = slot_size;
  lb->slot_count = slots;
  lb->head = 0;
  lb->buffer = calloc(slots * slot_size, sizeof(char));
  return lb->buffer != NULL;
}

void logbuffer_free(LogBuffer * lb) {
  if(lb->buffer) {
    free(lb->buffer);
    lb->buffer = NULL;
  }
}

char * logbuffer_str(LogBuffer * lb, size_t slot) {
  return lb->buffer + (lb->slot_size * slot);
}

// void lobbuffer_log_std(LogBuffer * lb, const char * str, u8 color) {
//   printf("\x1b[%dm", color);
//   //printf("[%02d|%02d:%02d] ", _db_prnt_num, timeinfo->tm_hour, timeinfo->tm_min);
//   vprintf(fmt, args);
//   printf("\n");
// }
