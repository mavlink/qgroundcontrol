#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
#include <QtPositioning/QGeoPositionInfoSource>

#include "GPSNotificationQueue.h"
#include "GPSPositionBackendAdapter.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSSourceHealth.h"
#include "ScheduledTask.h"

class GPSPositionService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(SourceMode sourceMode READ sourceMode NOTIFY sourceModeChanged)
    Q_PROPERTY(SelectedSource selectedSource READ selectedSource NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSourceName READ selectedSourceName NOTIFY selectionChanged)
    Q_PROPERTY(SourceStatus sourceStatus READ sourceStatus NOTIFY selectionChanged)
    Q_PROPERTY(QString sourceStatusText READ sourceStatusText NOTIFY selectionChanged)
    Q_PROPERTY(QGeoCoordinate gcsPosition READ gcsPosition NOTIFY gcsPositionChanged)
    Q_PROPERTY(qreal gcsHeading READ gcsHeading NOTIFY gcsHeadingChanged)
    Q_PROPERTY(qreal gcsPositionHorizontalAccuracy READ gcsPositionHorizontalAccuracy NOTIFY
                   gcsPositionHorizontalAccuracyChanged)

    friend class GPSPositionSourceRegistration;

public:
    /// Values of AutoConnectSettings::gcsPositionSource.
    enum class SourceMode
    {
        Automatic = 0,
        ReceiverOnly = 1,
        InternalOnly = 3,
    };
    Q_ENUM(SourceMode)

    enum class SelectedSource
    {
        None,
        Receiver,
        Internal,
        Simulated,
    };
    Q_ENUM(SelectedSource)

    enum class SourceStatus
    {
        NoSource,
        PermissionRequired,
        PermissionDenied,
        BackendUnavailable,
        WaitingForFix,
        Active,
        Stale,
        InvalidFix,
    };
    Q_ENUM(SourceStatus)

    SourceMode sourceMode() const { return _sourceMode; }

    void setSourceMode(SourceMode mode);

    SelectedSource selectedSource() const { return _selectedSource; }

    QString selectedSourceName() const;

    SourceStatus sourceStatus() const { return _sourceStatus; }

    QString sourceStatusText() const;

    explicit GPSPositionService(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSPositionService() override;

    /// Borrow a platform source; the application owns permission requests and source creation.
    void setInternalPositionSource(QGeoPositionInfoSource* source, SourceStatus status, bool custom = false);

    void setInternalPositionStatus(SourceStatus status);
    void setSimulatedPositionSource(QGeoPositionInfoSource* source);

    QGeoCoordinate gcsPosition() const { return _published.position; }

    qreal gcsHeading() const { return _published.heading; }

    qreal gcsPositionHorizontalAccuracy() const { return _published.horizontalAccuracy; }

    std::optional<GPSObservation> acceptedObservation(
        GPSObservation::PositionUse use = GPSObservation::PositionUse::GroundStation,
        std::optional<std::chrono::milliseconds> maximumAge = std::nullopt) const;

    QGeoPositionInfoSource::Error gcsPositioningError() const { return _gcsPositioningError; }

    int updateInterval() const { return _updateInterval; }

    /// Health of the producer registered for @a kind, or null when none is.
    GPSSourceHealth* sourceHealth(SelectedSource kind) const;

    /// Health of the selected producer, or null when no source is selected.
    GPSSourceHealth* selectedHealth() const { return _currentHealth.data(); }

    /// Every producer publishes observations through a GPSSourceHealth. A receiver producer runs independently of
    /// selection; its observations must carry @a sessionId when it is nonzero.
    GPSPositionSourceRegistration registerPositionSource(SelectedSource kind, GPSSourceHealth* producer,
                                                         quint64 sessionId = 0);

    /// Adapts a Qt positioning backend, which runs only while its role is active. A backend serves one role.
    GPSPositionSourceRegistration registerPositionSource(SelectedSource kind, QGeoPositionInfoSource* backend,
                                                         quint64 sessionId = 0);

signals:
    void sourceModeChanged();
    void selectionChanged();
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void gcsPositionHorizontalAccuracyChanged(qreal gcsPositionHorizontalAccuracy);

public:
    RuntimeScheduler* scheduler() const { return _scheduler; }

private slots:
    void _positionError(QGeoPositionInfoSource::Error gcsPositioningError);

private:
    struct SourceBinding
    {
        SourceBinding(GPSPositionService* owner, SelectedSource kind);
        ~SourceBinding();
        /// Binds @a producer, which may be null; a backend adapter, when given, owns that producer.
        void bind(GPSSourceHealth* producer, std::unique_ptr<GPSPositionBackendAdapter> adapter, quint64 sessionId);
        void disconnectNotifications();
        void observeHealth(bool observe);
        void setActive(bool active);

        GPSSourceHealth* health() const { return producer.data(); }

        QGeoPositionInfoSource* backend() const;
        int updateInterval() const;

        GPSPositionService* owner;
        SelectedSource kind;
        quint64 token = 0;
        bool pendingObservation = false;
        QPointer<GPSSourceHealth> producer;
        std::unique_ptr<GPSPositionBackendAdapter> adapter;
        QMetaObject::Connection producerDestroyed;
        QMetaObject::Connection backendDestroyed;
        QMetaObject::Connection observationConnection;
        quint64 sessionId = 0;
        bool active = false;
        quint64 generation = 0;
    };

    SourceBinding& _binding(SelectedSource kind) const { return *_bindings[static_cast<size_t>(kind)]; }

    void _retireRegistration(int kind, quint64 token);
    bool _canBindProducer(const QObject* producer) const;
    bool _canBindBackend(SelectedSource kind, QGeoPositionInfoSource* backend) const;
    void _bindProducer(SelectedSource kind, GPSSourceHealth* producer, quint64 sessionId);
    void _bindBackend(SelectedSource kind, QGeoPositionInfoSource* backend, quint64 sessionId);
    GPSPositionSourceRegistration _register(SelectedSource kind);
    void _setPositionSource(SelectedSource source);
    void _selectPositionSource();
    SelectedSource _choosePositionSource();
    GPSSourceHealth* _sourceFor(SelectedSource source) const;
    void _bindingChanged(SelectedSource kind);
    void _backendError(SelectedSource kind, QGeoPositionInfoSource::Error error);
    void _clearPendingObservations();
    void _sourceObservationChanged(SelectedSource kind);
    void _updateSourceActivity();
    void _updateSelectionStatus();
    void _clearPosition();
    void _externalPositionChanged();
    void _publishPosition(const std::optional<GPSObservation>& observation);

    RuntimeScheduler* const _scheduler;
    ScheduledTask _recoveryTask;
    std::array<std::unique_ptr<SourceBinding>, 4> _bindings;

    struct Recovery
    {
        std::optional<SelectedSource> candidate;
        qint64 sinceMs = 0;
    } _recovery;
    SelectedSource _selectedKind = SelectedSource::Internal;
    SourceMode _sourceMode = SourceMode::Automatic;
    SelectedSource _selectedSource = SelectedSource::None;
    SourceStatus _sourceStatus = SourceStatus::NoSource;
    SourceStatus _platformStatus = SourceStatus::NoSource;
    QString _selectionName;
    static constexpr std::chrono::milliseconds RECOVERY_DELAY{5000};
    bool _selectingSource = false;
    bool _selectionPending = false;
    bool _selectionPublicationPending = false;
    bool _forceSourceRefresh = false;
    bool _usingPluginSource = false;
    int _updateInterval = 0;
    std::optional<GPSObservation> _acceptedSourceObservation(
        SelectedSource source, GPSObservation::PositionUse use = GPSObservation::PositionUse::GroundStation,
        std::optional<std::chrono::milliseconds> maximumAge = std::nullopt) const;
    QPointer<GPSSourceHealth> _currentHealth;

    QGeoPositionInfoSource::Error _gcsPositioningError = QGeoPositionInfoSource::NoError;

    struct NotifiedPosition
    {
        QGeoCoordinate position;
        qreal heading = qQNaN();
        qreal horizontalAccuracy = std::numeric_limits<qreal>::infinity();
    } _notified;

    NotifiedPosition _published;

    quint64 _selectedBindingRevision = 0;
    quint64 _selectionObservationRevision = 0;
    bool _selectedObservationAuthorized = false;
    // Declared last so bindings stop producing notifications before the queue is destroyed.
    GPSNotificationQueue _notifications{this};
};
