/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/


/// @file
///     @brief  ESP8266 WiFi Config Qml Controller
///     @author Gus Grubba <mavlink@grubba.com>

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

    Q_PROPERTY(int              componentID     READ componentID                            CONSTANT)
    Q_PROPERTY(QString          version         READ version                                NOTIFY versionChanged)
    Q_PROPERTY(QString          wifiSSID        READ wifiSSID       WRITE setWifiSSID       NOTIFY wifiSSIDChanged)
    Q_PROPERTY(QString          wifiPassword    READ wifiPassword   WRITE setWifiPassword   NOTIFY wifiPasswordChanged)
    Q_PROPERTY(QStringList      wifiChannels    READ wifiChannels                           CONSTANT)
    Q_PROPERTY(QStringList      baudRates       READ baudRates                              CONSTANT)
    Q_PROPERTY(int              baudIndex       READ baudIndex      WRITE setBaudIndex      NOTIFY baudIndexChanged)
    Q_PROPERTY(bool             busy            READ busy                                   NOTIFY busyChanged)
    Q_PROPERTY(Vehicle*         vehicle         READ vehicle        CONSTANT)

    Q_INVOKABLE void restoreDefaults();
    Q_INVOKABLE void reboot         ();

    int             componentID     () { return MAV_COMP_ID_UDP_BRIDGE; }
    QString         version         ();
    QString         wifiSSID        ();
    QString         wifiPassword    ();
    QStringList     wifiChannels    () { return _channels; }
    QStringList     baudRates       () { return _baudRates; }
    int             baudIndex       ();
    bool            busy            () { return _waitType != WAIT_FOR_NOTHING; }
    Vehicle*        vehicle         () { return _vehicle; }

    void        setWifiSSID         (QString id);
    void        setWifiPassword     (QString pwd);
    void        setBaudIndex        (int idx);

signals:
    void        versionChanged      ();
    void        wifiSSIDChanged     ();
    void        wifiPasswordChanged ();
    void        baudIndexChanged    ();
    void        busyChanged         ();

private slots:
    void        _processTimeout     ();
    void        _commandAck         (uint8_t compID, uint16_t command, uint8_t result);
    void        _ssidChanged        (QVariant value);
    void        _passwordChanged    (QVariant value);
    void        _baudChanged        (QVariant value);
    void        _versionChanged     (QVariant value);

private:
    void        _reboot             ();
    void        _restoreDefaults    ();

private:
    QTimer      _timer;
    QStringList _channels;
    QStringList _baudRates;

    enum {
        WAIT_FOR_NOTHING,
        WAIT_FOR_REBOOT,
        WAIT_FOR_RESTORE
    };

    int         _waitType;
    int         _retries;
};

#endif // ESP8266ComponentController_H
