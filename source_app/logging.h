#ifndef __HEADER_JD_LOGGING__
#define __HEADER_JD_LOGGING__

#include "littlelogbox.h"

// NOTE: for now, the log can't even scroll, so no need to make it large
#define MAX_LOGMESSAGES 30
#define MAX_LOGMSGLENGTH 100

// Remove any of these where you don't want logging for that
#define DO_LOGTRACE
#define DO_LOGDEBUG
#define DO_LOGINFO
#define DO_LOGWARNING
#define DO_LOGERROR

int logging_init();
// Using the given render function, only render if there has been messages since
// the last time we rendered.
void logging_try_render(void (*render)(tui_logbox *), int force);
void logging_free();

#endif
