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
    explicit GPSPositionSourceAdapter(QObject* parent = nullptr);
    ~GPSPositionSourceAdapter() override;

    void configure(QGeoPositionInfoSource* source, GPSSourceHealth* health, const QString& identity, bool platform);
    void setActive(bool active);
    void updatePosition(const QGeoPositionInfo& position);

    QGeoPositionInfoSource* source() const { return _source; }

    GPSSourceHealth* health()
    {
        return _source ? (_providedHealth ? _providedHealth.data() : &_fallbackHealth) : nullptr;
    }

    GPSSourceHealth& fallbackHealth() { return _fallbackHealth; }

    int updateInterval() const;

signals:
    void bindingChanged();
    void observationChanged();
    void backendError(QGeoPositionInfoSource::Error error);

private:
    void _disconnectSource();
    QPointer<QGeoPositionInfoSource> _source;
    QPointer<GPSSourceHealth> _providedHealth;
    GPSSourceHealth _fallbackHealth;
    QList<QMetaObject::Connection> _connections;
    QString _identity;
    bool _platform = false;
    bool _active = false;
    quint64 _generation = 0;
};
