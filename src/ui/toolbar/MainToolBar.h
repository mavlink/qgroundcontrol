/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief QGC Main Tool Bar
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef MAINTOOLBAR_H
#define MAINTOOLBAR_H

#include "QGCQmlWidgetHolder.h"

#define TOOL_BAR_SETTINGS_GROUP "TOOLBAR_SETTINGS_GROUP"
#define TOOL_BAR_SHOW_BATTERY   "ShowBattery"
#define TOOL_BAR_SHOW_GPS       "ShowGPS"
#define TOOL_BAR_SHOW_MAV       "ShowMav"
#define TOOL_BAR_SHOW_MESSAGES  "ShowMessages"
#define TOOL_BAR_SHOW_RSSI       "ShowRSSI"

class UASInterface;
class UASMessage;
class UASMessageViewRollDown;

class MainToolBar : public QGCQmlWidgetHolder
{
    Q_OBJECT
    Q_ENUMS(ViewType_t)
    Q_ENUMS(MessageType_t)
public:
    MainToolBar(QWidget* parent = NULL);
    ~MainToolBar();

    typedef enum {
        ViewNone    = -1,
        ViewAnalyze, // MainWindow::VIEW_ENGINEER
        ViewPlan   , // MainWindow::VIEW_MISSION
        ViewFly    , // MainWindow::VIEW_FLIGHT
        ViewSetup  , // MainWindow::VIEW_SETUP
    } ViewType_t;

    typedef enum {
        MessageNone,
        MessageNormal,
        MessageWarning,
        MessageError
    } MessageType_t;

    Q_INVOKABLE void    onSetupView();
    Q_INVOKABLE void    onPlanView();
    Q_INVOKABLE void    onFlyView();
    Q_INVOKABLE void    onAnalyzeView();
    Q_INVOKABLE void    onConnect(QString conf);
    Q_INVOKABLE void    onDisconnect(QString conf);
    Q_INVOKABLE void    onEnterMessageArea(int x, int y);
    Q_INVOKABLE QString getMavIconColor();

    Q_PROPERTY(int           connectionCount    MEMBER _connectionCount         NOTIFY connectionCountChanged)
    Q_PROPERTY(double        batteryVoltage     MEMBER _batteryVoltage          NOTIFY batteryVoltageChanged)
    Q_PROPERTY(double        batteryPercent     MEMBER _batteryPercent          NOTIFY batteryPercentChanged)
    Q_PROPERTY(ViewType_t    currentView        MEMBER _currentView             NOTIFY currentViewChanged)
    Q_PROPERTY(QStringList   configList         MEMBER _linkConfigurations      NOTIFY configListChanged)
    Q_PROPERTY(bool          systemArmed        MEMBER _systemArmed             NOTIFY systemArmedChanged)
    Q_PROPERTY(unsigned int  heartbeatTimeout   MEMBER _currentHeartbeatTimeout NOTIFY heartbeatTimeoutChanged)
    Q_PROPERTY(QString       currentMode        MEMBER _currentMode             NOTIFY currentModeChanged)
    Q_PROPERTY(MessageType_t messageType        MEMBER _currentMessageType      NOTIFY messageTypeChanged)
    Q_PROPERTY(int           newMessageCount    MEMBER _currentMessageCount     NOTIFY newMessageCountChanged)
    Q_PROPERTY(int           messageCount       MEMBER _messageCount            NOTIFY messageCountChanged)
    Q_PROPERTY(QString       systemPixmap       MEMBER _systemPixmap            NOTIFY systemPixmapChanged)
    Q_PROPERTY(int           satelliteCount     MEMBER _satelliteCount          NOTIFY satelliteCountChanged)
    Q_PROPERTY(QStringList   connectedList      MEMBER _connectedList           NOTIFY connectedListChanged)
    Q_PROPERTY(bool          mavPresent         READ mavPresent                 NOTIFY mavPresentChanged)
    Q_PROPERTY(QString       currentState       MEMBER _currentState            NOTIFY currentStateChanged)
    Q_PROPERTY(int           satelliteLock      MEMBER _satelliteLock           NOTIFY satelliteLockChanged)
    Q_PROPERTY(bool          showGPS            MEMBER _showGPS                 NOTIFY showGPSChanged)
    Q_PROPERTY(bool          showMav            MEMBER _showMav                 NOTIFY showMavChanged)
    Q_PROPERTY(bool          showMessages       MEMBER _showMessages            NOTIFY showMessagesChanged)
    Q_PROPERTY(bool          showBattery        MEMBER _showBattery             NOTIFY showBatteryChanged)
    Q_PROPERTY(bool          showRSSI           MEMBER _showRSSI                NOTIFY showRSSIChanged)
    Q_PROPERTY(float         progressBarValue   MEMBER _progressBarValue        NOTIFY progressBarValueChanged)
    Q_PROPERTY(int           remoteRSSI         READ remoteRSSI                 NOTIFY remoteRSSIChanged)
    Q_PROPERTY(int           telemetryRRSSI     READ telemetryRRSSI             NOTIFY telemetryRRSSIChanged)
    Q_PROPERTY(int           telemetryLRSSI     READ telemetryLRSSI             NOTIFY telemetryLRSSIChanged)

