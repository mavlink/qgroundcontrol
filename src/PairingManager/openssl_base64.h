#pragma once

#include <string>
#include <vector>
#include <openssl/evp.h>

class OpenSSL_Base64
{
public:
    static std::string encode(const std::vector<unsigned char>& binary);

    static std::vector<unsigned char> decode(std::string encoded);
};
