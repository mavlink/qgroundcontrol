#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class NTRIPSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit NTRIPSettings(QObject* parent = nullptr);

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
    /// connection, cleared when self-signed certificates are no longer accepted.
    DEFINE_SETTINGFACT(ntripPinnedCertificate)
    DEFINE_SETTINGFACT(ntripGgaPositionSource)
    DEFINE_SETTINGFACT(ntripGgaIntervalSec)
    /// Not shown in the UI: the URLs of the casters found to take NTRIP version 1 requests only, the latest last, so
    /// that they are asked in that form at once after a restart. A caster is taken out when it no longer accepts
    /// such a request.
    DEFINE_SETTINGFACT(ntripVersion1Casters)
};
