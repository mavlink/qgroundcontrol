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

    Q_PROPERTY(double       height              MEMBER _toolbarHeight           NOTIFY heightChanged)
    Q_PROPERTY(float        progressBarValue    MEMBER _progressBarValue        NOTIFY progressBarValueChanged)
    Q_PROPERTY(int          telemetryRRSSI      READ telemetryRRSSI             NOTIFY telemetryRRSSIChanged)
    Q_PROPERTY(int          telemetryLRSSI      READ telemetryLRSSI             NOTIFY telemetryLRSSIChanged)
    Q_PROPERTY(unsigned int telemetryRXErrors   READ telemetryRXErrors          NOTIFY telemetryRXErrorsChanged)
    Q_PROPERTY(unsigned int telemetryFixed      READ telemetryFixed             NOTIFY telemetryFixedChanged)
    Q_PROPERTY(unsigned int telemetryTXBuffer   READ telemetryTXBuffer          NOTIFY telemetryTXBufferChanged)
    Q_PROPERTY(unsigned int telemetryLNoise     READ telemetryLNoise            NOTIFY telemetryLNoiseChanged)
    Q_PROPERTY(unsigned int telemetryRNoise     READ telemetryRNoise            NOTIFY telemetryRNoiseChanged)

    void        viewStateChanged        (const QString& key, bool value);

    int         telemetryRRSSI          () { return _telemetryRRSSI; }
    int         telemetryLRSSI          () { return _telemetryLRSSI; }
    unsigned int telemetryRXErrors      () { return _telemetryRXErrors; }
    unsigned int telemetryFixed         () { return _telemetryFixed; }
    unsigned int telemetryTXBuffer      () { return _telemetryTXBuffer; }
    unsigned int telemetryLNoise        () { return _telemetryLNoise; }
    unsigned int telemetryRNoise        () { return _telemetryRNoise; }

signals:
    void progressBarValueChanged        (float value);
    void telemetryRRSSIChanged          (int value);
    void telemetryLRSSIChanged          (int value);
    void heightChanged                  (double height);
    void telemetryRXErrorsChanged       (unsigned int value);
    void telemetryFixedChanged          (unsigned int value);
    void telemetryTXBufferChanged       (unsigned int value);
    void telemetryLNoiseChanged         (unsigned int value);
    void telemetryRNoiseChanged         (unsigned int value);

private slots:
    void _activeVehicleChanged          (Vehicle* vehicle);
    void _setProgressBarValue           (float value);
    void _telemetryChanged              (LinkInterface* link, unsigned rxerrors, unsigned fixed, int rssi, int remrssi, unsigned txbuf, unsigned noise, unsigned remnoise);

private:
    Vehicle*        _vehicle;
    UASInterface*   _mav;
    float           _progressBarValue;
    double          _remoteRSSIstore;
    int             _telemetryRRSSI;
    int             _telemetryLRSSI;
    uint32_t        _telemetryRXErrors;
    uint32_t        _telemetryFixed;
    uint32_t        _telemetryTXBuffer;
    uint32_t        _telemetryLNoise;
    uint32_t        _telemetryRNoise;

    double          _toolbarHeight;

    QStringList     _toolbarMessageQueue;
    QMutex          _toolbarMessageQueueMutex;
};

#endif // MainToolBarController_H
