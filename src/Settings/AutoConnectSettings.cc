/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AutoConnectSettings.h"
#include "LinkManager.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(AutoConnect, "LinkManager")
{
    qmlRegisterUncreatableType<AutoConnectSettings>("QGroundControl.SettingsManager", 1, 0, "AutoConnectSettings", "Reference only"); \
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
#ifdef __ios__
        _autoConnectPixhawkFact->setVisible(false);
#endif
    }
    return _autoConnectPixhawkFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectSiKRadio)
{
    if (!_autoConnectSiKRadioFact) {
        _autoConnectSiKRadioFact = _createSettingsFact(autoConnectSiKRadioName);
#ifdef __ios__
        _autoConnectSiKRadioFact->setVisible(false);
#endif
    }
    return _autoConnectSiKRadioFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectPX4Flow)
{
    if (!_autoConnectPX4FlowFact) {
        _autoConnectPX4FlowFact = _createSettingsFact(autoConnectPX4FlowName);
#ifdef __ios__
        _autoConnectPX4FlowFact->setVisible(false);
#endif
    }
    return _autoConnectPX4FlowFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectRTKGPS)
{
    if (!_autoConnectRTKGPSFact) {
        _autoConnectRTKGPSFact = _createSettingsFact(autoConnectRTKGPSName);
#ifdef __ios__
        _autoConnectRTKGPSFact->setVisible(false);
#endif
    }
    return _autoConnectRTKGPSFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectLibrePilot)
{
    if (!_autoConnectLibrePilotFact) {
        _autoConnectLibrePilotFact = _createSettingsFact(autoConnectLibrePilotName);
#ifdef __ios__
        _autoConnectLibrePilotFact->setVisible(false);
#endif
    }
    return _autoConnectLibrePilotFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectNmeaPort)
{
    if (!_autoConnectNmeaPortFact) {
        _autoConnectNmeaPortFact = _createSettingsFact(autoConnectNmeaPortName);
#ifdef __ios__
        _autoConnectNmeaPortFact->setVisible(false);
#endif
    }
    return _autoConnectNmeaPortFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectNmeaBaud)
{
    if (!_autoConnectNmeaBaudFact) {
        _autoConnectNmeaBaudFact = _createSettingsFact(autoConnectNmeaBaudName);
#ifdef __ios__
        _autoConnectNmeaBaudFact->setVisible(false);
#endif
    }
    return _autoConnectNmeaBaudFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AutoConnectSettings, autoConnectZeroConf)
{
    if (!_autoConnectZeroConfFact) {
        _autoConnectZeroConfFact = _createSettingsFact(autoConnectZeroConfName);
#ifdef __ios__
        _autoConnectZeroConfFact->setVisible(false);
#endif
    }
    return _autoConnectZeroConfFact;
}
