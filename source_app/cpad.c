#include "cpad.h"

#include <math.h>
#include <stdlib.h>

CpadProfile cpadprofile_default() {
  return (CpadProfile) {
    .deadzone = 40,
    .mod_constant = 1,
    .mod_multiplier = 0.02f,
    .mod_curve = 3.2f,
    .mod_general = 1,
  };
}

float cpadprofile_translate(CpadProfile * profile, s16 cpad_magnitude, float existing_pos) {
  u16 cpadmag = abs(cpad_magnitude);
  if(cpadmag > profile->deadzone) {
    return existing_pos + (cpad_magnitude < 0 ? -profile->mod_general : profile->mod_general) * 
    (profile->mod_constant + pow(cpadmag * profile->mod_multiplier, profile->mod_curve));
  }
  else {
    return existing_pos;
  }
}
