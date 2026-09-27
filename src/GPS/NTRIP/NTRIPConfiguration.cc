#include "NTRIPConfiguration.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>

namespace {
QString redactedPasswordState(const QString& password)
{
    return password.isEmpty() ? QStringLiteral("<empty>") : QStringLiteral("<set>");
}
}  // namespace

QDebug operator<<(QDebug debug, const NTRIPConnectionConfig& configuration)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "NTRIPConnectionConfig(host=" << configuration.host << ", port=" << configuration.port
                              << ", username=" << configuration.username
                              << ", password=" << redactedPasswordState(configuration.password)
                              << ", mountpoint=" << configuration.mountpoint << ", useTls=" << configuration.useTls
                              << ", allowSelfSignedCerts=" << configuration.allowSelfSignedCerts << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const NTRIPRtcmFilterConfig& configuration)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "NTRIPRtcmFilterConfig(whitelist=" << configuration.whitelist << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const NTRIPConfiguration& configuration)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "NTRIPConfiguration(connection=" << configuration.connection
                              << ", filter=" << configuration.filter << ')';
    return debug;
}

QString NTRIPConnectionConfig::validationError() const
{
    const auto tr = [](const char* s) { return QCoreApplication::translate("NTRIPConnectionConfig", s); };

    if (host.isEmpty()) {
        return tr("No host address");
    }
    if (port <= 0 || port > 65535) {
        return tr("Invalid port");
    }

    static const QRegularExpression controlChars(QStringLiteral("[\\x00-\\x1f\\x7f]"));
    if (host.contains(controlChars)) {
        return tr("Invalid host (contains control characters)");
    }
    if (host.contains(QLatin1Char(' '))) {
        return tr("Invalid host (contains spaces)");
    }
    if (!mountpoint.isEmpty() && mountpoint.contains(controlChars)) {
        return tr("Invalid mountpoint name (contains control characters)");
    }
    if (mountpoint.contains(QLatin1Char(' '))) {
        return tr("Invalid mountpoint name (contains spaces)");
    }
    // RFC 7617 forbids colons in Basic-auth usernames.
    if (username.contains(QLatin1Char(':'))) {
        return tr("Invalid username (must not contain ':')");
    }

    return QString();
}

QString NTRIPConnectionConfig::streamValidationError() const
{
    if (const QString error = validationError(); !error.isEmpty()) {
        return error;
    }
    if (mountpoint.trimmed().isEmpty()) {
        return QCoreApplication::translate("NTRIPConnectionConfig", "Select a mountpoint before connecting");
    }
    return {};
}

QVector<int> NTRIPRtcmFilterConfig::messageIds() const
{
    QVector<int> ids;
    if (whitelist.isEmpty()) {
        return ids;
    }
    for (const auto& token : whitelist.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        bool ok = false;
        const int id = token.trimmed().toInt(&ok);
        if (ok && id > 0) {
            ids.append(id);
        }
    }
    return ids;
}
