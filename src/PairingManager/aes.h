#ifndef AES_H
#define AES_H

#pragma once

#include <string>
#include <vector>
#include <openssl/evp.h>

class AES
{
public:
    AES(std::string password, unsigned long long salt);

    ~AES();

    std::string encrypt(std::string plainText);

    std::string decrypt(std::string cipherText);

private:
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_CIPHER_CTX *encCipherContext = nullptr;
    EVP_CIPHER_CTX *decCipherContext = nullptr;
#else
    EVP_CIPHER_CTX encCipherContext;
    EVP_CIPHER_CTX decCipherContext;
#endif

    std::string base64Encode(const std::vector<unsigned char>& binary);

    std::vector<unsigned char> base64Decode(std::string encoded);
};

#endif // AES_H
