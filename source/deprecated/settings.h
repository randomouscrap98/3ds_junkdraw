#ifndef __HEADER_SETTINGS
#define __HEADER_SETTINGS

#include "system.h"

void set_default_settings(struct SystemState *);
int save_settings(struct SystemState *, const char *);
int load_settings(struct SystemState *, const char *);

#endif
