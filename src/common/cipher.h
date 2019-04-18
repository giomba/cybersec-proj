
class Crypto{
private:
	unsigned char* key;
	unsigned char* iv;	
public:
	Crypto(const char* key, const char* iv);
	const char* encrypt(const char* bufferName, size_t size);
	const char* decrytp(const char* bufferName, size_t size);
}
