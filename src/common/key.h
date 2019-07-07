#ifndef KEY_H
#define KEY_H

#include <string>
using namespace std;
#include "cybrand.h"

class Key {
    private:
        char* key;
        int len;
    public:
        Key(int);
        Key(string);
        Key(const Key& old);    /* never use! */
        ~Key();

        string str(void);
};

#endif