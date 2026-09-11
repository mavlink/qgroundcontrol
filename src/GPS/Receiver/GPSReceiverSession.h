#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>

#include "GPSByteStream.h"
#include "GPSIntegrityObservation.h"
#include "GPSProvider.h"
#include "GPSReceiverAttempt.h"
#include "GPSReceiverAttemptReducer.h"

class GPSRecordingBuffer;

/// Owns a receiver attempt and retires cancelled workers without blocking the UI.
class GPSReceiverSession : public QObject
{
    Q_OBJECT

    friend class GPSReceiverSessionTest;
    friend class GPSReceiverTest;
    friend class GPSBaseStationStateTest;

public:
    /// Clock functions are called on both session and worker threads; do not capture a thread-confined scheduler.
    explicit GPSReceiverSession(QObject* parent = nullptr, GPSExecutionContext context = {});
    ~GPSReceiverSession() override;

    void start(const GPSReceiverProfile& profile, GPSProvider::TransportFactory factory);
    void stop();

    /// Set at composition time; newly created attempts share this bounded recorder.
    void setRecordingBuffer(const std::shared_ptr<GPSRecordingBuffer>& buffer) { _recordingBuffer = buffer; }
    /// Join all workers during final application shutdown, without an event loop.
    void shutdown();

    bool ready() const { return _attempt.ready(); }

    bool readyForCorrections() const;
    /// Returns queue acceptance, not device acknowledgement. Call on the session thread.
    bool submitCorrections(const QByteArray& data, qint64 receivedAtMs, quint64 sessionId);
    GPSCorrectionSubmitResult submitCorrections(const GPSCorrectionFrame& frame, quint64 sessionId);
    void clearPendingCorrections();
    GPSReceiverMailbox::Stats deliveryStats() const;

    bool hasReceiver() const { return !_provider.isNull(); }

    bool stopping() const { return !_retiring.isEmpty(); }

    quint64 sessionId() const { return _generation; }

    const GPSReceiverConfig& config() const { return profile().receiver; }

    const GPSReceiverProfile& profile() const;

    const GPSReceiverAttempt& attempt() const { return _attempt; }

    QIODevice* nmeaDevice() const { return _nmeaStream.get(); }

    const GPSReceiverCapabilities& capabilities() const { return _capabilities; }

    QString errorDetail() const { return _attempt.errorDetail; }

    const GPSConfigurationReport& configurationReport() const { return _configurationReport; }

signals:
    void attemptChanged(const GPSReceiverAttempt& attempt);
    void receiverTypeChanged(GPSType type);
    void configurationStarted();
    void configurationReported(const GPSConfigurationReport& report);
    void receiverReady();
    void disconnected();
    void connectionError(GPSConnectionError error);
    void connectionErrorDetail(GPSConnectionError error, const QString& detail);
    void capabilitiesUpdated(const GPSReceiverCapabilities& capabilities);
    void stateChanged();
    void positionReceived(const GPSObservation& observation);
    void integrityReceived(const GPSIntegrityObservation& observation);
    void satellitesReceived(const GPSSatelliteObservation& observation);
    void relativePositionReceived(const GPSRelativeObservation& observation);
    void rtcmReceived(const QByteArray& data);
    void rtcmFrameReceived(const QByteArray& data, qint64 receivedAtMs, quint64 sessionId);
    void surveyInReceived(const GPSSurveyInStatus& status);
    void correctionDeliveriesReady(const QList<GPSCorrectionDelivery>& deliveries);

private:
    void _applyEvent(const GPSReceiverEvent& event);
    bool _transition(GPSReceiverAttempt::Phase phase);
    void _finishAttempt(GPSConnectionError error = GPSConnectionError::None);
    void _invalidateConfigurationReport();
    void _drain(const std::shared_ptr<GPSReceiverMailbox>& mailbox, quint64 generation);
    void _flushDeliveries(const std::shared_ptr<GPSReceiverMailbox>& mailbox, quint64 generation);

    GPSExecutionContext _clock;
    std::shared_ptr<GPSRecordingBuffer> _recordingBuffer;
    QPointer<GPSProvider> _provider;
    std::unique_ptr<GPSByteStream> _nmeaStream;
    QSet<GPSProvider*> _workers;
    QSet<GPSProvider*> _started;
    QSet<GPSProvider*> _retiring;
    quint64 _generation = 0;
    bool _shutdown = false;
    bool _configurationTerminal = true;
    GPSReceiverAttempt _attempt;
    GPSReceiverCapabilities _capabilities;
    GPSConfigurationReport _configurationReport;
};
