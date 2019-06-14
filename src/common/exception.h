#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <cstring>
#include <string>
using namespace std;

/* `explicit` specifier forces constructors to not being implicitly called */
class Ex {
    private:
        string msg;
    public:
        explicit Ex();
        explicit Ex(const char* msg);
        explicit Ex(const char* msg, int errn);
        friend ostream& operator<<(ostream&, const Ex&);
};

/* Parent::Parent makes classes inherit their constructors from parents (C++11) */
/* Network Exceptions */
class ExNetwork : public Ex { using Ex::Ex; };
class ExBind : public ExNetwork { using ExNetwork::ExNetwork; };
class ExSend : public ExNetwork { using ExNetwork::ExNetwork; };
class ExRecv : public ExNetwork { using ExNetwork::ExNetwork; };
class ExListen : public ExNetwork { using ExNetwork::ExNetwork; };
class ExSocket : public ExNetwork { using ExNetwork::ExNetwork; };
class ExConnect : public ExNetwork { using ExNetwork::ExNetwork; };
class ExPort : public ExNetwork { using ExNetwork::ExNetwork; };

/* User Input Exception */
class ExUserInput : public Ex { using Ex::Ex; };

/* Crypto Exceptions */
class ExCrypto : public Ex { using Ex::Ex; };
class ExEVPUpdate : public ExCrypto { using ExCrypto::ExCrypto; };

#endif
