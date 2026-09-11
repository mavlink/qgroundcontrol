#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSPositionService.h"

class GPSCorrectionManager;
class GPSReceiverSession;
class GPSReceiverState;
class NMEASourceManager;
class NTRIPManager;
class SettingsManager;

/// Owns position/correction registrations and retires them with their producing sessions.
class GPSSourceBindings : public QObject
{
    Q_OBJECT

public:
    GPSSourceBindings(SettingsManager& settings, GPSPositionService* positions, GPSReceiverState& receiver,
                      NMEASourceManager& nmea, GPSCorrectionManager& corrections, QObject* parent = nullptr);
    ~GPSSourceBindings() override;
    void init(NTRIPManager* ntrip);
    void stop();
    void shutdown();

private:
    struct PositionBinding
    {
        QPointer<QObject> source;
        quint64 session = 0;
        quint64 revision = 0;
        GPSPositionSourceRegistration registration;
    };

    void _updatePositionBinding(PositionBinding& binding, GPSPositionService::SelectedSource kind, QObject* producer,
                                GPSSourceHealth* health, quint64 session);
    void _updatePositionSource();
    void _updateNmeaPositionSource();
    void _updatePositionSourceMode();
    void _updateCorrectionSettings();
    void _startReceiverCorrections();
    void _stopReceiverCorrections();
    void _startNtripCorrections(quint64 attemptId, const QString& sourceId);
    void _stopNtripCorrections(quint64 attemptId);
    void _updateNtripUdpOutput();
    SettingsManager& _settings;
    QPointer<GPSPositionService> _positionManager;
    GPSReceiverState& _receiver;
    GPSReceiverSession& _receiverSession;
    NMEASourceManager* _nmeaSources;
    GPSCorrectionManager& _corrections;
    QPointer<NTRIPManager> _ntrip;
    PositionBinding _receiverBinding;
    PositionBinding _nmeaBinding;
    GPSCorrectionSourceRegistration _receiverCorrectionSource;
    GPSCorrectionSourceRegistration _ntripCorrectionSource;
    quint64 _correctionDestinationSession = 0;
    quint64 _ntripAttemptId = 0;
    quint64 _receiverCorrectionAttemptId = 0;
    quint64 _receiverCorrectionRevision = 0;
    quint64 _ntripCorrectionRevision = 0;
    quint64 _correctionSettingsRevision = 0;
    QMetaObject::Connection _routingChanges;
    QMetaObject::Connection _deliveryReports;
    bool _initialized = false;
    bool _shutdown = false;
    bool _sinkRetired = false;
};