    bool        mavPresent              () { return _mav != NULL; }
    int         remoteRSSI              () { return _remoteRSSI; }
    int         telemetryRRSSI          () { return _telemetryRRSSI; }
    int         telemetryLRSSI          () { return _telemetryLRSSI; }

    void        setCurrentView          (int currentView);
    void        viewStateChanged        (const QString& key, bool value);

signals:
    void connectionCountChanged         (int count);
    void batteryVoltageChanged          (double value);
    void batteryPercentChanged          (double value);
    void currentViewChanged             ();
    void configListChanged              ();
    void systemArmedChanged             (bool systemArmed);
    void heartbeatTimeoutChanged        (unsigned int hbTimeout);
    void currentModeChanged             ();
    void messageTypeChanged             (MessageType_t type);
    void newMessageCountChanged         (int count);
    void messageCountChanged            (int count);
    void currentConfigChanged           (QString config);
    void systemPixmapChanged            (QPixmap pix);
    void satelliteCountChanged          (int count);
    void connectedListChanged           (QStringList connectedList);
    void mavPresentChanged              (bool present);
    void currentStateChanged            (QString state);
    void satelliteLockChanged           (int lock);
    void showGPSChanged                 (bool value);
    void showMavChanged                 (bool value);
    void showMessagesChanged            (bool value);
    void showBatteryChanged             (bool value);
    void showRSSIChanged                (bool value);
    void progressBarValueChanged        (float value);
    void remoteRSSIChanged              (int value);
    void telemetryRRSSIChanged          (int value);
    void telemetryLRSSIChanged          (int value);

private slots:
    void _setActiveUAS                  (UASInterface* active);
    void _updateBatteryRemaining        (UASInterface*, double voltage, double, double percent, int);
    void _updateArmingState             (bool armed);
    void _updateConfigurations          ();
    void _linkConnected                 (LinkInterface* link);
    void _linkDisconnected              (LinkInterface* link);
    void _updateState                   (UASInterface* system, QString name, QString description);
    void _updateMode                    (int system, QString name, QString description);
    void _updateName                    (const QString& name);
    void _setSystemType                 (UASInterface* uas, unsigned int systemType);
    void _heartbeatTimeout              (bool timeout, unsigned int ms);
    void _handleTextMessage             (int newCount);
    void _updateCurrentWaypoint         (quint16 id);
    void _updateWaypointDistance        (double distance);
    void _setSatelliteCount             (double val, QString name);
    void _leaveMessageView              ();
    void _setSatLoc                     (UASInterface* uas, int fix);
    void _setProgressBarValue           (float value);
    void _remoteControlRSSIChanged      (uint8_t rssi);
    void _telemetryChanged              (LinkInterface* link, unsigned rxerrors, unsigned fixed, unsigned rssi, unsigned remrssi, unsigned txbuf, unsigned noise, unsigned remnoise);

private:
    void _updateConnection              (LinkInterface *disconnectedLink = NULL);
    void _setToolBarState               (const QString& key, bool value);

private:
    UASInterface*   _mav;
    QQuickItem*     _toolBar;
    ViewType_t      _currentView;
    double          _batteryVoltage;
    double          _batteryPercent;
    QStringList     _linkConfigurations;
    int             _connectionCount;
    bool            _systemArmed;
    QString         _currentState;
    QString         _currentMode;
    QString         _systemName;
    QString         _systemPixmap;
    unsigned int    _currentHeartbeatTimeout;
    double          _waypointDistance;
    quint16         _currentWaypoint;
    int             _currentMessageCount;
    int             _messageCount;
    int             _currentErrorCount;
    int             _currentWarningCount;
    int             _currentNormalCount;
    MessageType_t   _currentMessageType;
    int             _satelliteCount;
    QStringList     _connectedList;
    int             _satelliteLock;
    bool            _showGPS;
    bool            _showMav;
    bool            _showMessages;
    bool            _showRSSI;
    bool            _showBattery;
    float           _progressBarValue;
    int             _remoteRSSI;
    int             _telemetryRRSSI;
    int             _telemetryLRSSI;

    UASMessageViewRollDown* _rollDownMessages;
};

#endif // MAINTOOLBAR_H
