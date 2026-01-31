#include "ESP8266ComponentController.h"
#include "ESP8266StateMachine.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtNetwork/QHostAddress>

QGC_LOGGING_CATEGORY(ESP8266ComponentControllerLog, "AutoPilotPlugins.ESP8266ComponentController")

ESP8266ComponentController::ESP8266ComponentController(QObject *parent)
    : FactPanelController(parent)
    , _baud(getParameterFact(componentID(), QStringLiteral("UART_BAUDRATE")))
    , _ver(getParameterFact(componentID(), QStringLiteral("SW_VER")))
    , _ssid1(getParameterFact(componentID(), QStringLiteral("WIFI_SSID1")))
    , _ssid2(getParameterFact(componentID(), QStringLiteral("WIFI_SSID2")))
    , _ssid3(getParameterFact(componentID(), QStringLiteral("WIFI_SSID3")))
    , _ssid4(getParameterFact(componentID(), QStringLiteral("WIFI_SSID4")))
    , _pwd1(getParameterFact(componentID(), QStringLiteral("WIFI_PASSWORD1")))
    , _pwd2(getParameterFact(componentID(), QStringLiteral("WIFI_PASSWORD2")))
    , _pwd3(getParameterFact(componentID(), QStringLiteral("WIFI_PASSWORD3")))
    , _pwd4(getParameterFact(componentID(), QStringLiteral("WIFI_PASSWORD4")))
    , _ssidsta1(getParameterFact(componentID(), QStringLiteral("WIFI_SSIDSTA1"), false))
    , _ssidsta2(getParameterFact(componentID(), QStringLiteral("WIFI_SSIDSTA2"), false))
    , _ssidsta3(getParameterFact(componentID(), QStringLiteral("WIFI_SSIDSTA3"), false))
    , _ssidsta4(getParameterFact(componentID(), QStringLiteral("WIFI_SSIDSTA4"), false))
    , _pwdsta1(getParameterFact(componentID(), QStringLiteral("WIFI_PWDSTA1"), false))
    , _pwdsta2(getParameterFact(componentID(), QStringLiteral("WIFI_PWDSTA2"), false))
    , _pwdsta3(getParameterFact(componentID(), QStringLiteral("WIFI_PWDSTA3"), false))
    , _pwdsta4(getParameterFact(componentID(), QStringLiteral("WIFI_PWDSTA4"), false))
{
    for (int i = 1; i < 12; i++) {
        _channels.append(QString::number(i));
    }

    // Create state machine
    _stateMachine = new ESP8266StateMachine(this, this);
    (void) connect(_stateMachine, &ESP8266StateMachine::busyChanged, this, &ESP8266ComponentController::busyChanged);

    (void) connect(_vehicle, &Vehicle::mavCommandResult, this, &ESP8266ComponentController::_mavCommandResult);
    (void) connect(_ssid4, &Fact::valueChanged, this, &ESP8266ComponentController::_ssidChanged);
    (void) connect(_pwd4, &Fact::valueChanged, this, &ESP8266ComponentController::_passwordChanged);
    (void) connect(_baud, &Fact::valueChanged, this, &ESP8266ComponentController::_baudChanged);
    (void) connect(_ver, &Fact::valueChanged, this, &ESP8266ComponentController::_versionChanged);
}

ESP8266ComponentController::~ESP8266ComponentController()
{
    // qCDebug(RadioComponentControllerLog) << Q_FUNC_INFO << this;
}

QString ESP8266ComponentController::version() const
{
    const uint32_t uv = getParameterFact(componentID(), QStringLiteral("SW_VER"))->rawValue().toUInt();
    const QString versionString = QStringLiteral("%1.%2.%3").arg(uv >> 24).arg((uv >> 16) & 0xFF).arg(uv & 0xFFFF);
    return versionString;
}

QString ESP8266ComponentController::wifiIPAddress()
{
    if (_ipAddress.isEmpty()) {
        if (parameterExists(componentID(), QStringLiteral("WIFI_IPADDRESS"))) {
            const QHostAddress address(qFromBigEndian(getParameterFact(componentID(), QStringLiteral("WIFI_IPADDRESS"))->rawValue().toUInt()));
            _ipAddress = address.toString();
        } else {
            _ipAddress = QStringLiteral("192.168.4.1");
        }
    }

    return _ipAddress;
}

QString ESP8266ComponentController::wifiSSID() const
{
    const uint32_t s1 = _ssid1->rawValue().toUInt();
    const uint32_t s2 = _ssid2->rawValue().toUInt();
    const uint32_t s3 = _ssid3->rawValue().toUInt();
    const uint32_t s4 = _ssid4->rawValue().toUInt();

    char tmp[20];
    (void) memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    (void) memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    (void) memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    (void) memcpy(&tmp[12], &s4, sizeof(uint32_t));

    return QString(tmp);
}

void ESP8266ComponentController::setWifiSSID(const QString &ssid) const
{
    char tmp[20];
    (void) memset(tmp, 0, sizeof(tmp));
    const std::string sid = ssid.toStdString();
    (void) strncpy(tmp, sid.c_str(), sizeof(tmp));

    uint32_t u;
    (void) memcpy(&u, &tmp[0], sizeof(uint32_t));
    _ssid1->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[4], sizeof(uint32_t));
    _ssid2->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[8], sizeof(uint32_t));
    _ssid3->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[12], sizeof(uint32_t));
    _ssid4->setRawValue(QVariant(u));
}

