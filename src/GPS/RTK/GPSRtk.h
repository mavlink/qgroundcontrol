#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSProvider.h"

Q_DECLARE_LOGGING_CATEGORY(GPSRtkLog)

class GPSPositionSource;
class GPSRTKFactGroup;
class GPSTransport;
class FactGroup;
class QThread;
class RTCMRouter;
class RTKSatelliteModel;

class GPSRtk : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("GPSRTKFactGroup.h")
    Q_MOC_INCLUDE("RTKSatelliteModel.h")
    Q_PROPERTY(QString devicePath READ devicePath CONSTANT)
    Q_PROPERTY(QString deviceType READ deviceType NOTIFY deviceTypeChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(FactGroup* factGroup READ gpsRtkFactGroup CONSTANT)
    Q_PROPERTY(RTKSatelliteModel* satelliteModel READ satelliteModel CONSTANT)

public:
    explicit GPSRtk(QObject *parent = nullptr);
    ~GPSRtk() override;

    void connectGPS(const QString &device, QStringView gps_type);
    void connectGPS(GPSTransport *transport, QStringView gps_type);
    void disconnectGPS();
    bool connected() const;
    QString devicePath() const { return _devicePath; }
    QString deviceType() const { return _deviceType; }
    FactGroup *gpsRtkFactGroup();
    RTKSatelliteModel *satelliteModel() { return _satelliteModel; }
    GPSPositionSource *positionSource() { return _positionSource; }
    void injectRTCMData(const QByteArray &data);

    void setRtcmRouter(RTCMRouter *router) { _rtcmRouter = router; }

signals:
    void connectedChanged();
    void deviceTypeChanged();
    void errorOccurred(const QString &message);

private:
    void _resetState();

    QPointer<GPSProvider> _gpsProvider;
    QPointer<QThread> _gpsThread;
    GPSRTKFactGroup *_gpsRtkFactGroup = nullptr;
    RTKSatelliteModel *_satelliteModel = nullptr;
    GPSPositionSource *_positionSource = nullptr;
    RTCMRouter *_rtcmRouter = nullptr;

    QString _devicePath;
    QString _deviceType;

    std::atomic_bool _requestGpsStop = false;

    static constexpr uint32_t kGPSThreadGracefulTimeout = 5000;
    static constexpr uint32_t kGPSThreadTerminateTimeout = 1000;
};
