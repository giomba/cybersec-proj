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

/* Cryptography Exceptions */
class ExCrypto : public Ex { using Ex::Ex; };
class ExCryptoComputation : public ExCrypto { using ExCrypto::ExCrypto; };
class ExBadSeqNum : public ExCrypto { using ExCrypto::ExCrypto; };
class ExBadHMAC : public ExCrypto { using ExCrypto::ExCrypto; };
class ExBadProtocol : public ExCrypto { using ExCrypto::ExCrypto; };
class ExCertificate : public ExCrypto { using ExCrypto::ExCrypto; };
class ExRandom : public ExCrypto { using ExCrypto::ExCrypto; };
class ExSignature : public ExCrypto { using ExCrypto::ExCrypto; };
class ExSeqNumOverflow : public ExCrypto { using ExCrypto::ExCrypto; };

/* Protocol exceptions */
class ExProtocol : public Ex { using Ex::Ex; };
class ExTooBig : public ExProtocol { using ExProtocol::ExProtocol; };
class ExNonce : public ExProtocol { using ExProtocol::ExProtocol; };

#endif
