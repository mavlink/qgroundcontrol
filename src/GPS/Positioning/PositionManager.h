#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSPositionService.h"

class NMEASourceManager;

/// QGC owns permissions and source creation; the service owns positioning policy.
class QGCPositionManager : public GPSPositionService
{
    Q_OBJECT
    Q_PROPERTY(NMEASourceManager* nmeaInput READ nmeaInput NOTIFY nmeaInputChanged)
    Q_MOC_INCLUDE("NMEASourceManager.h")
    QML_ELEMENT
    QML_UNCREATABLE("Created by QGroundControl")

public:
    explicit QGCPositionManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~QGCPositionManager() override;

    static QGCPositionManager* instance();
    void init();

    NMEASourceManager* nmeaInput() const;
    void setNmeaInput(NMEASourceManager* input);

signals:
    void nmeaInputChanged();

private:
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    QPointer<NMEASourceManager> _nmeaInput;
    QMetaObject::Connection _nmeaInputDestroyedConnection;
    QMetaObject::Connection _sourceSettingConnection;
    bool _destroying = false;
};
