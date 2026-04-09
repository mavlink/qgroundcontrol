#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

/// Platform-abstracted secure credential storage.
/// Uses the OS keychain (macOS Keychain, Windows Credential Manager, Linux libsecret)
/// with automatic insecure fallback for headless environments.
/// All methods are synchronous (blocking via nested QEventLoop).
namespace QGCKeychain
{
    bool writeBinary(const QString& key, const QByteArray& data);
    QByteArray readBinary(const QString& key);
    bool remove(const QString& key);
}
