/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AutoConnectSettings.h"
#include "LinkManager.h"

#include <QQmlEngine>
#include <QtQml>

const char* AutoConnectSettings::name =                                 "AutoConnect";
const char* AutoConnectSettings::settingsGroup =                        "LinkManager";

const char* AutoConnectSettings:: autoConnectUDPSettingsName =          "AutoconnectUDP";
const char* AutoConnectSettings:: autoConnectPixhawkSettingsName =      "AutoconnectPixhawk";
const char* AutoConnectSettings:: autoConnectSiKRadioSettingsName =     "Autoconnect3DRRadio";
const char* AutoConnectSettings:: autoConnectPX4FlowSettingsName =      "AutoconnectPX4Flow";
const char* AutoConnectSettings:: autoConnectRTKGPSSettingsName =       "AutoconnectRTKGPS";
const char* AutoConnectSettings:: autoConnectLibrePilotSettingsName =   "AutoconnectLibrePilot";
const char* AutoConnectSettings:: autoConnectNmeaPortName =             "AutoconnectNmeaPort";
const char* AutoConnectSettings:: autoConnectNmeaBaudName =             "AutoconnectNmeaBaud";
const char* AutoConnectSettings:: udpListenPortName =                   "AutoconnectUDPListenPort";
const char* AutoConnectSettings:: udpTargetHostIPName =                 "AutoconnectUDPTargetHostIP";
const char* AutoConnectSettings:: udpTargetHostPortName =               "AutoconnectUDPTargetHostPort";


AutoConnectSettings::AutoConnectSettings(QObject* parent)
    : SettingsGroup             (name, settingsGroup, parent)
    , _autoConnectUDPFact       (NULL)
    , _autoConnectPixhawkFact   (NULL)
    , _autoConnectSiKRadioFact  (NULL)
    , _autoConnectPX4FlowFact   (NULL)
    , _autoConnectRTKGPSFact    (NULL)
    , _autoConnectLibrePilotFact(NULL)
    , _autoConnectNmeaPortFact  (NULL)
    , _autoConnectNmeaBaudFact  (NULL)
    , _udpListenPortFact        (NULL)
    , _udpTargetHostIPFact      (NULL)
    , _udpTargetHostPortFact    (NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<AutoConnectSettings>("QGroundControl.SettingsManager", 1, 0, "AutoConnectSettings", "Reference only");
}

Fact* AutoConnectSettings::autoConnectUDP(void)
{
    if (!_autoConnectUDPFact) {
        _autoConnectUDPFact = _createSettingsFact(autoConnectUDPSettingsName);
    }

    return _autoConnectUDPFact;
}

Fact* AutoConnectSettings::autoConnectPixhawk(void)
{
    if (!_autoConnectPixhawkFact) {
        _autoConnectPixhawkFact = _createSettingsFact(autoConnectPixhawkSettingsName);
#ifdef __ios__
        _autoConnectPixhawkFact->setVisible(false);
#endif
    }

    return _autoConnectPixhawkFact;
}

Fact* AutoConnectSettings::autoConnectSiKRadio(void)
{
    if (!_autoConnectSiKRadioFact) {
        _autoConnectSiKRadioFact = _createSettingsFact(autoConnectSiKRadioSettingsName);
#ifdef __ios__
        _autoConnectSiKRadioFact->setVisible(false);
#endif
    }

    return _autoConnectSiKRadioFact;
}

Fact* AutoConnectSettings::autoConnectPX4Flow(void)
{
    if (!_autoConnectPX4FlowFact) {
        _autoConnectPX4FlowFact = _createSettingsFact(autoConnectPX4FlowSettingsName);
#ifdef __ios__
        _autoConnectPX4FlowFact->setVisible(false);
#endif
    }

    return _autoConnectPX4FlowFact;
}

Fact* AutoConnectSettings::autoConnectRTKGPS(void)
{
    if (!_autoConnectRTKGPSFact) {
        _autoConnectRTKGPSFact = _createSettingsFact(autoConnectRTKGPSSettingsName);
#ifdef __ios__
        _autoConnectRTKGPSFact->setVisible(false);
#endif
    }

    return _autoConnectRTKGPSFact;
}

Fact* AutoConnectSettings::autoConnectLibrePilot(void)
{
    if (!_autoConnectLibrePilotFact) {
        _autoConnectLibrePilotFact = _createSettingsFact(autoConnectLibrePilotSettingsName);
#ifdef __ios__
        _autoConnectLibrePilotFact->setVisible(false);
#endif
    }

    return _autoConnectLibrePilotFact;
}

Fact* AutoConnectSettings::autoConnectNmeaPort(void)
{
    if (!_autoConnectNmeaPortFact) {
        _autoConnectNmeaPortFact = _createSettingsFact(autoConnectNmeaPortName);
#ifdef __ios__
        _autoConnectNmeaPortFact->setVisible(false);
#endif
    }

    return _autoConnectNmeaPortFact;
}

Fact* AutoConnectSettings::autoConnectNmeaBaud(void)
{
    if (!_autoConnectNmeaBaudFact) {
        _autoConnectNmeaBaudFact = _createSettingsFact(autoConnectNmeaBaudName);
#ifdef __ios__
        _autoConnectNmeaBaudFact->setVisible(false);
#endif
    }

    return _autoConnectNmeaBaudFact;
}

Fact* AutoConnectSettings::udpListenPort(void)
{
    if (!_udpListenPortFact) {
        _udpListenPortFact = _createSettingsFact(udpListenPortName);
    }

    return _udpListenPortFact;
}

Fact* AutoConnectSettings::udpTargetHostIP(void)
{
    if (!_udpTargetHostIPFact) {
        _udpTargetHostIPFact = _createSettingsFact(udpTargetHostIPName);
    }

    return _udpTargetHostIPFact;
}

Fact* AutoConnectSettings::udpTargetHostPort(void)
{
    if (!_udpTargetHostPortFact) {
        _udpTargetHostPortFact = _createSettingsFact(udpTargetHostPortName);
    }

    return _udpTargetHostPortFact;
}
