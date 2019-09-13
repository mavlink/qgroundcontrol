#pragma once

#include <string>
#include <memory>
#include <openssl/rsa.h>

using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;

class OpenSSL_RSA
{
public:
    OpenSSL_RSA();

    ~OpenSSL_RSA();

    bool generate();

    bool generate_public(std::string key);

    bool generate_private(std::string key);

    std::string get_public_key();

    std::string get_private_key();

    std::string encrypt(std::string plain_text);

    std::string decrypt(std::string cipher_text);

    std::string sign(std::string plain_text);

    bool verify(std::string cipher_text, std::string signature);

private:
    RSA_ptr     _rsa_public;
    RSA_ptr     _rsa_private;
};
