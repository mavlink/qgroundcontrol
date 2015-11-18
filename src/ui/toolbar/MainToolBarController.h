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

#ifndef MainToolBarController_H
#define MainToolBarController_H

#include <QObject>

#include "Vehicle.h"
#include "UASMessageView.h"

#define TOOL_BAR_SETTINGS_GROUP "TOOLBAR_SETTINGS_GROUP"
#define TOOL_BAR_SHOW_BATTERY   "ShowBattery"
#define TOOL_BAR_SHOW_GPS       "ShowGPS"
#define TOOL_BAR_SHOW_MAV       "ShowMav"
#define TOOL_BAR_SHOW_MESSAGES  "ShowMessages"
#define TOOL_BAR_SHOW_RSSI      "ShowRSSI"

class MainToolBarController : public QObject
{
    Q_OBJECT

public:
    MainToolBarController(QObject* parent = NULL);
    ~MainToolBarController();

    Q_INVOKABLE void    onSetupView();
    Q_INVOKABLE void    onPlanView();
    Q_INVOKABLE void    onFlyView();
    Q_INVOKABLE void    onEnterMessageArea(int x, int y);
    Q_INVOKABLE void    onToolBarMessageClosed(void);
    Q_INVOKABLE void    showSettings(void);
    Q_INVOKABLE void    manageLinks(void);

    Q_PROPERTY(double       height              MEMBER _toolbarHeight           NOTIFY heightChanged)
    Q_PROPERTY(float        progressBarValue    MEMBER _progressBarValue        NOTIFY progressBarValueChanged)
    Q_PROPERTY(int          telemetryRRSSI      READ telemetryRRSSI             NOTIFY telemetryRRSSIChanged)
    Q_PROPERTY(int          telemetryLRSSI      READ telemetryLRSSI             NOTIFY telemetryLRSSIChanged)

    void        viewStateChanged        (const QString& key, bool value);
    int         telemetryRRSSI          () { return _telemetryRRSSI; }
    int         telemetryLRSSI          () { return _telemetryLRSSI; }
    
    void showToolBarMessage(const QString& message);
    
signals:
    void progressBarValueChanged        (float value);
    void telemetryRRSSIChanged          (int value);
    void telemetryLRSSIChanged          (int value);
    void heightChanged                  (double height);
    
    /// Shows a non-modal message below the toolbar
    void showMessage(const QString& message);

private slots:
    void _activeVehicleChanged          (Vehicle* vehicle);
    void _leaveMessageView              ();
    void _setProgressBarValue           (float value);
    void _telemetryChanged              (LinkInterface* link, unsigned rxerrors, unsigned fixed, unsigned rssi, unsigned remrssi, unsigned txbuf, unsigned noise, unsigned remnoise);
    void _delayedShowToolBarMessage     (void);

private:
    Vehicle*        _vehicle;
    UASInterface*   _mav;
    float           _progressBarValue;
    double          _remoteRSSIstore;
    int             _telemetryRRSSI;
    int             _telemetryLRSSI;
    double          _toolbarHeight;

    UASMessageViewRollDown* _rollDownMessages;
    
    bool            _toolbarMessageVisible;
    QStringList     _toolbarMessageQueue;
    QMutex          _toolbarMessageQueueMutex;
};

#endif // MainToolBarController_H
