#include "UomRegistrationCrypto.h"

#include <array>
#include <memory>
#include <openssl/crypto.h>
#include <openssl/evp.h>

namespace UomRegistration {
namespace {
bool validHex(const QByteArray& hex)
{
    if (hex.isEmpty() || hex.size() % 2 != 0) {
        return false;
    }
    for (const char c : hex) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    return true;
}

std::optional<QByteArray> transform(const QByteArray& input, const QString& keyHex, bool encrypting)
{
    if (!validKey(keyHex) || input.isEmpty() || input.size() > 65536) {
        return std::nullopt;
    }
    QByteArray key = QByteArray::fromHex(keyHex.toLatin1());
    const std::array<unsigned char, 16> iv{};  // UOM Appendix A specifies the all-zero IV.
    const std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> context(EVP_CIPHER_CTX_new(),
                                                                                  EVP_CIPHER_CTX_free);
    const bool initialized = context && EVP_CipherInit_ex(context.get(), EVP_sm4_cbc(), nullptr,
                                                          reinterpret_cast<const unsigned char*>(key.constData()),
                                                          iv.data(), encrypting ? 1 : 0) == 1;
    OPENSSL_cleanse(key.data(), size_t(key.size()));
    if (!initialized) {
        return std::nullopt;
    }
    QByteArray output(input.size() + EVP_MAX_BLOCK_LENGTH, '\0');
    int written = 0;
    int finalBytes = 0;
    if (EVP_CipherUpdate(context.get(), reinterpret_cast<unsigned char*>(output.data()), &written,
                         reinterpret_cast<const unsigned char*>(input.constData()), int(input.size())) != 1 ||
        EVP_CipherFinal_ex(context.get(), reinterpret_cast<unsigned char*>(output.data()) + written, &finalBytes) !=
            1) {
        OPENSSL_cleanse(output.data(), size_t(output.size()));
        return std::nullopt;
    }
    output.resize(written + finalBytes);
    return output;
}
}  // namespace

bool validKey(const QString& keyHex)
{
    return keyHex.size() == 32 && validHex(keyHex.toLatin1());
}

std::optional<QByteArray> encrypt(const QByteArray& plaintext, const QString& keyHex)
{
    const auto encrypted = transform(plaintext, keyHex, true);
    return encrypted ? std::optional<QByteArray>(encrypted->toHex()) : std::nullopt;
}

std::optional<QByteArray> decrypt(const QByteArray& cipherHex, const QString& keyHex)
{
    if (!validHex(cipherHex) || cipherHex.size() > 131072 || cipherHex.size() % 32 != 0) {
        return std::nullopt;
    }
    return transform(QByteArray::fromHex(cipherHex), keyHex, false);
}
}  // namespace UomRegistration
