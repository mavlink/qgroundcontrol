#pragma once

#include <string>

class OpenSSL_Rand
{
public:
    static std::string random_string(unsigned int length);
};
