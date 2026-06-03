#include "metadata.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int get_iso_datetime(char * buf, u32 size) {
  if(size < 20) {
    return -1;
  }

  time_t raw_time;
  struct tm *local_time;

  if (time(&raw_time) == (time_t)(-1)) {
    return -1;
  }

  // Convert to UTC (thread-safe variants like gmtime_r are preferred if available)
  local_time = localtime(&raw_time);
  if (local_time == NULL) {
      return -1;
  }

  sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
          local_time->tm_year + 1900, // tm_year is years since 1900
          local_time->tm_mon + 1,     // tm_mon is 0-11
          local_time->tm_mday,
          local_time->tm_hour,
          local_time->tm_min,
          local_time->tm_sec);

  return 0;
}

int metacontainer_init(metacontainer * mc, u32 size) {
  mc->raw = malloc(sizeof(char) * size);
  if(mc->raw == NULL) {
    return -1;
  }
  mc->size = size;
  *mc->raw = 0;
  return 0;
}

void metacontainer_free(metacontainer * mc) {
  free(mc->raw);
}

int metacontainer_addsimple(metacontainer * mc, const char * key) {
  char time[64];
  int result = get_iso_datetime(time, 64);
  if(result) {
    return result;
  }
  u32 end = strlen(mc->raw);
  // 3 = null terminator, newline, space (notice we don't include null in left)
  if(mc->size - end < 3 + strlen(key) + strlen(time)) {
    return -1;
  }
  sprintf(mc->raw + end, "%s %s\n", key, time);
  return 0;
}

char * metacontainer_scanback(metacontainer * mc, char * from) {
  if(from == NULL) {
    from = mc->raw + strlen(mc->raw);
  }
  // Already at front; nothing left to scan
  if(from == mc->raw) {
    return NULL;
  }
  from--; // Move back off the current location, should be at newline
  while(from != mc->raw) {
    from--; // move back immediately
    if(*from == '\n') {
      // This is the place, but don't include the newline
      return from + 1;
    }
  }
  return from;
}

char * metacontainer_scanback_key(metacontainer * mc, char * from, const char * key) {
  int keylen = strlen(key);
  while(1) {
    from = metacontainer_scanback(mc, from);
    if(from == NULL) {
      return NULL;
    } else if(strncmp(from, key, keylen) == 0) {
      return from;
    }
  }
}

char * metacontainer_skip_key(metacontainer * mc, char * pos) {
  if(pos == NULL) { //|| pos >= mc->raw + strlen(mc->raw)) {
    return NULL;
  }
  char * result = strchr(pos, ' ');
  if(result != NULL) {
    while(*result == ' ') {
      result++;
    }
  }
  return result;
}

int metacontainer_lastloads_differentdate(metacontainer * mc) {
  // Scan back one, get last date
  char * lastload = metacontainer_scanback_key(mc, NULL, METAKEY_LOAD);
  if(lastload == NULL) {
    return 1; // Not even a single load
  }
  char * lastdate = metacontainer_skip_key(mc, lastload);
  if(lastdate == NULL) {
    return 1; // Not even a single load? Or date is broken?
  }

  char * penultload = metacontainer_scanback_key(mc, lastload, METAKEY_LOAD);
  if(penultload == NULL) {
    return 1; // Not a second load
  }
  char * penultdate = metacontainer_skip_key(mc, penultload);
  if(penultdate == NULL) {
    return 1; // Not even a single load? Or date is broken?
  }

  // The amount of chars to check is 10: YYYY-MM-DD
  return strncmp(lastdate, penultdate, 10);
}
