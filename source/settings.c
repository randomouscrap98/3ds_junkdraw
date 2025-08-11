#include <ini.h>

#include "filesys.h"
#include "settings.h"
#include "system.h"

#define SLOWAVGKEY "slow_avg"
#define POWERSAVEKEY "power_saver"
#define ONIONCOUNTKEY "onion_count"
#define ONIONSTARTKEY "onion_blendstart"
#define COLORMODEKEY "color_mode"

#define MAXSETTINGSREAD 2048

// TODO: get rid of this later
// #include "console.h"

void load_settings_raw(struct SystemState *sys, char *settings) {
  // printf("%s\n", settings);
  // printf_flush("\x1b[%d;1H%-150s\x1b[%d;2H\x1b[%dmSettings len: %d", 6, "",
  // 6,
  //              33, strlen(settings));
  // while (1) {
  // }
  ini_t ini = ini_parse_str(settings, NULL);
  initable_t *iniroot = ini_get_table(&ini, INI_ROOT);
  sys->slow_avg = (float)ini_as_num(ini_get(iniroot, SLOWAVGKEY));
  sys->power_saver = (bool)ini_as_int(ini_get(iniroot, POWERSAVEKEY));
  sys->onion_count = (int)ini_as_int(ini_get(iniroot, ONIONCOUNTKEY));
  set_systemstate_onionstart(
      sys, (float)ini_as_num(ini_get(iniroot, ONIONSTARTKEY)));
  sys->colors.mode = (int)ini_as_int(ini_get(iniroot, COLORMODEKEY));
  ini_free(&ini);
}

void set_default_settings(struct SystemState *sys) {
  sys->slow_avg = 0.15;
  sys->power_saver = false;
  sys->onion_count = 3;
  set_systemstate_onionstart(sys, 0.3);
  sys->colors.mode = 0;
}

int save_settings(struct SystemState *sys, const char *path) {
  char settings[MAXSETTINGSREAD];
  /* clang-format off */
  // NOTE: DO NOT SET LAYER VISIBILITY! That should always default to "all"
  // and is runtime dependent anyway
  snprintf(settings,MAXSETTINGSREAD,
    //"version = 1\n"
    SLOWAVGKEY " = %f\n"
    POWERSAVEKEY " = %d\n"
    ONIONCOUNTKEY " = %d\n"
    ONIONSTARTKEY " = %f\n"
    COLORMODEKEY " = %d\n",
      sys->slow_avg,
      sys->power_saver,
      sys->onion_count,
      sys->onion_blendstart,
      sys->colors.mode
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
