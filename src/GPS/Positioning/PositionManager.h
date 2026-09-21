#pragma once

#include <memory>

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSPositionService.h"

class QIODevice;
class NMEADecoderSession;
class NMEASourceManager;

/// QGC owns permissions and source creation; the service owns positioning policy.
class QGCPositionManager : public GPSPositionService
{
    Q_OBJECT
    Q_PROPERTY(GPSSourceHealth* nmeaHealth READ nmeaHealth NOTIFY nmeaSourceChanged)
    Q_PROPERTY(bool nmeaReceiving READ nmeaReceiving NOTIFY nmeaActivityChanged)
    Q_PROPERTY(bool nmeaHasData READ nmeaHasData NOTIFY nmeaActivityChanged)
    Q_PROPERTY(NMEASourceManager* nmeaInput READ nmeaInput NOTIFY nmeaInputChanged)
    Q_MOC_INCLUDE("NMEASourceManager.h")
    QML_ELEMENT
    QML_UNCREATABLE("Created by QGroundControl")

public:
    explicit QGCPositionManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~QGCPositionManager() override;

    static QGCPositionManager* instance();
    void init();

    void setNmeaSourceDevice(QIODevice* device);
    void resetNmeaSourceDevice();
    /// A retiring input owner must not clear a source installed by a replacement callback.
    void resetNmeaSourceDevice(QIODevice* expectedDevice);
    QIODevice* nmeaSourceDevice() const;

    GPSSourceHealth* nmeaHealth() const;
    bool nmeaReceiving() const;
    bool nmeaHasData() const;
    NMEASourceManager* nmeaInput() const;
    void setNmeaInput(NMEASourceManager* input);

signals:
    void nmeaSourceChanged();
    void nmeaActivityChanged();
    void nmeaInputChanged();

private:
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    void _resetNmeaSourceDevice(const char* reason);

    std::unique_ptr<NMEADecoderSession> _nmeaSource;
    QPointer<QIODevice> _nmeaDevice;
    GPSPositionSourceRegistration _nmeaRegistration;
    QMetaObject::Connection _nmeaDeviceDestroyedConnection;
    QMetaObject::Connection _nmeaDeviceClosedConnection;
    QPointer<NMEASourceManager> _nmeaInput;
    QMetaObject::Connection _nmeaInputDestroyedConnection;
    quint64 _nmeaRevision = 0;
    bool _destroying = false;
};
