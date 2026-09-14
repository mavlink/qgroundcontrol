#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoPositionInfoSource>

#include "GPSSourceHealth.h"

/// Normalizes raw Qt positioning once; supplied health remains owned by its producer.
class GPSPositionSourceAdapter : public QObject
{
    Q_OBJECT

public:
    explicit GPSPositionSourceAdapter(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSPositionSourceAdapter() override;

    void configure(QObject* producer, GPSSourceHealth* health, const QString& identity, bool platform,
                   quint64 sessionId = 0);
    void setActive(bool active);

    QObject* source() const { return _producer; }

    GPSSourceHealth* health()
    {
        return _producer ? (_providedHealth ? _providedHealth.data() : &_fallbackHealth) : nullptr;
    }

    int updateInterval() const;

signals:
    void bindingChanged();
    void observationChanged();
    void backendError(QGeoPositionInfoSource::Error error);

private:
    void _disconnectSource();
    void _updatePosition(const QGeoPositionInfo& position);
    QPointer<QObject> _producer;
    QPointer<QGeoPositionInfoSource> _source;
    QPointer<GPSSourceHealth> _providedHealth;
    QPointer<RuntimeScheduler> _scheduler;
    GPSSourceHealth _fallbackHealth;
    QList<QMetaObject::Connection> _connections;
    QString _identity;
    quint64 _sessionId = 0;
    bool _platform = false;
    bool _active = false;
    bool _updatesStarted = false;
    quint64 _generation = 0;
    quint64 _backendRevision = 0;
};
