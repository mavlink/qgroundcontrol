#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSPositionService.h"

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

private:
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    QMetaObject::Connection _sourceSettingConnection;
    bool _destroying = false;
};
