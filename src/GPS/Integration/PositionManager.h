#pragma once

#include <functional>

#include "GPSPositionService.h"

/// QGC platform permissions, custom position source, and application lifetime.
class QGCPositionManager : public GPSPositionService
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit QGCPositionManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~QGCPositionManager() override;
    static QGCPositionManager* instance();
    void init();
    void configurePositionSources(
        QGeoPositionInfoSource* customSource,
        const std::function<QGeoPositionInfoSource*(const QString&, QObject*)>& createPlatformSource);

private:
    void _setupPositionSources();
    static QString _platformSourceName();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
};
