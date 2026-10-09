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
    DEFINE_SETTINGFACT(ntripWhitelist)
    DEFINE_SETTINGFACT(ntripUseTls)
    DEFINE_SETTINGFACT(ntripAllowSelfSignedCerts)
    /// Not shown in the UI: host:port and SHA-256 digest of the self-signed caster certificate trusted on first
    /// connection, cleared when self-signed certificates are no longer accepted.
    DEFINE_SETTINGFACT(ntripPinnedCertificate)
    DEFINE_SETTINGFACT(ntripGgaPositionSource)
    DEFINE_SETTINGFACT(ntripGgaIntervalSec)
};
