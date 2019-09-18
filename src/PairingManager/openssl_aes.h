#pragma once

#include <string>
#include <openssl/evp.h>

class OpenSSL_AES
{
public:
    OpenSSL_AES(std::string password, unsigned long long salt, bool use_compression = true);

    ~OpenSSL_AES();

    std::string encrypt(std::string plain_text);

    std::string decrypt(std::string cipher_text);

private:
    bool _use_compression;
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_CIPHER_CTX *enc_cipher_context = nullptr;
    EVP_CIPHER_CTX *dec_cipher_context = nullptr;
#else
    EVP_CIPHER_CTX enc_cipher_context;
    EVP_CIPHER_CTX dec_cipher_context;
#endif
};
