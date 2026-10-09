#pragma once

#include <QtCore/QString>
#include <QtCore/QUrl>

struct NTRIPConnectionConfig
{
    QString host;
    int port = 2101;
    QString username;
    QString password;
    QString mountpoint;
    bool useTls = false;
    bool allowSelfSignedCerts = false;
    /// "host:port|sha256-hex" of the self-signed certificate trusted on first use; other endpoints ignore it.
    QString pinnedCertificate;

    bool operator==(const NTRIPConnectionConfig&) const = default;
    [[nodiscard]] QString validationError() const;
    [[nodiscard]] QString streamValidationError() const;
    /// The caster's HTTP(S) URL, with the mountpoint as its path when @a withMountpoint is set, else "/".
    [[nodiscard]] QUrl url(bool withMountpoint = true) const;
    /// Credentials go to the caster without TLS.
    [[nodiscard]] bool sendsCredentialsInClear() const;
    /// The user-visible warning when credentials go to the caster without TLS; empty otherwise.
    [[nodiscard]] QString credentialsInClearWarning() const;
};
