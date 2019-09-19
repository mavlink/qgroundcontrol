#include "openssl_base64.h"

#include <memory>
#include <openssl/bio.h>

//-----------------------------------------------------------------------------
struct BIOFreeAll { void operator()(BIO* p) { BIO_free_all(p); } };

std::string
OpenSSL_Base64::encode(const std::vector<unsigned char>& binary)
{
    std::unique_ptr<BIO, BIOFreeAll> b64(BIO_new(BIO_f_base64()));
    BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
    BIO* sink = BIO_new(BIO_s_mem());
    BIO_push(b64.get(), sink);
    BIO_write(b64.get(), binary.data(), static_cast<int>(binary.size()));
    BIO_ctrl(b64.get(), BIO_CTRL_FLUSH, 0, nullptr);
    const char* encoded;
    const unsigned long len = static_cast<unsigned long>(BIO_ctrl(sink, BIO_CTRL_INFO, 0, &encoded));

    return std::string(encoded, len);
}

//-----------------------------------------------------------------------------
std::vector<unsigned char>
OpenSSL_Base64::decode(std::string encoded)
{
    std::unique_ptr<BIO, BIOFreeAll> b64(BIO_new(BIO_f_base64()));
    BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
    BIO* source = BIO_new_mem_buf(encoded.c_str(), -1); // read-only source
    BIO_push(b64.get(), source);
    const unsigned long maxlen = encoded.length() / 4 * 3 + 1;
    std::vector<unsigned char> decoded(maxlen);
    const unsigned long len = static_cast<unsigned long>(BIO_read(b64.get(), decoded.data(), static_cast<int>(maxlen)));
    decoded.resize(len);

    return decoded;
}

//-----------------------------------------------------------------------------
