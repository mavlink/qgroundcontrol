/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief  ESP8266 WiFi Config Qml Controller
///     @author Gus Grubba <gus@auterion.com>

#include "ESP8266ComponentController.h"
#include "QGCApplication.h"
#include "UAS.h"
#include "ParameterManager.h"

#include <QHostAddress>
#include <QtEndian>

QGC_LOGGING_CATEGORY(ESP8266ComponentControllerLog, "ESP8266ComponentControllerLog")

#define MAX_RETRIES 5

//-----------------------------------------------------------------------------
ESP8266ComponentController::ESP8266ComponentController()
    : _waitType(WAIT_FOR_NOTHING)
    , _retries(0)
{
    for(int i = 1; i < 12; i++) {
        _channels.append(QString::number(i));
    }
    _baudRates.append(QStringLiteral("57600"));
    _baudRates.append(QStringLiteral("115200"));
    _baudRates.append(QStringLiteral("230400"));
    _baudRates.append(QStringLiteral("460800"));
    _baudRates.append(QStringLiteral("921600"));
    connect(_vehicle, &Vehicle::mavCommandResult, this, &ESP8266ComponentController::_mavCommandResult);
    Fact* ssid = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID4"));
    connect(ssid, &Fact::valueChanged, this, &ESP8266ComponentController::_ssidChanged);
    Fact* paswd = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD4"));
    connect(paswd, &Fact::valueChanged, this, &ESP8266ComponentController::_passwordChanged);
    Fact* baud = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("UART_BAUDRATE"));
    connect(baud, &Fact::valueChanged, this, &ESP8266ComponentController::_baudChanged);
    Fact* ver = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("SW_VER"));
    connect(ver, &Fact::valueChanged, this, &ESP8266ComponentController::_versionChanged);
}

