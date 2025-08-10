#ifndef __HEADER_SETTINGS
#define __HEADER_SETTINGS

#include "system.h"

void set_default_settings(struct SystemState *);
void save_settings(struct SystemState *, const char *);
void load_settings(struct SystemState *, const char *);

#endif
