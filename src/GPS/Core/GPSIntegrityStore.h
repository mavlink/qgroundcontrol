#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "GPSIntegrityObservation.h"
#include "GPSScheduledTask.h"

/// Accepted receiver diagnostics with separate freshness for each reported provenance group.
class GPSIntegrityStore : public QObject
{
    Q_OBJECT

public:
    explicit GPSIntegrityStore(QObject* parent = nullptr, GPSRuntimeScheduler* scheduler = nullptr);
    ~GPSIntegrityStore() override;

    void beginSession(quint64 sessionId);
    void updateObservation(const GPSIntegrityObservation& observation);
    void reset();

    GPSIntegrityObservation observation() const { return _observation; }

    bool available() const { return _available; }

    bool systemErrorsKnown() const { return _observation.systemErrors.has_value(); }

signals:
    void observationChanged(const GPSIntegrityObservation& observation);

private:
    void _refresh();

    QPointer<GPSRuntimeScheduler> _scheduler;
    GPSScheduledTask _expiryTask;
    GPSIntegrityObservation _observation;
    std::optional<quint64> _sessionId = std::nullopt;
    bool _available = false;

    static constexpr quint64 FRESHNESS_TIMEOUT_US = 5000000;
};
