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

class UASInterface;
class UASMessage;
class UASMessageViewRollDown;

class MainToolBar : public QGCQmlWidgetHolder
{
    Q_OBJECT
    Q_ENUMS(ViewType_t)
    Q_ENUMS(MessageType_t)
public:
    MainToolBar();
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
    Q_INVOKABLE void    onLinkConfigurationChanged(const QString& config);
    Q_INVOKABLE void    onEnterMessageArea(int x, int y);
    Q_INVOKABLE QString getMavIconColor();

    Q_PROPERTY(int           connectionCount    READ connectionCount    NOTIFY connectionCountChanged)
    Q_PROPERTY(double        batteryVoltage     READ batteryVoltage     NOTIFY batteryVoltageChanged)
    Q_PROPERTY(double        batteryPercent     READ batteryPercent     NOTIFY batteryPercentChanged)
    Q_PROPERTY(ViewType_t    currentView        READ currentView        NOTIFY currentViewChanged)
    Q_PROPERTY(QStringList   configList         READ configList         NOTIFY configListChanged)
    Q_PROPERTY(bool          systemArmed        READ systemArmed        NOTIFY systemArmedChanged)
    Q_PROPERTY(unsigned int  heartbeatTimeout   READ heartbeatTimeout   NOTIFY heartbeatTimeoutChanged)
    Q_PROPERTY(QString       currentMode        READ currentMode        NOTIFY currentModeChanged)
    Q_PROPERTY(MessageType_t messageType        READ messageType        NOTIFY messageTypeChanged)
    Q_PROPERTY(int           messageCount       READ messageCount       NOTIFY messageCountChanged)
    Q_PROPERTY(QString       currentConfig      READ currentConfig      NOTIFY currentConfigChanged)
    Q_PROPERTY(QString       systemPixmap       READ systemPixmap       NOTIFY systemPixmapChanged)
    Q_PROPERTY(int           satelliteCount     READ satelliteCount     NOTIFY satelliteCountChanged)
    Q_PROPERTY(QStringList   connectedList      READ connectedList      NOTIFY connectedListChanged)
    Q_PROPERTY(bool          mavPresent         READ mavPresent         NOTIFY mavPresentChanged)
    Q_PROPERTY(QString       currentState       READ currentState       NOTIFY currentStateChanged)
    Q_PROPERTY(double        dotsPerInch        READ dotsPerInch        NOTIFY dotsPerInchChanged)

    int           connectionCount        () { return _connectionCount; }
    double        batteryVoltage         () { return _batteryVoltage; }
    double        batteryPercent         () { return _batteryPercent; }
    ViewType_t    currentView            () { return _currentView; }
    QStringList   configList             () { return _linkConfigurations; }
    bool          systemArmed            () { return _systemArmed; }
    unsigned int  heartbeatTimeout       () { return _currentHeartbeatTimeout; }
    QString       currentMode            () { return _currentMode; }
    MessageType_t messageType            () { return _currentMessageType; }
    int           messageCount           () { return _currentMessageCount; }
    QString       currentConfig          () { return _currentConfig; }
    QString       systemPixmap           () { return _systemPixmap; }
    int           satelliteCount         () { return _satelliteCount; }
    QStringList   connectedList          () { return _connectedList; }
    bool          mavPresent             () { return _mav != NULL; }
    QString       currentState           () { return _currentState; }
    double        dotsPerInch            () { return _dotsPerInch; }

    void          setCurrentView         (int currentView);
    void          leaveMessageView       ();

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
    void messageCountChanged            (int count);
    void currentConfigChanged           (QString config);
    void systemPixmapChanged            (QPixmap pix);
    void satelliteCountChanged          (int count);
    void connectedListChanged           (QStringList connectedList);
    void mavPresentChanged              (bool present);
    void currentStateChanged            (QString state);
    void dotsPerInchChanged             ();

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
    void _handleTextMessage             (UASMessage* message);
    void _updateCurrentWaypoint         (quint16 id);
    void _updateWaypointDistance        (double distance);
    void _setSatelliteCount             (double val, QString name);

private:
    void _updateConnection              (LinkInterface *disconnectedLink = NULL);

private:
    UASInterface*   _mav;
    QQuickItem*     _toolBar;
    ViewType_t      _currentView;
    double          _batteryVoltage;
    double          _batteryPercent;
    QStringList     _linkConfigurations;
    QString         _currentConfig;
    bool            _linkSelected;
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
    MessageType_t   _currentMessageType;
    int             _satelliteCount;
    QStringList     _connectedList;
    qreal           _dotsPerInch;
    // TODO Convert this to a signal
    UASMessageViewRollDown* _rollDownMessages;
};

#endif // MAINTOOLBAR_H