//-----------------------------------------------------------------------------
ESP8266ComponentController::~ESP8266ComponentController()
{

}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::version()
{
    uint32_t uv = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("SW_VER"))->rawValue().toUInt();
    QString versionString = QString("%1.%2.%3").arg(uv >> 24).arg((uv >> 16) & 0xFF).arg(uv & 0xFFFF);
    return versionString;
}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::wifiIPAddress()
{
    if(_ipAddress.isEmpty()) {
        if(parameterExists(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_IPADDRESS"))) {
            QHostAddress address(qFromBigEndian(getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_IPADDRESS"))->rawValue().toUInt()));
            _ipAddress = address.toString();
        } else {
            _ipAddress = "192.168.4.1";
        }
    }
    return _ipAddress;
}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::wifiSSID()
{
    uint32_t s1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID1"))->rawValue().toUInt();
    uint32_t s2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID2"))->rawValue().toUInt();
    uint32_t s3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID3"))->rawValue().toUInt();
    uint32_t s4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID4"))->rawValue().toUInt();
    char tmp[20];
    memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    memcpy(&tmp[12], &s4, sizeof(uint32_t));
    return QString(tmp);
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::setWifiSSID(QString ssid)
{
    char tmp[20];
    memset(tmp, 0, sizeof(tmp));
    std::string	sid = ssid.toStdString();
    strncpy(tmp, sid.c_str(), sizeof(tmp));
    Fact* f1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID1"));
    Fact* f2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID2"));
    Fact* f3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID3"));
    Fact* f4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSID4"));
    uint32_t u;
    memcpy(&u, &tmp[0], sizeof(uint32_t));
    f1->setRawValue(QVariant(u));
    memcpy(&u, &tmp[4], sizeof(uint32_t));
    f2->setRawValue(QVariant(u));
    memcpy(&u, &tmp[8], sizeof(uint32_t));
    f3->setRawValue(QVariant(u));
    memcpy(&u, &tmp[12], sizeof(uint32_t));
    f4->setRawValue(QVariant(u));
}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::wifiPassword()
{
    uint32_t s1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD1"))->rawValue().toUInt();
    uint32_t s2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD2"))->rawValue().toUInt();
    uint32_t s3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD3"))->rawValue().toUInt();
    uint32_t s4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD4"))->rawValue().toUInt();
    char tmp[20];
    memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    memcpy(&tmp[12], &s4, sizeof(uint32_t));
    return QString(tmp);
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::setWifiPassword(QString password)
{
    char tmp[20];
    memset(tmp, 0, sizeof(tmp));
    std::string	pwd = password.toStdString();
    strncpy(tmp, pwd.c_str(), sizeof(tmp));
    Fact* f1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD1"));
    Fact* f2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD2"));
    Fact* f3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD3"));
    Fact* f4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PASSWORD4"));
    uint32_t u;
    memcpy(&u, &tmp[0], sizeof(uint32_t));
    f1->setRawValue(QVariant(u));
    memcpy(&u, &tmp[4], sizeof(uint32_t));
    f2->setRawValue(QVariant(u));
    memcpy(&u, &tmp[8], sizeof(uint32_t));
    f3->setRawValue(QVariant(u));
    memcpy(&u, &tmp[12], sizeof(uint32_t));
    f4->setRawValue(QVariant(u));
}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::wifiSSIDSta()
{
    if(!parameterExists(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSIDSTA1")) {
        return QString();
    }
    uint32_t s1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSIDSTA1"))->rawValue().toUInt();
    uint32_t s2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSIDSTA2"))->rawValue().toUInt();
    uint32_t s3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSIDSTA3"))->rawValue().toUInt();
    uint32_t s4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSIDSTA4"))->rawValue().toUInt();
    char tmp[20];
    memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    memcpy(&tmp[12], &s4, sizeof(uint32_t));
    return QString(tmp);
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::setWifiSSIDSta(QString ssid)
{
    if(parameterExists(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSIDSTA1")) {
        char tmp[20];
        memset(tmp, 0, sizeof(tmp));
        std::string	sid = ssid.toStdString();
        strncpy(tmp, sid.c_str(), sizeof(tmp));
        Fact* f1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSIDSTA1"));
        Fact* f2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSIDSTA2"));
        Fact* f3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSIDSTA3"));
        Fact* f4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_SSIDSTA4"));
        uint32_t u;
        memcpy(&u, &tmp[0], sizeof(uint32_t));
        f1->setRawValue(QVariant(u));
        memcpy(&u, &tmp[4], sizeof(uint32_t));
        f2->setRawValue(QVariant(u));
        memcpy(&u, &tmp[8], sizeof(uint32_t));
        f3->setRawValue(QVariant(u));
        memcpy(&u, &tmp[12], sizeof(uint32_t));
        f4->setRawValue(QVariant(u));
    }
}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::wifiPasswordSta()
{
    if(!parameterExists(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA1"))) {
        return QString();
    }
    uint32_t s1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA1"))->rawValue().toUInt();
    uint32_t s2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA2"))->rawValue().toUInt();
    uint32_t s3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA3"))->rawValue().toUInt();
    uint32_t s4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA4"))->rawValue().toUInt();
    char tmp[20];
    memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    memcpy(&tmp[12], &s4, sizeof(uint32_t));
    return QString(tmp);
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::setWifiPasswordSta(QString password)
{
    if(parameterExists(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA1"))) {
        char tmp[20];
        memset(tmp, 0, sizeof(tmp));
        std::string	pwd = password.toStdString();
        strncpy(tmp, pwd.c_str(), sizeof(tmp));
        Fact* f1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA1"));
        Fact* f2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA2"));
        Fact* f3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA3"));
        Fact* f4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("WIFI_PWDSTA4"));
        uint32_t u;
        memcpy(&u, &tmp[0], sizeof(uint32_t));
        f1->setRawValue(QVariant(u));
        memcpy(&u, &tmp[4], sizeof(uint32_t));
        f2->setRawValue(QVariant(u));
        memcpy(&u, &tmp[8], sizeof(uint32_t));
        f3->setRawValue(QVariant(u));
        memcpy(&u, &tmp[12], sizeof(uint32_t));
        f4->setRawValue(QVariant(u));
    }
}

//-----------------------------------------------------------------------------
int
ESP8266ComponentController::baudIndex()
{
    int b = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("UART_BAUDRATE"))->rawValue().toInt();
    switch (b) {
        case 57600:
            return 0;
        case 115200:
            return 1;
        case 230400:
            return 2;
        case 460800:
            return 3;
        case 921600:
        default:
            return 4;
    }
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::setBaudIndex(int idx)
{
    if(idx >= 0 && idx != baudIndex()) {
        int baud = 921600;
        switch(idx) {
            case 0:
                baud = 57600;
                break;
            case 1:
                baud = 115200;
                break;
            case 2:
                baud = 230400;
                break;
            case 3:
                baud = 460800;
                break;
            default:
                baud = 921600;
        }
        Fact* b = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, QStringLiteral("UART_BAUDRATE"));
        b->setRawValue(baud);
    }
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::reboot()
{
    _waitType = WAIT_FOR_REBOOT;
    emit busyChanged();
    _retries  = MAX_RETRIES;
    _reboot();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::restoreDefaults()
{
    _waitType = WAIT_FOR_RESTORE;
    emit busyChanged();
    _retries  = MAX_RETRIES;
    _restoreDefaults();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_reboot()
{
    _vehicle->sendMavCommand(MAV_COMP_ID_UDP_BRIDGE, MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, true /* showError */, 0.0f, 1.0f);
    qCDebug(ESP8266ComponentControllerLog) << "_reboot()";
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_restoreDefaults()
{
    _vehicle->sendMavCommand(MAV_COMP_ID_UDP_BRIDGE, MAV_CMD_PREFLIGHT_STORAGE, true /* showError */, 2.0f);
    qCDebug(ESP8266ComponentControllerLog) << "_restoreDefaults()";
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(noReponseFromVehicle);

    if (component == MAV_COMP_ID_UDP_BRIDGE) {
        if (result != MAV_RESULT_ACCEPTED) {
            qWarning() << "ESP8266ComponentController command" << command << "rejected.";
            return;
        }
        if ((_waitType == WAIT_FOR_REBOOT  && command == MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN) ||
                (_waitType == WAIT_FOR_RESTORE && command == MAV_CMD_PREFLIGHT_STORAGE)) {
            _waitType = WAIT_FOR_NOTHING;
            emit busyChanged();
            qCDebug(ESP8266ComponentControllerLog) << "_commandAck for" << command;
            if (command == MAV_CMD_PREFLIGHT_STORAGE) {
                _vehicle->parameterManager()->refreshAllParameters(MAV_COMP_ID_UDP_BRIDGE);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_ssidChanged(QVariant)
{
    emit wifiSSIDChanged();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_passwordChanged(QVariant)
{
    emit wifiPasswordChanged();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_baudChanged(QVariant)
{
    emit baudIndexChanged();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_versionChanged(QVariant)
{
    emit versionChanged();
}
