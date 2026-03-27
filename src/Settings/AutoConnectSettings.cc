#include "AutoConnectSettings.h"
#include "LinkManager.h"

DECLARE_SETTINGGROUP(AutoConnect, "AutoConnect")
{
    // Settings group name was changed from "LinkManager" to "AutoConnect" in v5.0.0
    // Copy over an old settings to the new name
    QSettings settings;
    static const char* deprecatedGroupName = "LinkManager";
    if (settings.childGroups().contains(deprecatedGroupName)) {
        settings.beginGroup(deprecatedGroupName);
        QList<QPair<QString, QVariant>> values;
        for (const QString& key: settings.childKeys()) {
            values.append(QPair<QString, QVariant>(key, settings.value(key)));
        }
        settings.endGroup();
        settings.remove(deprecatedGroupName);

        settings.beginGroup(_name);
        for (const QPair<QString, QVariant>& pair: values) {
            settings.setValue(pair.first, pair.second);
        }
        settings.endGroup();
    }
}

DECLARE_SETTINGSFACT(AutoConnectSettings, autoConnectUDP)
DECLARE_SETTINGSFACT(AutoConnectSettings, udpListenPort)
DECLARE_SETTINGSFACT(AutoConnectSettings, udpTargetHostIP)
DECLARE_SETTINGSFACT(AutoConnectSettings, udpTargetHostPort)
DECLARE_SETTINGSFACT(AutoConnectSettings, nmeaUdpPort)

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectPixhawk)
{
    if (!_autoConnectPixhawkFact) {
        _autoConnectPixhawkFact = _createSettingsFact(autoConnectPixhawkName);
#ifdef Q_OS_IOS
        _autoConnectPixhawkFact->setUserVisible(false);
#endif
    }
    return _autoConnectPixhawkFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectSiKRadio)
{
    if (!_autoConnectSiKRadioFact) {
        _autoConnectSiKRadioFact = _createSettingsFact(autoConnectSiKRadioName);
#ifdef Q_OS_IOS
        _autoConnectSiKRadioFact->setUserVisible(false);
#endif
    }
    return _autoConnectSiKRadioFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectRTKGPS)
{
    if (!_autoConnectRTKGPSFact) {
        _autoConnectRTKGPSFact = _createSettingsFact(autoConnectRTKGPSName);
#ifdef Q_OS_IOS
        _autoConnectRTKGPSFact->setUserVisible(false);
#endif
    }
    return _autoConnectRTKGPSFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectLibrePilot)
{
    if (!_autoConnectLibrePilotFact) {
        _autoConnectLibrePilotFact = _createSettingsFact(autoConnectLibrePilotName);
#ifdef Q_OS_IOS
        _autoConnectLibrePilotFact->setUserVisible(false);
#endif
    }
    return _autoConnectLibrePilotFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectNmeaPort)
{
    if (!_autoConnectNmeaPortFact) {
        _autoConnectNmeaPortFact = _createSettingsFact(autoConnectNmeaPortName);
#ifdef Q_OS_IOS
        _autoConnectNmeaPortFact->setUserVisible(false);
#endif
    }
    return _autoConnectNmeaPortFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectNmeaBaud)
{
    if (!_autoConnectNmeaBaudFact) {
        _autoConnectNmeaBaudFact = _createSettingsFact(autoConnectNmeaBaudName);
#ifdef Q_OS_IOS
        _autoConnectNmeaBaudFact->setUserVisible(false);
#endif
    }
    return _autoConnectNmeaBaudFact;
}


