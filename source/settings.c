#include <ini.h>

#include "filesys.h"
#include "settings.h"
#include "system.h"

#define DEFAULTKEY "default"
#define SLOWAVGKEY "slow_avg"
#define POWERSAVEKEY "power_saver"
#define ONIONCOUNTKEY "onion_count"
#define ONIONSTARTKEY "onion_blendstart"
#define COLORMODEKEY "color_mode"
#define CONTROLSCHEMEKEY "control_scheme"
#define DATESTAMPEKEY "datestamp"

#define MAXSETTINGSREAD 2048

void load_settings_raw(struct SystemState *sys, char *settings) {
  ini_t ini = ini_parse_str(settings, NULL);
  initable_t *iniroot = ini_get_table(&ini, DEFAULTKEY);
  sys->slow_avg = (float)ini_as_num(ini_get(iniroot, SLOWAVGKEY));
  sys->power_saver = (bool)ini_as_int(ini_get(iniroot, POWERSAVEKEY));
  sys->onion_count = (int)ini_as_int(ini_get(iniroot, ONIONCOUNTKEY));
  set_systemstate_onionstart(
      sys, (float)ini_as_num(ini_get(iniroot, ONIONSTARTKEY)));
  sys->colors.mode = (int)ini_as_int(ini_get(iniroot, COLORMODEKEY));
  sys->control_scheme = (u16)ini_as_int(ini_get(iniroot, CONTROLSCHEMEKEY));
  sys->datestamp = (bool)ini_as_int(ini_get(iniroot, DATESTAMPEKEY));
  ini_free(&ini);
}

void set_default_settings(struct SystemState *sys) {
  sys->slow_avg = 0.15;
  sys->power_saver = false;
  sys->onion_count = 3;
  set_systemstate_onionstart(sys, 0.3);
  sys->colors.mode = 0;
  sys->control_scheme = 0;
  sys->datestamp = false;
}

int save_settings(struct SystemState *sys, const char *path) {
  char settings[MAXSETTINGSREAD];
  /* clang-format off */
  // NOTE: DO NOT SET LAYER VISIBILITY! That should always default to "all"
  // and is runtime dependent anyway
  snprintf(settings,MAXSETTINGSREAD,
    "[" DEFAULTKEY "]\n"
    "version = 1\n"
    SLOWAVGKEY " = %f\n"
    POWERSAVEKEY " = %d\n"
    DATESTAMPEKEY " = %d\n"
    ONIONCOUNTKEY " = %d\n"
    ONIONSTARTKEY " = %f\n"
    COLORMODEKEY " = %d\n"
    CONTROLSCHEMEKEY " = %d\n",
      sys->slow_avg,
      sys->power_saver,
      sys->datestamp,
      sys->onion_count,
      sys->onion_blendstart,
      sys->colors.mode,
      sys->control_scheme
    );
  /* clang-format on */
  return write_file(path, settings);
}

int load_settings(struct SystemState *sys, const char *path) {
  char settings[MAXSETTINGSREAD];
  if (!read_file(path, settings, MAXSETTINGSREAD)) {
    return -1;
  }
  load_settings_raw(sys, settings);
  return 0;
}
