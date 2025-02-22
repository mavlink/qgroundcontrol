/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QVariant>

#include "FactPanelController.h"
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(ESP8266ComponentControllerLog)

class Fact;
class Vehicle;

class ESP8266ComponentController : public FactPanelController
{
    Q_OBJECT
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(int          componentID     READ componentID                                    CONSTANT)
    Q_PROPERTY(QString      version         READ version                                        NOTIFY versionChanged)
    Q_PROPERTY(QString      wifiIPAddress   READ wifiIPAddress                                  CONSTANT)
    Q_PROPERTY(QString      wifiSSID        READ wifiSSID           WRITE setWifiSSID           NOTIFY wifiSSIDChanged)
    Q_PROPERTY(QString      wifiPassword    READ wifiPassword       WRITE setWifiPassword       NOTIFY wifiPasswordChanged)
    Q_PROPERTY(QString      wifiSSIDSta     READ wifiSSIDSta        WRITE setWifiSSIDSta        NOTIFY wifiSSIDStaChanged)
    Q_PROPERTY(QString      wifiPasswordSta READ wifiPasswordSta    WRITE setWifiPasswordSta    NOTIFY wifiPasswordStaChanged)
    Q_PROPERTY(QStringList  wifiChannels    READ wifiChannels                                   CONSTANT)
    Q_PROPERTY(QStringList  baudRates       READ baudRates                                      CONSTANT)
    Q_PROPERTY(int          baudIndex       READ baudIndex          WRITE setBaudIndex          NOTIFY baudIndexChanged)
    Q_PROPERTY(bool         busy            READ busy                                           NOTIFY busyChanged)
    Q_PROPERTY(Vehicle      *vehicle        READ vehicle                                        CONSTANT)

public:
    explicit ESP8266ComponentController(QObject *parent = nullptr);
    ~ESP8266ComponentController();

    Q_INVOKABLE void restoreDefaults();
    Q_INVOKABLE void reboot();

    static int componentID() { return MAV_COMP_ID_UDP_BRIDGE; }
    QString version() const;
    QString wifiIPAddress();
    QString wifiSSID() const;
    QString wifiPassword() const;
    QString wifiSSIDSta() const;
    QString wifiPasswordSta() const;
    QStringList wifiChannels() const { return _channels; }
    QStringList baudRates() const { return _baudRates; }
    int baudIndex() const;
    bool busy() const { return (_waitType != WAIT_FOR_NOTHING); }
    Vehicle *vehicle() const { return _vehicle; }

    void setWifiSSID(const QString &id) const;
    void setWifiPassword(const QString &pwd) const;
    void setWifiSSIDSta(const QString &id) const;
    void setWifiPasswordSta(const QString &pwd) const;
    void setBaudIndex(int idx) const;

signals:
    void versionChanged();
    void wifiSSIDChanged();
    void wifiPasswordChanged();
    void wifiSSIDStaChanged();
    void wifiPasswordStaChanged();
    void baudIndexChanged();
    void busyChanged();

private slots:
    void _mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle);
    void _ssidChanged(QVariant value) { emit wifiSSIDChanged(); }
    void _passwordChanged(QVariant value) { emit wifiPasswordChanged(); }
    void _baudChanged(QVariant value) { emit baudIndexChanged(); }
    void _versionChanged(QVariant value) { emit versionChanged(); }

private:
    void _reboot() const;
    void _restoreDefaults() const;

    QStringList _channels;
    const QStringList _baudRates = { QStringLiteral("57600"), QStringLiteral("115200"), QStringLiteral("230400"), QStringLiteral("460800"), QStringLiteral("921600") };
    QString _ipAddress;

    enum {
        WAIT_FOR_NOTHING,
        WAIT_FOR_REBOOT,
        WAIT_FOR_RESTORE
    };

    int _waitType = WAIT_FOR_NOTHING;
    int _retries = 0;

    Fact *_baud = nullptr;
    Fact *_ver = nullptr;

    Fact *_ssid1 = nullptr;
    Fact *_ssid2 = nullptr;
    Fact *_ssid3 = nullptr;
    Fact *_ssid4 = nullptr;

    Fact *_pwd1 = nullptr;
    Fact *_pwd2 = nullptr;
    Fact *_pwd3 = nullptr;
    Fact *_pwd4 = nullptr;

    Fact *_ssidsta1 = nullptr;
    Fact *_ssidsta2 = nullptr;
    Fact *_ssidsta3 = nullptr;
    Fact *_ssidsta4 = nullptr;

    Fact *_pwdsta1 = nullptr;
    Fact *_pwdsta2 = nullptr;
    Fact *_pwdsta3 = nullptr;
    Fact *_pwdsta4 = nullptr;
};
