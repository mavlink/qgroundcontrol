#pragma once

#include <QtCore/QObject>

#include "GPSBaseReference.h"
#include "GPSIntegrityStore.h"
#include "GPSReceiverSession.h"
#include "GPSRelativePositionStore.h"
#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"

/// Accepted receiver observations survive replacement of their presentation objects.
class GPSReceiverState : public QObject
{
    Q_OBJECT

public:
    explicit GPSReceiverState(GPSReceiverSession& session, QObject* parent = nullptr,
                              RuntimeScheduler* scheduler = nullptr);
    ~GPSReceiverState() override;

    GPSReceiverSession& session() const { return _session; }

    GPSSourceHealth* health() { return &_health; }

    GPSSatelliteStore* satellites() { return &_satellites; }

    GPSRelativePositionStore* relativePosition() { return &_relativePosition; }

    GPSIntegrityStore* integrity() { return &_integrity; }

    GPSBaseReference reference() const { return _reference; }

    GPSSurveyInStatus surveyStatus() const { return _survey; }

    void updatePosition(const GPSObservation& observation);
    void reset();

signals:
    void referenceChanged();

private:
    void _attemptChanged(const GPSReceiverAttempt& attempt);
    void _updateSurvey(const GPSSurveyInStatus& status);
    void _resetReference();
    bool _acceptsSurvey() const;

    GPSReceiverSession& _session;
    GPSSourceHealth _health;
    GPSSatelliteStore _satellites;
    GPSRelativePositionStore _relativePosition;
    GPSIntegrityStore _integrity;
    GPSBaseReference _reference;
    GPSSurveyInStatus _survey;
    quint64 _revision = 0;
};
