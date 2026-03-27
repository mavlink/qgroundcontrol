#pragma once

#include "GPSRTKFactGroup.h"
#include "SatelliteModel.h"
#include "satellite_info.h"
#include "sensor_gps.h"

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtQmlIntegration/QtQmlIntegration>

class MockGPSRtk : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(QString devicePath READ devicePath CONSTANT)
    Q_PROPERTY(QString deviceType READ deviceType NOTIFY deviceTypeChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(FactGroup* factGroup READ gpsRtkFactGroup CONSTANT)
    Q_PROPERTY(SatelliteModel* satelliteModel READ satelliteModel CONSTANT)

public:
    explicit MockGPSRtk(QObject *parent = nullptr)
        : QObject(parent)
        , _factGroup(new GPSRTKFactGroup(this))
        , _satelliteModel(new SatelliteModel(this))
    {}

    // --- Same interface as GPSRtk ---

    QString devicePath() const { return _devicePath; }
    QString deviceType() const { return _deviceType; }
    bool connected() const { return _connected; }
    FactGroup *gpsRtkFactGroup() { return _factGroup; }
    SatelliteModel *satelliteModel() { return _satelliteModel; }

    void injectRTCMData(const QByteArray &data)
    {
        if (!_connected) return;
        injectedRtcm.append(data);
    }

    // --- Test control ---

    void simulateConnect(const QString &device, const QString &type)
    {
        _devicePath = device;
        _deviceType = type;
        _connected = true;
        emit deviceTypeChanged();
        emit connectedChanged();
    }

    void simulateDisconnect()
    {
        _connected = false;
        _factGroup->connected()->setRawValue(false);
        emit connectedChanged();
    }

    void simulateSurveyInStatus(float duration, float accuracyMM, bool valid, bool active)
    {
        _factGroup->currentDuration()->setRawValue(duration);
        _factGroup->currentAccuracy()->setRawValue(static_cast<double>(accuracyMM) / 1000.0);
        _factGroup->valid()->setRawValue(valid);
        _factGroup->active()->setRawValue(active);
    }

    void simulatePosition(double lat, double lon, double alt, int fixType)
    {
        _factGroup->baseLatitude()->setRawValue(lat);
        _factGroup->baseLongitude()->setRawValue(lon);
        _factGroup->baseAltitude()->setRawValue(alt);
        _factGroup->baseFixType()->setRawValue(fixType);
        _factGroup->connected()->setRawValue(true);
    }

    void simulateSatellites(int count)
    {
        _factGroup->numSatellites()->setRawValue(count);
    }

    // --- Test inspection ---

    QVector<QByteArray> injectedRtcm;

signals:
    void connectedChanged();
    void deviceTypeChanged();

private:
    GPSRTKFactGroup *_factGroup = nullptr;
    SatelliteModel *_satelliteModel = nullptr;
    QString _devicePath;
    QString _deviceType;
    bool _connected = false;
};
