#ifndef DEBUG_H
#define DEBUG_H

enum DebugType {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

void debug(DebugType type, const char* msg);

#endif
