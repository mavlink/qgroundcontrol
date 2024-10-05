/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtSensors/QAmbientTemperatureSensor>
#include <QtSensors/QPressureSensor>
#include <QtSensors/QCompass>
#include <QtPositioning/QGeoPositionInfo>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCDeviceInfoLog)

namespace QGCDeviceInfo
{

bool isInternetAvailable();
bool isBluetoothAvailable();
bool isNetworkWired();

////////////////////////////////////////////////////////////////////

class QGCAmbientTemperatureFilter : public QAmbientTemperatureFilter
{
public:
    QGCAmbientTemperatureFilter();
    ~QGCAmbientTemperatureFilter();

    bool filter(QAmbientTemperatureReading *reading) final;

private:
    static constexpr const qreal s_minValidTemperatureC = -40.;
    static constexpr const qreal s_maxValidTemperatureC = 85.;
};

class QGCAmbientTemperature : public QObject
{
    Q_OBJECT

public:
    QGCAmbientTemperature(QObject* parent = nullptr);
    ~QGCAmbientTemperature();

    static QGCAmbientTemperature* instance();

    qreal temperature() const { return _temperatureC; }

    bool init();
    void quit();

signals:
    void temperatureUpdated(qreal temperature);

private:
    QAmbientTemperatureSensor* _ambientTemperature = nullptr;
    std::shared_ptr<QGCAmbientTemperatureFilter> _ambientTemperatureFilter = nullptr;

    QMetaObject::Connection _readingChangedConnection;

    qreal _temperatureC = 0;
};

////////////////////////////////////////////////////////////////////

class QGCPressureFilter : public QPressureFilter
{
public:
    QGCPressureFilter();
    ~QGCPressureFilter();

    bool filter(QPressureReading *reading) final;

private:
    static constexpr const qreal s_minValidPressurePa = 45000.;
    static constexpr const qreal s_maxValidPressurePa = 110000.;

    static constexpr const qreal s_minValidTemperatureC = -40.;
    static constexpr const qreal s_maxValidTemperatureC = 85.;
};

class QGCPressure : public QObject
{
    Q_OBJECT

public:
    QGCPressure(QObject* parent = nullptr);
    ~QGCPressure();

    static QGCPressure* instance();

    qreal pressure() const { return _pressurePa; }
    qreal temperature() const { return _temperatureC; }

    bool init();
    void quit();

signals:
    void pressureUpdated(qreal pressure, qreal temperature);

private:
    QPressureSensor* _pressure = nullptr;
    std::shared_ptr<QGCPressureFilter> _pressureFilter = nullptr;

    QMetaObject::Connection _readingChangedConnection;

    qreal _temperatureC = 0;
    qreal _pressurePa = 0;
};

////////////////////////////////////////////////////////////////////

class QGCCompassFilter : public QCompassFilter
{
public:
    QGCCompassFilter();
    ~QGCCompassFilter();

    bool filter(QCompassReading *reading) final;

private:
    static constexpr qreal s_minCompassCalibrationLevel = 0.65;
};

class QGCCompass : public QObject
{
    Q_OBJECT

public:
    QGCCompass(QObject *parent = nullptr);
    ~QGCCompass();

    static QGCCompass* instance();

    bool init();
    void quit();

signals:
    void compassUpdated(qreal azimuth);
    void positionUpdated(QGeoPositionInfo update);

private:
    QCompass *_compass = nullptr;
    std::shared_ptr<QGCCompassFilter> _compassFilter = nullptr;

    QMetaObject::Connection _readingChangedConnection;

    qreal _azimuth = 0;
    qreal _calibrationLevel = 0;
};

////////////////////////////////////////////////////////////////////

} /* namespace QGCDeviceInfo */
