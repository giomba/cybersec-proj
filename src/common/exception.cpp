#include "exception.h"

Ex::Ex() {
    this->msg = string("generic exception");
}

Ex::Ex(const char* msg) {
    this->msg = string(msg);
}

Ex::Ex(const char* msg, int n) {
    this->msg = string(msg) + ": " + strerror(n);
}


ostream& operator<<(ostream& os, const Ex& e) {
    os << e.msg;
    return os;
}
