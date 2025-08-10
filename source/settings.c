#include <ini.h>

#include "filesys.h"
#include "log.h"
#include "settings.h"
#include "system.h"

#define SLOWAVGKEY "slow_avg"
#define POWERSAVEKEY "power_saver"
#define ONIONCOUNTKEY "onion_count"
#define ONIONSTARTKEY "onion_blendstart"
#define COLORMODEKEY "color_mode"

// // C lets you redefine stuff... right?
// #ifndef LOGDBG
// #define LOGDBG(f_, ...)
// #endif

// NOTE: DO NOT SET LAYER VISIBILITY! That should always default to "all"
// and is runtime dependent anyway
/* clang-format off */
#define DEFAULT_SETTINGS                                                       \
  "version = 1\n"                                                              \
  SLOWAVGKEY " = 0.15\n"                                                       \
  POWERSAVEKEY " = 0\n"                                                        \
  ONIONCOUNTKEY " = 3\n"                                                       \
  ONIONSTARTKEY " = 0.3\n"                                                     \
  COLORMODEKEY " = 0"
/* clang-format on */

#define MAXSETTINGSREAD 2048

void load_settings_raw(struct SystemState *sys, char *settings) {
  ini_t ini = ini_parse_str(settings, NULL);
  initable_t *iniroot = ini_get_table(&ini, INI_ROOT);
  sys->slow_avg = (float)ini_as_num(ini_get(iniroot, SLOWAVGKEY));
  sys->power_saver = (bool)ini_as_int(ini_get(iniroot, POWERSAVEKEY));
  sys->onion_count = ini_as_int(ini_get(iniroot, ONIONCOUNTKEY));
  set_systemstate_onionstart(
      sys, (float)ini_as_num(ini_get(iniroot, ONIONSTARTKEY)));
  sys->colors.mode = ini_as_int(ini_get(iniroot, COLORMODEKEY));
  ini_free(&ini);
}

void set_default_settings(struct SystemState *sys) {
  load_settings_raw(sys, DEFAULT_SETTINGS);
}

void load_settings(struct SystemState *sys, const char *path) {
  char settings[MAXSETTINGSREAD];
  if (!read_file(path, settings, MAXSETTINGSREAD)) {
    LOGDBG("No settings.ini: Using default settings");
    strcpy(settings, DEFAULT_SETTINGS);
  }
  load_settings_raw(sys, settings);
}

void save_settings(struct SystemState *sys, const char *path) {
  // ini_t ini = ini_parse_str(settings, NULL);
  // initable_t *iniroot = ini_get_table(&ini, INI_ROOT);
  // sys->slow_avg = (float)ini_as_num(ini_get(iniroot, SLOWAVGKEY));
  // sys->power_saver = (bool)ini_as_int(ini_get(iniroot, POWERSAVEKEY));
  // sys->onion_count = ini_as_int(ini_get(iniroot, ONIONCOUNTKEY));
  // set_systemstate_onionstart(
  //     sys, (float)ini_as_num(ini_get(iniroot, ONIONSTARTKEY)));
  // sys->colors.mode = ini_as_int(ini_get(iniroot, COLORMODEKEY));
  // ini_free(&ini);
}
