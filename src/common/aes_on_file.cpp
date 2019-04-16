#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
using namespace std;

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char * key, unsigned char *iv, unsigned char *ciphertext)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int ciphertext_len;

	ctx = EVP_CIPHER_CTX_new();

	EVP_EncryptInit(ctx, EVP_aes_128_ecb(), key, iv);

	EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
	ciphertext_len = len;

	EVP_EncryptFinal(ctx, ciphertext + len, &len);
	ciphertext_len += len;

	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char * key, unsigned char *iv, unsigned char *decryptedtext)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int decryptedtext_len;

	ctx = EVP_CIPHER_CTX_new();

	EVP_DecryptInit(ctx, EVP_aes_128_ecb(), key, iv);

	EVP_DecryptUpdate(ctx, decryptedtext, &len, ciphertext, ciphertext_len);
	decryptedtext_len = len;

	EVP_DecryptFinal(ctx, decryptedtext + len, &len);
	decryptedtext_len += len;

	EVP_CIPHER_CTX_free(ctx);

	return decryptedtext_len;
}

void encryptfile(string file_in, string file_out)
{
	unsigned char *key = (unsigned char *) "0123456789012345";
	string plaintext; 
	fstream inFile,outFile;
	inFile.open(file_in, ios_base::in | ios_base::binary); 
	outFile.open(file_out, ios_base::out | ios_base::binary);
	unsigned char ch = inFile.get();
    	while (inFile.good())
    	{
        	plaintext.push_back(ch);
        	ch = inFile.get();
    	}
    	size_t size = plaintext.size();
	unsigned char* ciphertext = (unsigned char *) malloc(sizeof(plaintext)+16);
	int ciphertext_len;
	ciphertext_len = encrypt((unsigned char*) plaintext.data(), plaintext.size(), key, NULL, ciphertext);
	for (int i = 0; i < ciphertext_len; ++i) {
		cout.put(ciphertext[i]);
	}
	outFile.write((char*)&ciphertext, sizeof((char*)ciphertext));
	inFile.close();
	outFile.close();
}

void decryptfile(string file_in, string file_out)
{
	string ciphertext;
	unsigned char *key = (unsigned char *) "0123456789012345";
	fstream inFile,outFile;
	inFile.open(file_in, ios_base::in | ios_base::binary); 
	outFile.open(file_out, ios_base::out | ios_base::binary);
	unsigned char ch = inFile.get();
	while(inFile.good())
	{
		ciphertext.push_back(ch);	
		ch = inFile.get();	
	}
	unsigned char* decryptedtext = (unsigned char *)malloc(sizeof(ciphertext));
	int decryptedtext_len, ciphertext_len;
	ciphertext_len = ciphertext.size();
	for (int i = 0; i < ciphertext_len; ++i) {
		cout.put(ciphertext[i]);
	}
	decryptedtext_len = decrypt((unsigned char*)ciphertext.data(), ciphertext_len, key, NULL, decryptedtext);
	printf("%s\n",(char*)decryptedtext);
	for (int i = 0; i < decryptedtext_len; ++i) {
		cout.put(decryptedtext[i]);
	}
	outFile.write((char*)&decryptedtext, sizeof(decryptedtext));
	inFile.close();
	outFile.close();	
}
int main(void)
{
/*	unsigned char *key = (unsigned char *) "0123456789012345";
	string plaintext; 
	fstream inFile,outFile;
	outFile.open("ciphered.bin", ios_base::out | ios_base::binary);
	inFile.open("plaintext.txt", ios_base::in | ios_base::binary); 
	unsigned char ch = inFile.get();
    	while (inFile.good())
    	{
        	plaintext.push_back(ch);
        	ch = inFile.get();
    	}
    	size_t size = plaintext.size();
	//plaintext = "A very top secret message";
	
	unsigned char* ciphertext = (unsigned char *) malloc(sizeof(plaintext)+16);

	int decryptedtext_len, ciphertext_len;

	ciphertext_len = encrypt((unsigned char*) plaintext.data(), plaintext.size(), key, NULL, ciphertext);
	outFile.write((char*)&ciphertext, sizeof(ciphertext));
	for (int i = 0; i < ciphertext_len; ++i) {
		cout.put(ciphertext[i]);
	}
//	printf("Ciphertext is:\n");
//	BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);

	unsigned char* decryptedtext = (unsigned char *)malloc(sizeof(plaintext)+16);

	decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, NULL, decryptedtext);
//	decryptedtext_len = decrypt((unsigned char*) plaintext.data(), plaintext.size(), key, NULL, decryptedtext);

	for (int i = 0; i < decryptedtext_len; ++i) {
		cout.put(decryptedtext[i]);
	}
	
//	decryptedtext[decryptedtext_len]='\0';
//	printf("Decrypted text is:\n");
//	printf("%s\n", decryptedtext);

	inFile.close();
	outFile.close();
*/
	encryptfile("plaintext.txt","binary2.bin");
	//decryptfile("binary2.bin", "plaintext2.txt");
	return 0;
}
