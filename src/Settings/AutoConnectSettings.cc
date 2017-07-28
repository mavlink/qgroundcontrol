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

const char* AutoConnectSettings::_settingsGroup =                       "LinkManager";

const char* AutoConnectSettings:: autoConnectUDPSettingsName =          "AutoconnectUDP";
const char* AutoConnectSettings:: autoConnectPixhawkSettingsName =      "AutoconnectPixhawk";
const char* AutoConnectSettings:: autoConnectSiKRadioSettingsName =     "Autoconnect3DRRadio";
const char* AutoConnectSettings:: autoConnectPX4FlowSettingsName =      "AutoconnectPX4Flow";
const char* AutoConnectSettings:: autoConnectRTKGPSSettingsName =       "AutoconnectRTKGPS";
const char* AutoConnectSettings:: autoConnectLibrePilotSettingsName =   "AutoconnectLibrePilot";
const char* AutoConnectSettings:: udpListenPortName =                   "AutoconnectUDPListenPort";
const char* AutoConnectSettings:: udpTargetHostIPName =                 "AutoconnectUDPTargetHostIP";
const char* AutoConnectSettings:: udpTargetHostPortName =               "AutoconnectUDPTargetHostPort";

const char* AutoConnectSettings::autoConnectSettingsGroupName = "AutoConnect";

AutoConnectSettings::AutoConnectSettings(QObject* parent)
    : SettingsGroup             (autoConnectSettingsGroupName, _settingsGroup, parent)
    , _autoConnectUDPFact       (NULL)
    , _autoConnectPixhawkFact   (NULL)
    , _autoConnectSiKRadioFact  (NULL)
    , _autoConnectPX4FlowFact   (NULL)
    , _autoConnectRTKGPSFact    (NULL)
    , _autoConnectLibrePilotFact(NULL)
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
    }

    return _autoConnectPixhawkFact;
}

Fact* AutoConnectSettings::autoConnectSiKRadio(void)
{
    if (!_autoConnectSiKRadioFact) {
        _autoConnectSiKRadioFact = _createSettingsFact(autoConnectSiKRadioSettingsName);
    }

    return _autoConnectSiKRadioFact;
}

Fact* AutoConnectSettings::autoConnectPX4Flow(void)
{
    if (!_autoConnectPX4FlowFact) {
        _autoConnectPX4FlowFact = _createSettingsFact(autoConnectPX4FlowSettingsName);
    }

    return _autoConnectPX4FlowFact;
}

Fact* AutoConnectSettings::autoConnectRTKGPS(void)
{
    if (!_autoConnectRTKGPSFact) {
        _autoConnectRTKGPSFact = _createSettingsFact(autoConnectRTKGPSSettingsName);
    }

    return _autoConnectRTKGPSFact;
}

Fact* AutoConnectSettings::autoConnectLibrePilot(void)
{
    if (!_autoConnectLibrePilotFact) {
        _autoConnectLibrePilotFact = _createSettingsFact(autoConnectLibrePilotSettingsName);
    }

    return _autoConnectLibrePilotFact;
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
