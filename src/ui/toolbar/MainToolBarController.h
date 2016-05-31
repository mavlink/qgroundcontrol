/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
