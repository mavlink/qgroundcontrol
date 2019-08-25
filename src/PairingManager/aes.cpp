#include "aes.h"

#include <memory>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <zlib.h>

//-----------------------------------------------------------------------------
AES::AES(std::string password, unsigned long long salt)
{
    int nrounds = 5;
    unsigned char key[32], iv[32];

    /*
     * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
     * nrounds is the number of times the we hash the material. More rounds are more secure but
     * slower.
     */
    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(),
                   reinterpret_cast<const unsigned char*>(&salt),
                   reinterpret_cast<const unsigned char*>(password.c_str()),
                   static_cast<int>(password.length()),
                   nrounds, key, iv);

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    encCipherContext = EVP_CIPHER_CTX_new();
    decCipherContext = EVP_CIPHER_CTX_new();

    EVP_CIPHER_CTX_init(encCipherContext);
    EVP_EncryptInit_ex(encCipherContext, EVP_aes_256_cbc(), nullptr, key, iv);
    EVP_CIPHER_CTX_init(decCipherContext);
    EVP_DecryptInit_ex(decCipherContext, EVP_aes_256_cbc(), nullptr, key, iv);
#else
    EVP_CIPHER_CTX_init(&encCipherContext);
    EVP_EncryptInit_ex(&encCipherContext, EVP_aes_256_cbc(), nullptr, key, iv);
    EVP_CIPHER_CTX_init(&decCipherContext);
    EVP_DecryptInit_ex(&decCipherContext, EVP_aes_256_cbc(), nullptr, key, iv);
#endif
}

//-----------------------------------------------------------------------------
AES::~AES()
{
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_CIPHER_CTX_free(encCipherContext);
    EVP_CIPHER_CTX_free(decCipherContext);
#else
    EVP_CIPHER_CTX_cleanup(&encCipherContext);
    EVP_CIPHER_CTX_cleanup(&decCipherContext);
#endif
}

//-----------------------------------------------------------------------------
std::string
AES::encrypt(std::string plainText)
{
    unsigned long sourceLen = static_cast<unsigned long>(plainText.length() + 1);
    unsigned long destLen = sourceLen * 2;
    unsigned char* compressed = new unsigned char[destLen];
    int err = compress2(compressed, &destLen,
                        reinterpret_cast<const unsigned char *>(plainText.c_str()),
                        sourceLen, 9);
    if (err != Z_OK) {
        return {};
    }

    int pLen = static_cast<int>(destLen);
    int cLen = pLen + AES_BLOCK_SIZE;
    int fLen = 0;
    unsigned char* cipherText = new unsigned char[cLen];

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_EncryptInit_ex(encCipherContext, nullptr, nullptr, nullptr, nullptr);
    EVP_EncryptUpdate(encCipherContext, cipherText, &cLen, compressed, pLen);
    EVP_EncryptFinal_ex(encCipherContext, cipherText + cLen, &fLen);
#else
    EVP_EncryptInit_ex(&encCipherContext, nullptr, nullptr, nullptr, nullptr);
    EVP_EncryptUpdate(&encCipherContext, cipherText, &cLen, compressed, pLen);
    EVP_EncryptFinal_ex(&encCipherContext, cipherText + cLen, &fLen);
#endif

    std::vector<unsigned char> data(cipherText, cipherText + cLen + fLen);
    std::string res = base64Encode(data);
    delete[] cipherText;
    delete[] compressed;

    return res;
}
//-----------------------------------------------------------------------------
std::string
AES::decrypt(std::string cipherText)
{
    int fLen = 0;
    std::vector<unsigned char> text = base64Decode(cipherText);
    int pLen = static_cast<int>(text.size());
    unsigned char* plainText = new unsigned char[pLen];

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    EVP_DecryptInit_ex(decCipherContext, nullptr, nullptr, nullptr, nullptr);
    EVP_DecryptUpdate(decCipherContext, plainText, &pLen, text.data(), pLen);
    EVP_DecryptFinal_ex(decCipherContext, plainText + pLen, &fLen);
#else
    EVP_DecryptInit_ex(&decCipherContext, nullptr, nullptr, nullptr, nullptr);
    EVP_DecryptUpdate(&decCipherContext, plainText, &pLen, text.data(), pLen);
    EVP_DecryptFinal_ex(&decCipherContext, plainText + pLen, &fLen);
#endif

    unsigned long destLen = static_cast<unsigned long>((pLen + fLen) * 2);
    unsigned char* uncompressed = new unsigned char[destLen];
    int err = uncompress(uncompressed, &destLen, plainText, static_cast<unsigned long>(pLen + fLen));
    if (err != Z_OK) {
        return {};
    }

    std::string res(reinterpret_cast<char*>(uncompressed));
    delete[] uncompressed;

    return res;
}

//-----------------------------------------------------------------------------
struct BIOFreeAll { void operator()(BIO* p) { BIO_free_all(p); } };

std::string
AES::base64Encode(const std::vector<unsigned char>& binary)
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
AES::base64Decode(std::string encoded)
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
