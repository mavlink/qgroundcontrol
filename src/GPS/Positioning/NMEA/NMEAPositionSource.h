#pragma once

#include <optional>

#include <QtCore/QDate>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoPositionInfoSource>

#include "GPSObservation.h"
#include "NMEASentenceEnvelope.h"
#include "RuntimeScheduler.h"
#include "ScheduledTask.h"

class QIODevice;

/// Owns single-pass NMEA framing, position publication cadence, and single-request deadlines.
class NMEAPositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT

    friend class NMEAPositionSourceTest;

public:
    explicit NMEAPositionSource(QIODevice* device, QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NMEAPositionSource() override;

    void setUpdateInterval(int msec) override;
    QGeoPositionInfo lastKnownPosition(bool satelliteOnly = false) const override;
    PositioningMethods supportedPositioningMethods() const override;
    int minimumUpdateInterval() const override;
    Error error() const override;

    GPSObservation lastObservation() const { return _lastObservation; }

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
    struct EpochMetadata
    {
        GPSObservation observation;
        std::optional<unsigned> ggaQuality;
        std::optional<unsigned> dimension;
        quint64 navigationSequence = 0;
        bool published = false;
    };

    void _resetDecoder();
    void _readAvailableData();
    void _discardAvailableData();
    void _closeInput();
    void _processSentence(const NMEASentenceEnvelope& envelope);
    void _handlePositionSentence(const NMEASentenceEnvelope& envelope);
    void _handleUntimedMetadata(const NMEASentenceEnvelope& envelope);
    void _handleAccuracy(const NMEASentenceEnvelope& envelope);
    void _handleDatedSentence(const NMEASentenceEnvelope& envelope);
    void _queueEpoch(int timeMs);
    void _trimEpochs();
    QDate _dateForTime(int timeMs) const;
    void _setDateReference(const QDate& date, int timeMs);
    QDateTime _timestamp(int timeMs) const;
    QDateTime _receiptTime(quint64 timestampUs) const;
    bool _acceptNavigationStatus(std::optional<int> timeMs, const QDate& date, quint64 receivedAtUs);
    static void _resetExpiredEpoch(EpochMetadata& epoch, const QDateTime& timestamp, quint64 receivedAtUs);
    static GPSObservation::FixQuality _fixQuality(const EpochMetadata& epoch);
    void _fixLost(GPSObservation observation);
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
        bool requested = false;
    } _pendingFix;

    QHash<int, EpochMetadata> _epochs;
    std::optional<int> _currentEpochMs;
    QDate _dateReference;
    std::optional<int> _dateReferenceTimeMs;
    quint64 _statusReceiptUs = 0;
    std::optional<int> _statusTimeMs;
    QDate _statusDate;
    quint64 _sentenceSequence = 0;
    quint64 _invalidThroughSequence = 0;
    bool _navigationValid = true;
    QByteArray _sentence;
    quint64 _sentenceTimestampUs = 0;
    bool _closed = false;
    bool _drainPending = false;
    Error _error = NoError;
    quint64 _generation = 0;
    bool _started = false;
};
