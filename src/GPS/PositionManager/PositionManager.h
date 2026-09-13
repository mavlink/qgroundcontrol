#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include <memory>

#include "GPSPositionService.h"

class QIODevice;
class QNmeaPositionInfoSource;

/// QGC owns permissions and source creation; the service owns positioning policy.
class QGCPositionManager : public GPSPositionService
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by QGroundControl")

public:
    explicit QGCPositionManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~QGCPositionManager() override;

    static QGCPositionManager* instance();
    void init();

    void setNmeaSourceDevice(QIODevice* device);
    void resetNmeaSourceDevice();

private:
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();

    QPointer<QGeoPositionInfoSource> _platformSource;
    std::unique_ptr<QNmeaPositionInfoSource> _nmeaSource;
    GPSPositionSourceRegistration _nmeaRegistration;
    QMetaObject::Connection _nmeaDeviceDestroyedConnection;
    quint64 _nmeaRevision = 0;
    bool _destroying = false;
};
