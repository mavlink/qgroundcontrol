#pragma once
#include "GPSPositionService.h"

/// QGC platform permissions, custom position source, and application lifetime.
class QGCPositionManager : public GPSPositionService
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    friend class PositionManagerTest;

public:
    explicit QGCPositionManager(QObject* parent = nullptr);
    ~QGCPositionManager() override;
    static QGCPositionManager* instance();
    void init();

private:
    void _setupPositionSources();
    void _setupPositionSources(
        QGeoPositionInfoSource* customSource,
        const std::function<QGeoPositionInfoSource*(const QString&, QObject*)>& createPlatformSource);
    static QString _platformSourceName();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
};
