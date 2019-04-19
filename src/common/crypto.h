#ifndef CIPHER_H
#define CIPHER_H

class Crypto{
private:
	const char* key;
	const char* iv;	
public:
	Crypto(const char* key, const char* iv);
	unsigned char* encrypt(const char* bufferName, size_t size);
	unsigned char* decrypt(const char* bufferName, size_t size);
};

#endif
