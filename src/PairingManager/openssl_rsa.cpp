#include "openssl_aes.h"
#include "openssl_rsa.h"
#include "openssl_base64.h"

#include <algorithm>
#include <memory>
#include <memory.h>
#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <sstream>
#include <zlib.h>

using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using BIO_MEM_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

const int                rsa_ley_length = 2048;
const unsigned long long rsa_aes_salt   = 0xea07f38b3a0ec213;

//-----------------------------------------------------------------------------
static std::vector<std::string>
split(std::string str, char delimiter)
{
    std::vector<std::string> internal;
    std::stringstream ss(str); // Turn the string into a stream.
    std::string tok;

    while (getline(ss, tok, delimiter)) {
        internal.push_back(tok);
    }

    return internal;
}

//-----------------------------------------------------------------------------
static std::string
random_string(uint length)
{
    srand(time(nullptr));
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const uint max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}

//-----------------------------------------------------------------------------
OpenSSL_RSA::OpenSSL_RSA() :
    _rsa_public(nullptr, ::RSA_free),
    _rsa_private(nullptr, ::RSA_free)
{
}

//-----------------------------------------------------------------------------
OpenSSL_RSA::~OpenSSL_RSA()
{
}

//-----------------------------------------------------------------------------
bool
OpenSSL_RSA::generate()
{
    int rc;

    BN_ptr bn(BN_new(), ::BN_free);
    rc = BN_set_word(bn.get(), RSA_F4);
    if (rc != 1) {
        return false;
    }

    RSA_ptr rsa(RSA_new(), ::RSA_free);
    rc = RSA_generate_key_ex(rsa.get(), rsa_ley_length, bn.get(), nullptr);
    if (rc != 1) {
        return false;
    }

    _rsa_public.reset(RSAPublicKey_dup(rsa.get()));
    _rsa_private.reset(RSAPrivateKey_dup(rsa.get()));

    return true;
}

//-----------------------------------------------------------------------------
bool
OpenSSL_RSA::generate_public(std::string key)
{
    BIO* bio = BIO_new_mem_buf(key.c_str(), key.length() + 1);
    if (bio == nullptr) {
        return false;
    }

    RSA* rsa = nullptr;
    PEM_read_bio_RSAPublicKey(bio, &rsa, nullptr, nullptr);
    if (rsa == nullptr) {
        return false;
    }

       _rsa_public.reset(rsa);

    return true;
}

//-----------------------------------------------------------------------------
bool
OpenSSL_RSA::generate_private(std::string key)
{
    BIO* bio = BIO_new_mem_buf(key.c_str(), -1);
    if (bio == nullptr) {
        return false;
    }

    RSA* rsa = nullptr;
    PEM_read_bio_RSAPrivateKey(bio, &rsa, nullptr, nullptr);
    if (rsa == nullptr) {
        return false;
    }

    _rsa_private.reset(rsa);

    return true;
}

//-----------------------------------------------------------------------------
std::string
OpenSSL_RSA::get_public_key()
{
    if (!_rsa_public.get()) {
        return {};
    }

    BIO_MEM_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);
    PEM_write_bio_RSAPublicKey(bio.get(), _rsa_public.get());
    int keylen = BIO_pending(bio.get());
    std::unique_ptr<char[]> pem_key(new char[keylen + 1]);
    BIO_read(bio.get(), pem_key.get(), keylen + 1);
    pem_key.get()[keylen] = 0;
    std::string key = pem_key.get();

    return key;
}

//-----------------------------------------------------------------------------
std::string
OpenSSL_RSA::get_private_key()
{
    if (!_rsa_private.get()) {
        return {};
    }

    BIO_MEM_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);
    PEM_write_bio_RSAPrivateKey(bio.get(), _rsa_private.get(), nullptr, nullptr, 0, nullptr, nullptr);
    int keylen = BIO_pending(bio.get());
    std::unique_ptr<char[]> pem_key(new char[keylen + 1]);
    BIO_read(bio.get(), pem_key.get(), keylen);
    pem_key.get()[keylen] = 0;
    std::string key = pem_key.get();

    return key;
}

//-----------------------------------------------------------------------------
std::string
OpenSSL_RSA::encrypt(std::string plain_text)
{
    if (!_rsa_public.get()) {
        return {};
    }

    std::string aes_key = random_string(32);
    OpenSSL_AES aes(aes_key, rsa_aes_salt, false);

    std::unique_ptr<unsigned char[]> res(new unsigned char[RSA_size(_rsa_public.get())]);
    std::unique_ptr<unsigned char[]> from(new unsigned char[aes_key.length() + 1]);
    memcpy(from.get(), aes_key.c_str(), aes_key.length() + 1);
    int len = RSA_public_encrypt(aes_key.length() + 1, from.get(), res.get(), _rsa_public.get(), RSA_PKCS1_OAEP_PADDING);
    if (len <= 0) {
        return {};
    }

    std::vector<unsigned char> data(res.get(), res.get() + len);
    return OpenSSL_Base64::encode(data) + ":" + aes.encrypt(plain_text);
}

//-----------------------------------------------------------------------------
std::string
OpenSSL_RSA::decrypt(std::string cipher_text)
{
    if (!_rsa_private.get() || cipher_text.empty()) {
        return {};
    }

    auto a = split(cipher_text, ':');
    if (a.size() != 2) {
        return {};
    }

    std::unique_ptr<char[]> res(new char[RSA_size(_rsa_private.get())]);
    std::vector<unsigned char> text = OpenSSL_Base64::decode(a[0]);
    int pLen = static_cast<int>(text.size());
    int l = RSA_private_decrypt(pLen, text.data(), reinterpret_cast<unsigned char*>(res.get()), _rsa_private.get(), RSA_PKCS1_OAEP_PADDING);
    if (l < 0) {
        return {};
    }

    std::string aes_key = std::string(res.get());
    OpenSSL_AES aes(aes_key, rsa_aes_salt, false);
    return aes.decrypt(a[1]);
}

//-----------------------------------------------------------------------------
std::string
OpenSSL_RSA::sign(std::string message)
{
    if (!_rsa_private.get()) {
        return {};
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned char sign[RSA_size(_rsa_private.get())];
    unsigned int signLen;

    SHA256(reinterpret_cast<const unsigned char *>(message.c_str()), message.length() + 1, hash);

    // Sign with private key
    int rc = RSA_sign(NID_sha256, hash, SHA256_DIGEST_LENGTH, sign, &signLen, _rsa_private.get());
    if (rc != 1) {
        return {};
    }

    std::vector<unsigned char> data(sign, sign + signLen);
    return OpenSSL_Base64::encode(data);
}

//-----------------------------------------------------------------------------
bool
OpenSSL_RSA::verify(std::string message, std::string signature)
{
    if (!_rsa_public.get()) {
        return false;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    std::vector<unsigned char> sig = OpenSSL_Base64::decode(signature);
    unsigned int signLen = static_cast<unsigned int>(sig.size());
    SHA256(reinterpret_cast<const unsigned char *>(message.c_str()), message.length() + 1, hash);
    int rc = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, sig.data(), signLen, _rsa_public.get());
    return (rc == 1);
}

//-----------------------------------------------------------------------------
