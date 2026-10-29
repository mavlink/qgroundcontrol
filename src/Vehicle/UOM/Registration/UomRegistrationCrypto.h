#pragma once

#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace UomRegistration {
[[nodiscard]] bool validKey(const QString& keyHex);
/// GB 46761 Appendix A: SM4-CBC with a zero IV, PKCS#7 padding, and hexadecimal transport encoding.
[[nodiscard]] std::optional<QByteArray> encrypt(const QByteArray& plaintext, const QString& keyHex);
[[nodiscard]] std::optional<QByteArray> decrypt(const QByteArray& cipherHex, const QString& keyHex);
}  // namespace UomRegistration
