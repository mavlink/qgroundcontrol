#pragma once

#include <string>
#include <openssl/evp.h>

const unsigned long long default_salt = 0x368de30e8ec063ce;

class OpenSSL_AES
{
public:
    OpenSSL_AES();

    OpenSSL_AES(std::string password, unsigned long long salt = default_salt, bool use_compression = true)
    { init(password, salt, use_compression); }

    ~OpenSSL_AES();

    void init(std::string password, unsigned long long salt = default_salt, bool use_compression = true);

    void deinit();

    std::string encrypt(std::string plain_text);

    std::string decrypt(std::string cipher_text);

private:
    bool _initialized = false;
    std::string _password;
    unsigned long long _salt = default_salt;
    bool _use_compression = false;
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_CIPHER_CTX *enc_cipher_context = nullptr;
    EVP_CIPHER_CTX *dec_cipher_context = nullptr;
#else
    EVP_CIPHER_CTX enc_cipher_context;
    EVP_CIPHER_CTX dec_cipher_context;
#endif
};
