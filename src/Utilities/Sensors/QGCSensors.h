#pragma once

#include <QtPositioning/QGeoPositionInfo>
#include <QtSensors/QAmbientTemperatureSensor>
#include <QtSensors/QCompass>
#include <QtSensors/QPressureSensor>

namespace QGCSensors
{

////////////////////////////////////////////////////////////////////

class QGCAmbientTemperatureFilter : public QAmbientTemperatureFilter
{
public:
    bool filter(QAmbientTemperatureReading *reading) final;
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
    QGCAmbientTemperatureFilter _ambientTemperatureFilter;

    QMetaObject::Connection _readingChangedConnection;

    qreal _temperatureC = 0;
};

////////////////////////////////////////////////////////////////////

class QGCPressureFilter : public QPressureFilter
{
public:
    bool filter(QPressureReading *reading) final;

private:
    static constexpr const qreal s_minValidPressurePa = 45000.;
    static constexpr const qreal s_maxValidPressurePa = 110000.;
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
    QGCPressureFilter _pressureFilter;

    QMetaObject::Connection _readingChangedConnection;

    qreal _temperatureC = 0;
    qreal _pressurePa = 0;
};

////////////////////////////////////////////////////////////////////

class QGCCompassFilter : public QCompassFilter
{
public:
    bool filter(QCompassReading *reading) final;

private:
    static constexpr qreal s_minCompassCalibrationLevel = 0.65;
};

class QGCCompass : public QObject
{
    Q_OBJECT

public:
    QGCCompass(QObject *parent = nullptr);

    static QGCCompass* instance();

    bool init();
    void quit();

signals:
    void compassUpdated(qreal azimuth);
    void positionUpdated(QGeoPositionInfo update);

private:
    QCompass *_compass = nullptr;
    QGCCompassFilter _compassFilter;

    QMetaObject::Connection _readingChangedConnection;
};

////////////////////////////////////////////////////////////////////

} /* namespace QGCSensors */
