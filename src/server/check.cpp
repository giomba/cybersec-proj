#include "check.h"

bool checkPort(uint16_t port) {
    return (port < (1 << 16));
}
