#include "debug.h"

int debuglevel = 0;

/* parse debug level from command line arguments, and set debug level */
char* debugenable(char* argv) {

    #ifdef DEBUGPRINT
    debug(FATAL, "[ ] this is a debug release" << endl);

    if (strcmp("FATAL", argv) == 0) { debuglevel = FATAL; }
    if (strcmp("ERROR", argv) == 0) { debuglevel = ERROR; }
    if (strcmp("WARNING", argv) == 0) { debuglevel = WARNING; }
    if (strcmp("INFO", argv) == 0) { debuglevel = INFO; }
    if (strcmp("DEBUG", argv) == 0) { debuglevel = DEBUG; }

    debug(FATAL, "[ ] debug level: " << debuglevel << endl);
    #endif

    return argv;
}