#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

/// A caster connection saved under a name, which the NTRIP connection settings can be filled from.
struct NTRIPSavedCaster
{
    Q_GADGET
    QML_VALUE_TYPE(ntripSavedCaster)
    Q_PROPERTY(QString name MEMBER name FINAL)
    Q_PROPERTY(QString host MEMBER host FINAL)
    Q_PROPERTY(int port MEMBER port FINAL)
    Q_PROPERTY(QString mountpoint MEMBER mountpoint FINAL)

public:
    QString name;
    QString host;
    int port = 2101;
    QString username;
    /// Stored as ntripPassword is, in the settings. QML does not read it.
    QString password;
    QString mountpoint;
    bool useTls = false;
    bool allowSelfSignedCerts = false;
    /// The self-signed certificate trusted for this caster when it was saved, as ntripPinnedCertificate holds it;
    /// empty when none was.
    QString pinnedCertificate;

    bool operator==(const NTRIPSavedCaster&) const = default;
};

class NTRIPSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    /// The saved casters, in the order they were first saved. The connection settings stay the caster that is
    /// connected to; useSavedCaster() copies a saved caster into them.
    Q_PROPERTY(QList<NTRIPSavedCaster> savedCasters READ savedCasters NOTIFY savedCastersChanged FINAL)

public:
    explicit NTRIPSettings(QObject* parent = nullptr);

    /// Limits of the saved casters, as an enum so that QML reads them from the type.
    enum SavedCasterLimit : int
    {
        MaxSavedCasters = 20,
        MaxSavedCasterNameLength = 64,
        /// Of the host, the user name, the password and the mountpoint.
        MaxSavedCasterTextLength = 255,
    };
    Q_ENUM(SavedCasterLimit)

    QList<NTRIPSavedCaster> savedCasters() const { return _savedCasters; }

    /// Saves @a caster under its name without surrounding spaces, replacing the caster of that name. Returns false,
    /// saving nothing, for a name that is empty or longer than MaxSavedCasterNameLength, a caster without a host or
    /// with a host that holds a space or a control character, a port outside 1 to 65535, a text longer than
    /// MaxSavedCasterTextLength, or a new name when MaxSavedCasters casters are saved. A pin that does not name the
    /// caster's host and port is not kept.
    bool saveCaster(const NTRIPSavedCaster& caster);
    /// Saves the caster of the connection settings under @a casterName, as saveCaster() does, with the trusted
    /// certificate when that is this caster's.
    Q_INVOKABLE bool saveConnectionAsCaster(const QString& casterName);
    /// Makes the caster saved under @a casterName the caster of the connection settings. Returns false, changing
    /// nothing, when no caster has that name and while the connection is turned on, as the page locks the settings
    /// it would write.
    ///
    /// The trusted certificate setting belongs to one host and port. It is kept when it is this caster's, and
    /// otherwise replaced by the one saved with the caster, or cleared: the certificate of the caster used before
    /// never goes on to another.
    Q_INVOKABLE bool useSavedCaster(const QString& casterName);
    /// Returns false when no caster has that name.
    Q_INVOKABLE bool removeSavedCaster(const QString& casterName);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(ntripServerConnectEnabled)
    DEFINE_SETTINGFACT(ntripServerHostAddress)
    DEFINE_SETTINGFACT(ntripServerPort)
    DEFINE_SETTINGFACT(ntripUsername)
    DEFINE_SETTINGFACT(ntripPassword)
    DEFINE_SETTINGFACT(ntripMountpoint)
    DEFINE_SETTINGFACT(ntripUseTls)
    DEFINE_SETTINGFACT(ntripAllowSelfSignedCerts)
    /// Not shown in the UI: host:port and SHA-256 digest of the self-signed caster certificate trusted on first
    /// connection, cleared when self-signed certificates are no longer accepted. An IPv6 literal is in brackets,
    /// "[fe80::1]:2101": a pin stored without them, as it once was, is rewritten when it is read.
    DEFINE_SETTINGFACT(ntripPinnedCertificate)
    DEFINE_SETTINGFACT(ntripGgaPositionSource)
    DEFINE_SETTINGFACT(ntripGgaIntervalSec)
    /// Not shown in the UI: the URLs of the casters found to take NTRIP version 1 requests only, the latest last, so
    /// that they are asked in that form at once after a restart. A caster is taken out when it no longer accepts
    /// such a request.
    DEFINE_SETTINGFACT(ntripVersion1Casters)
    /// Not shown in the UI: the saved casters as JSON, each with its password.
    DEFINE_SETTINGFACT(ntripSavedCasters)
    DEFINE_SETTINGFACT(ntripFailoverEnabled)
    DEFINE_SETTINGFACT(ntripRequireVehicle)
    DEFINE_SETTINGFACT(ntripDataCapMB)

signals:
    void savedCastersChanged();

private:
    /// Reads the casters from ntripSavedCasters, dropping the entries that are not valid.
    void _loadSavedCasters();
    /// Rewrites an ntripPinnedCertificate that names an IPv6 literal without brackets.
    void _bracketPinnedLiteral();
    void _storeSavedCasters(const QList<NTRIPSavedCaster>& casters);

    QList<NTRIPSavedCaster> _savedCasters;
};
