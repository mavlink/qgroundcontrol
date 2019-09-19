#include "openssl_aes.h"
#include "openssl_base64.h"

#include <iostream>
#include <memory>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <string.h>
#include <zlib.h>

//-----------------------------------------------------------------------------
OpenSSL_AES::OpenSSL_AES(std::string password, unsigned long long salt, bool use_compression) :
    _use_compression(use_compression)
{
    int n_rounds = 5;
    unsigned char key[32], iv[32];

    /*
     * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
     * n_rounds is the number of times the we hash the material. More rounds are more secure but
     * slower.
     */
    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(),
                   reinterpret_cast<const unsigned char*>(&salt),
                   reinterpret_cast<const unsigned char*>(password.c_str()),
                   static_cast<int>(password.length()),
                   n_rounds, key, iv);

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    enc_cipher_context = EVP_CIPHER_CTX_new();
    dec_cipher_context = EVP_CIPHER_CTX_new();

    EVP_CIPHER_CTX_init(enc_cipher_context);
    EVP_EncryptInit_ex(enc_cipher_context, EVP_aes_256_cbc(), nullptr, key, iv);
    EVP_CIPHER_CTX_init(dec_cipher_context);
    EVP_DecryptInit_ex(dec_cipher_context, EVP_aes_256_cbc(), nullptr, key, iv);
#else
    EVP_CIPHER_CTX_init(&enc_cipher_context);
    EVP_EncryptInit_ex(&enc_cipher_context, EVP_aes_256_cbc(), nullptr, key, iv);
    EVP_CIPHER_CTX_init(&dec_cipher_context);
    EVP_DecryptInit_ex(&dec_cipher_context, EVP_aes_256_cbc(), nullptr, key, iv);
#endif
}

//-----------------------------------------------------------------------------
OpenSSL_AES::~OpenSSL_AES()
{
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_CIPHER_CTX_free(enc_cipher_context);
    EVP_CIPHER_CTX_free(dec_cipher_context);
#else
    EVP_CIPHER_CTX_cleanup(&enc_cipher_context);
    EVP_CIPHER_CTX_cleanup(&dec_cipher_context);
#endif
}

//-----------------------------------------------------------------------------
std::string
OpenSSL_AES::encrypt(std::string plain_text)
{
    unsigned long source_len = static_cast<unsigned long>(plain_text.length() + 1);
    unsigned long dest_len = source_len * 2;
    std::unique_ptr<unsigned char[]> compressed(new unsigned char[dest_len]);
    if (_use_compression) {
        int err = compress2(compressed.get(), &dest_len,
                            reinterpret_cast<const unsigned char *>(plain_text.c_str()),
                            source_len, 9);
        if (err != Z_OK) {
            return {};
        }
    } else {
        memcpy(compressed.get(), plain_text.c_str(), source_len);
    }

    int p_len = static_cast<int>(dest_len);
    int c_len = p_len + AES_BLOCK_SIZE;
    int f_len = 0;
    std::unique_ptr<unsigned char[]> cipher_text(new unsigned char[c_len]);

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_EncryptInit_ex(enc_cipher_context, nullptr, nullptr, nullptr, nullptr);
    EVP_EncryptUpdate(enc_cipher_context, cipher_text.get(), &c_len, compressed.get(), p_len);
    EVP_EncryptFinal_ex(enc_cipher_context, cipher_text.get() + c_len, &f_len);
#else
    EVP_EncryptInit_ex(&enc_cipher_context, nullptr, nullptr, nullptr, nullptr);
    EVP_EncryptUpdate(&enc_cipher_context, cipher_text.get(), &c_len, compressed.get(), p_len);
    EVP_EncryptFinal_ex(&enc_cipher_context, cipher_text.get() + c_len, &f_len);
#endif

    std::vector<unsigned char> data(cipher_text.get(), cipher_text.get() + c_len + f_len);
    std::string res = OpenSSL_Base64::encode(data);

    return res;
}
//-----------------------------------------------------------------------------
std::string
OpenSSL_AES::decrypt(std::string cipher_text)
{
    int f_len = 0;
    std::vector<unsigned char> text = OpenSSL_Base64::decode(cipher_text);
    int p_len = static_cast<int>(text.size());
    std::unique_ptr<unsigned char[]> plain_text(new unsigned char[p_len]);

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_DecryptInit_ex(dec_cipher_context, nullptr, nullptr, nullptr, nullptr);
    EVP_DecryptUpdate(dec_cipher_context, plain_text.get(), &p_len, text.data(), p_len);
    EVP_DecryptFinal_ex(dec_cipher_context, plain_text.get() + p_len, &f_len);
#else
    EVP_DecryptInit_ex(&dec_cipher_context, nullptr, nullptr, nullptr, nullptr);
    EVP_DecryptUpdate(&dec_cipher_context, plain_text.get(), &p_len, text.data(), p_len);
    EVP_DecryptFinal_ex(&dec_cipher_context, plain_text.get() + p_len, &f_len);
#endif

    unsigned long src_len = static_cast<unsigned long>(p_len + f_len);
    unsigned long dest_len = src_len * 4;
    std::unique_ptr<unsigned char[]> uncompressed(new unsigned char[dest_len]);

    if (_use_compression) {
        int err = uncompress(uncompressed.get(), &dest_len, plain_text.get(), src_len);
        if (err != Z_OK) {
            return {};
        }
    } else {
        memcpy(uncompressed.get(), plain_text.get(), src_len);
    }

    std::string res(reinterpret_cast<char*>(uncompressed.get()));

    return res;
}

//-----------------------------------------------------------------------------
