#ifndef __HEADER_JD_CPAD__
#define __HEADER_JD_CPAD__

#include <3ds.h>

typedef struct {
  u16 deadzone;
  float mod_constant;
  float mod_multiplier;
  float mod_curve;
  float mod_general;
} CpadProfile;

// Get a default cpad profile (user COULD modify these but we currently don't have a way to do that)
CpadProfile cpadprofile_default();

// Transform a given position (could be x or y) using the given cpad value (also assumed to be in the same axis)
float cpadprofile_translate(CpadProfile * profile, s16 cpad_magnitude, float existing_pos);

// Simplify: read the cpad 

#endif
