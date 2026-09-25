#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtPositioning/QGeoPositionInfoSource>

#include "GPSRevision.h"
#include "GPSSourceHealth.h"

class RuntimeScheduler;

/// Adapts a Qt positioning backend to the position producer contract: fixes become observations in an owned
/// GPSSourceHealth, and backend updates run only while the adapter is active. The backend is borrowed.
class GPSPositionBackendAdapter : public QObject
{
    Q_OBJECT

public:
    /// Runs before a backend event is applied. It may re-enter, deactivate, retire or delete the adapter; the event
    /// is then dropped.
    using EventHandler = std::function<void(QGeoPositionInfoSource::Error error)>;

    GPSPositionBackendAdapter(QGeoPositionInfoSource* backend, const QString& identity, bool platform,
                              quint64 sessionId, RuntimeScheduler* scheduler, QObject* parent = nullptr);
    ~GPSPositionBackendAdapter() override;

    QGeoPositionInfoSource* backend() const { return _backend.data(); }

    const QString& identity() const { return _identity; }

    GPSSourceHealth* health() { return &_health; }

    void setEventHandler(EventHandler handler) { _eventHandler = std::move(handler); }

    /// Starts or stops backend updates; deactivation discards the adapted observation.
    void setActive(bool active);

    /// Platform backends report their own minimum interval; other backends update as fast as they publish.
    int updateInterval() const;

    /// Stops updates and detaches from the backend; later events are ignored.
    void retire();

private:
    void _positionUpdated(const QGeoPositionInfo& position);
    void _errorOccurred(QGeoPositionInfoSource::Error error);
    bool _notify(QGeoPositionInfoSource::Error error);

    QPointer<QGeoPositionInfoSource> _backend;
    RuntimeScheduler* const _scheduler;
    GPSSourceHealth _health;
    EventHandler _eventHandler;
    QList<QMetaObject::Connection> _connections;
    QString _identity;
    quint64 _sessionId = 0;
    GPSRevision _eventRevision;
    bool _platform = false;
    bool _active = false;
    bool _updatesStarted = false;
};
