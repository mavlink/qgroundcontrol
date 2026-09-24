#pragma once

#include <array>
#include <optional>

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoPositionInfoSource>

#include "GPSObservation.h"
#include "NMEALineFramer.h"
#include "NMEANavigationEpoch.h"
#include "NMEASentenceEnvelope.h"
#include "RuntimeScheduler.h"
#include "ScheduledTask.h"

class QIODevice;

/// Owns single-pass NMEA framing, position publication cadence, and single-request deadlines.
class NMEAPositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT

    friend class GPSAsciiProtocolTest;
    friend class NMEAPositionSourceTest;

public:
    explicit NMEAPositionSource(QIODevice* device, QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NMEAPositionSource() override;

    void setUpdateInterval(int msec) override;
    QGeoPositionInfo lastKnownPosition(bool satelliteOnly = false) const override;
    PositioningMethods supportedPositioningMethods() const override;
    int minimumUpdateInterval() const override;
    Error error() const override;

signals:
    void dataReceived(quint64 receivedAtUs);
    void sentenceReceived(const NMEASentenceEnvelope& sentence);
    void closed();
    void observationReceived(const GPSObservation& observation);

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

private:
    void _resetDecoder();
    void _readAvailableData();
    void _discardAvailableData();
    void _closeInput();
    void _processSentence(const NMEASentenceEnvelope& envelope);
    void _queueEpoch(const NMEA::NavigationEpoch& epoch);
    QDateTime _receiptTime(quint64 timestampUs) const;
    static GPSObservation _observation(const NMEA::NavigationEpoch& epoch, const QDateTime& receivedAt);
    static GPSObservation _lossObservation(const NMEA::NavigationEpoch& epoch, const QDateTime& receivedAt);
    void _fixLost(const GPSObservation& observation);
    void _publishLoss();
    void _publishPending();
    void _schedulePublication();
    void _publishError(Error error);

    GPSObservation _lastObservation;
    QGeoPositionInfo _lastKnownPosition;
    QPointer<QIODevice> _device;
    RuntimeScheduler* const _scheduler;
    ScheduledTask _requestTask;
    ScheduledTask _publicationTask;
    ScheduledTask _lossTask;
    ScheduledTask _errorTask;
    std::optional<GPSObservation> _pendingLoss;

    struct PendingFix
    {
        std::optional<GPSObservation> observation;
        std::optional<QGeoPositionInfo> position;
        std::optional<int> epochTimeMs;
        quint64 epochRevision = 0;
        bool requested = false;
    } _pendingFix;

    QHash<int, quint64> _publishedEpochs;
    std::array<char, 1024> _sentenceBuffer{};
    NMEA::LineFramer _lineFramer;
    NMEA::NavigationEpochAssembler _navigationAssembler;
    quint64 _sentenceTimestampUs = 0;
    bool _closed = false;
    bool _drainPending = false;
    Error _error = NoError;
    quint64 _generation = 0;
    bool _started = false;
};