QString ESP8266ComponentController::wifiPassword() const
{
    const uint32_t s1 = _pwd1->rawValue().toUInt();
    const uint32_t s2 = _pwd2->rawValue().toUInt();
    const uint32_t s3 = _pwd3->rawValue().toUInt();
    const uint32_t s4 = _pwd4->rawValue().toUInt();

    char tmp[20];
    (void) memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    (void) memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    (void) memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    (void) memcpy(&tmp[12], &s4, sizeof(uint32_t));

    return QString(tmp);
}

void ESP8266ComponentController::setWifiPassword(const QString &password) const
{
    char tmp[20];
    (void) memset(tmp, 0, sizeof(tmp));
    const std::string pwd = password.toStdString();
    (void) strncpy(tmp, pwd.c_str(), sizeof(tmp));

    uint32_t u;
    (void) memcpy(&u, &tmp[0], sizeof(uint32_t));
    _pwd1->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[4], sizeof(uint32_t));
    _pwd2->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[8], sizeof(uint32_t));
    _pwd3->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[12], sizeof(uint32_t));
    _pwd4->setRawValue(QVariant(u));
}

QString ESP8266ComponentController::wifiSSIDSta() const
{
    if (!parameterExists(componentID(), "WIFI_SSIDSTA1")) {
        return QString();
    }

    const uint32_t s1 = _ssidsta1->rawValue().toUInt();
    const uint32_t s2 = _ssidsta2->rawValue().toUInt();
    const uint32_t s3 = _ssidsta3->rawValue().toUInt();
    const uint32_t s4 = _ssidsta4->rawValue().toUInt();

    char tmp[20];
    (void) memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    (void) memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    (void) memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    (void) memcpy(&tmp[12], &s4, sizeof(uint32_t));

    return QString(tmp);
}

void ESP8266ComponentController::setWifiSSIDSta(const QString &ssid) const
{
    if (!parameterExists(componentID(), "WIFI_SSIDSTA1")) {
        return;
    }

    char tmp[20];
    (void) memset(tmp, 0, sizeof(tmp));
    const std::string sid = ssid.toStdString();
    (void) strncpy(tmp, sid.c_str(), sizeof(tmp));

    uint32_t u;
    (void) memcpy(&u, &tmp[0], sizeof(uint32_t));
    _ssidsta1->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[4], sizeof(uint32_t));
    _ssidsta2->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[8], sizeof(uint32_t));
    _ssidsta3->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[12], sizeof(uint32_t));
    _ssidsta4->setRawValue(QVariant(u));
}

QString ESP8266ComponentController::wifiPasswordSta() const
{
    if (!parameterExists(componentID(), QStringLiteral("WIFI_PWDSTA1"))) {
        return QString();
    }

    const uint32_t s1 = _pwdsta1->rawValue().toUInt();
    const uint32_t s2 = _pwdsta2->rawValue().toUInt();
    const uint32_t s3 = _pwdsta3->rawValue().toUInt();
    const uint32_t s4 = _pwdsta4->rawValue().toUInt();

    char tmp[20];
    (void) memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    (void) memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    (void) memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    (void) memcpy(&tmp[12], &s4, sizeof(uint32_t));

    return QString(tmp);
}

void ESP8266ComponentController::setWifiPasswordSta(const QString &password) const
{
    if (!parameterExists(componentID(), QStringLiteral("WIFI_PWDSTA1"))) {
        return;
    }

    char tmp[20];
    (void) memset(tmp, 0, sizeof(tmp));
    const std::string pwd = password.toStdString();
    (void) strncpy(tmp, pwd.c_str(), sizeof(tmp));

    uint32_t u;
    (void) memcpy(&u, &tmp[0], sizeof(uint32_t));
    _pwdsta1->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[4], sizeof(uint32_t));
    _pwdsta2->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[8], sizeof(uint32_t));
    _pwdsta3->setRawValue(QVariant(u));
    (void) memcpy(&u, &tmp[12], sizeof(uint32_t));
    _pwdsta4->setRawValue(QVariant(u));
}

int ESP8266ComponentController::baudIndex() const
{
    const int b = getParameterFact(componentID(), QStringLiteral("UART_BAUDRATE"))->rawValue().toInt();
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

void ESP8266ComponentController::setBaudIndex(int idx) const
{
    if ((idx >= 0) && (idx != baudIndex())) {
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
        _baud->setRawValue(baud);
    }
}

bool ESP8266ComponentController::busy() const
{
    return _stateMachine->isBusy();
}

void ESP8266ComponentController::reboot()
{
    qCDebug(ESP8266ComponentControllerLog) << "reboot()";
    _stateMachine->startReboot();
}

void ESP8266ComponentController::restoreDefaults()
{
    qCDebug(ESP8266ComponentControllerLog) << "restoreDefaults()";
    _stateMachine->startRestore();
}

void ESP8266ComponentController::_mavCommandResult(int vehicleId, int component, int command, int result, int failureCode)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(failureCode);

    if (component != componentID()) {
        return;
    }

    // Delegate to state machine
    _stateMachine->handleCommandResult(command, result);
}
