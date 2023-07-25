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

#ifndef ESP8266ComponentController_H
#define ESP8266ComponentController_H

#include <QTimer>

#include "FactPanelController.h"
#include "UASInterface.h"
#include "QGCLoggingCategory.h"
#include "AutoPilotPlugin.h"

Q_DECLARE_LOGGING_CATEGORY(ESP8266ComponentControllerLog)

namespace Ui {
    class ESP8266ComponentController;
}

class ESP8266ComponentController : public FactPanelController
{
    Q_OBJECT

public:
    ESP8266ComponentController      ();
    ~ESP8266ComponentController     ();

    Q_PROPERTY(int              componentID     READ componentID                                    CONSTANT)
    Q_PROPERTY(QString          version         READ version                                        NOTIFY versionChanged)
    Q_PROPERTY(QString          wifiIPAddress   READ wifiIPAddress                                  CONSTANT)
    Q_PROPERTY(QString          wifiSSID        READ wifiSSID           WRITE setWifiSSID           NOTIFY wifiSSIDChanged)
    Q_PROPERTY(QString          wifiPassword    READ wifiPassword       WRITE setWifiPassword       NOTIFY wifiPasswordChanged)
    Q_PROPERTY(QString          wifiSSIDSta     READ wifiSSIDSta        WRITE setWifiSSIDSta        NOTIFY wifiSSIDStaChanged)
    Q_PROPERTY(QString          wifiPasswordSta READ wifiPasswordSta    WRITE setWifiPasswordSta    NOTIFY wifiPasswordStaChanged)
    Q_PROPERTY(QStringList      wifiChannels    READ wifiChannels                                   CONSTANT)
    Q_PROPERTY(QStringList      baudRates       READ baudRates                                      CONSTANT)
    Q_PROPERTY(int              baudIndex       READ baudIndex          WRITE setBaudIndex          NOTIFY baudIndexChanged)
    Q_PROPERTY(bool             busy            READ busy                                           NOTIFY busyChanged)
    Q_PROPERTY(Vehicle*         vehicle         READ vehicle                                        CONSTANT)

    Q_INVOKABLE void restoreDefaults();
    Q_INVOKABLE void reboot         ();

    int             componentID     () { return MAV_COMP_ID_UDP_BRIDGE; }
    QString         version         ();
    QString         wifiIPAddress   ();
    QString         wifiSSID        ();
    QString         wifiPassword    ();
    QString         wifiSSIDSta     ();
    QString         wifiPasswordSta ();
    QStringList     wifiChannels    () { return _channels; }
    QStringList     baudRates       () { return _baudRates; }
    int             baudIndex       ();
    bool            busy            () const{ return _waitType != WAIT_FOR_NOTHING; }
    Vehicle*        vehicle         () { return _vehicle; }

    void        setWifiSSID         (QString id);
    void        setWifiPassword     (QString pwd);
    void        setWifiSSIDSta      (QString id);
    void        setWifiPasswordSta  (QString pwd);
    void        setBaudIndex        (int idx);

signals:
    void        versionChanged          ();
    void        wifiSSIDChanged         ();
    void        wifiPasswordChanged     ();
    void        wifiSSIDStaChanged      ();
    void        wifiPasswordStaChanged  ();
    void        baudIndexChanged        ();
    void        busyChanged             ();

private slots:
    void        _mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle);
    void        _ssidChanged        (QVariant value);
    void        _passwordChanged    (QVariant value);
    void        _baudChanged        (QVariant value);
    void        _versionChanged     (QVariant value);

private:
    void        _reboot             ();
    void        _restoreDefaults    ();

private:
    QStringList _channels;
    QStringList _baudRates;
    QString     _ipAddress;

    enum {
        WAIT_FOR_NOTHING,
        WAIT_FOR_REBOOT,
        WAIT_FOR_RESTORE
    };

    int         _waitType;
    int         _retries;
};

#endif // ESP8266ComponentController_H
