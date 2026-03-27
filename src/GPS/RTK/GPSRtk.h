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
class GPSSatelliteInfoSource;
class GPSTransport;
class FactGroup;
class QThread;
class RTCMMavlink;
class SatelliteModel;

class GPSRtk : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("GPSRTKFactGroup.h")
    Q_MOC_INCLUDE("SatelliteModel.h")
    Q_PROPERTY(QString devicePath READ devicePath CONSTANT)
    Q_PROPERTY(QString deviceType READ deviceType NOTIFY deviceTypeChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(FactGroup* factGroup READ gpsRtkFactGroup CONSTANT)
    Q_PROPERTY(SatelliteModel* satelliteModel READ satelliteModel CONSTANT)

public:
    explicit GPSRtk(QObject *parent = nullptr);
    ~GPSRtk() override;

    void connectGPS(const QString &device, QStringView gps_type);
    void connectGPS(GPSTransport *transport, QStringView gps_type);
    /// Asynchronously tears down the worker thread and GPSProvider. Returns immediately —
    /// the thread is cleaned up on QThread::finished. Safe to call from the GUI thread.
    void disconnectGPS();
    bool connected() const;
    QString devicePath() const { return _devicePath; }
    QString deviceType() const { return _deviceType; }
    QString lastError() const { return _lastError; }
    FactGroup *gpsRtkFactGroup();
    SatelliteModel *satelliteModel() { return _satelliteModel; }
    GPSPositionSource *positionSource() { return _positionSource; }
    GPSSatelliteInfoSource *satelliteInfoSource() { return _satelliteInfoSource; }
    void injectRTCMData(const QByteArray &data);

    void setRtcmMavlink(RTCMMavlink *mavlink) { _rtcmMavlink = mavlink; }

signals:
    void connectedChanged();
    void deviceTypeChanged();
    void lastErrorChanged();
    void errorOccurred(const QString &message);

private:
    /// Tear down the worker thread. If waitForFinish, blocks up to
    /// kGPSThreadShutdownTimeoutMs for the thread to exit — only used in
    /// destruction and connect→reconnect paths to guarantee a clean slate.
    void _teardownThread(bool waitForFinish);
    void _resetState();
    void _connectProviderSignals();

    QPointer<GPSProvider> _gpsProvider;
    QPointer<QThread> _gpsThread;
    GPSRTKFactGroup *_gpsRtkFactGroup = nullptr;
    SatelliteModel *_satelliteModel = nullptr;
    GPSPositionSource *_positionSource = nullptr;
    GPSSatelliteInfoSource *_satelliteInfoSource = nullptr;
    RTCMMavlink *_rtcmMavlink = nullptr;

    QString _devicePath;
    QString _deviceType;
    QString _lastError;

    static constexpr uint32_t kGPSThreadShutdownTimeoutMs = 5000;
};
