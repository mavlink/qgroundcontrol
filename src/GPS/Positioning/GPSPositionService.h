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

#include "GPSPositionSourceRegistration.h"
#include "GPSSourceHealth.h"
#include "ScheduledTask.h"

class GPSPositionService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(SourceMode sourceMode READ sourceMode WRITE setSourceMode NOTIFY sourceModeChanged)
    Q_PROPERTY(SelectedSource selectedSource READ selectedSource NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSourceName READ selectedSourceName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectionReason READ selectionReason NOTIFY selectionChanged)
    Q_PROPERTY(SourceStatus sourceStatus READ sourceStatus NOTIFY selectionChanged)
    Q_PROPERTY(QString sourceStatusText READ sourceStatusText NOTIFY selectionChanged)
    Q_PROPERTY(GPSSourceHealth* sourceHealth READ sourceHealth NOTIFY sourceHealthChanged)
    Q_PROPERTY(QGeoCoordinate gcsPosition READ gcsPosition NOTIFY gcsPositionChanged)
    Q_PROPERTY(qreal gcsHeading READ gcsHeading NOTIFY gcsHeadingChanged)
    Q_PROPERTY(qreal gcsPositionHorizontalAccuracy READ gcsPositionHorizontalAccuracy NOTIFY
                   gcsPositionHorizontalAccuracyChanged)

    friend class GPSPositionSourceRegistration;

public:
    enum class SourceMode
    {
        LegacyPriority = 0,
        Automatic = 1,
        ReceiverOnly = 2,
        NmeaOnly = 3,
        InternalOnly = 4,
    };
    Q_ENUM(SourceMode)

    enum class SelectedSource
    {
        None,
        Receiver,
        Nmea,
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

    QString selectionReason() const { return _selectionReason; }

    SourceStatus sourceStatus() const { return _sourceStatus; }

    QString sourceStatusText() const;

    explicit GPSPositionService(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSPositionService() override;

    /// Borrow a platform source; the application owns permission requests and source creation.
    void setInternalPositionSource(QGeoPositionInfoSource* source, SourceStatus status, bool custom = false);

    void setInternalPositionStatus(SourceStatus status);
    void setSimulatedPositionSource(QGeoPositionInfoSource* source);

    GPSSourceHealth* sourceHealth() const { return _currentHealth; }

    QGeoCoordinate gcsPosition() const { return _published.position; }

    qreal gcsHeading() const { return _published.heading; }

    qreal gcsPositionHorizontalAccuracy() const { return _published.horizontalAccuracy; }

    QGeoPositionInfo geoPositionInfo() const { return _published.info; }

    std::optional<GPSObservation> acceptedObservation(
        GPSObservation::PositionUse use = GPSObservation::PositionUse::GroundStation,
        std::optional<std::chrono::milliseconds> maximumAge = std::nullopt) const;

    QGeoPositionInfoSource::Error gcsPositioningError() const { return _gcsPositioningError; }

    /// Local arrival time of the last position update which passed the accuracy gates and was
    /// copied into gcsPosition. Invalid until the first such update arrives. This is the local
    /// clock rather than the position source's own timestamp, which on some platforms (e.g.
    /// Android) is offset from the system clock.
    ///     @return Arrival time, in UTC, of the last position update applied to gcsPosition.
    QDateTime gcsPositionTimestamp() const { return _published.timestamp; }

    int updateInterval() const { return _updateInterval; }

    /// Raw Qt positioning sources require an exclusive binding; shared producers must supply health.
    GPSPositionSourceRegistration registerPositionSource(SelectedSource kind, QObject* source, GPSSourceHealth* health,
                                                         quint64 sessionId = 0);

signals:
    void sourceModeChanged();
    void selectionChanged();
    void sourceHealthChanged();
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void positionInfoUpdated(QGeoPositionInfo update);
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
        void configure(QObject* producer, GPSSourceHealth* health, const QString& identity, bool platform,
                       quint64 sessionId);
        void disconnectNotifications();
        void disconnectSource();
        void observeHealth(bool observe);
        void setActive(bool active);
        void updatePosition(const QGeoPositionInfo& position);
        QObject* source() const;
        GPSSourceHealth* health();
        int updateInterval() const;

        GPSPositionService* owner;
        SelectedSource kind;
        quint64 token = 0;
        bool pendingObservation = false;
        QPointer<QObject> producer;
        QPointer<QGeoPositionInfoSource> rawSource;
        QPointer<GPSSourceHealth> providedHealth;
        GPSSourceHealth fallbackHealth;
        QList<QMetaObject::Connection> connections;
        QMetaObject::Connection observationConnection;
        QString identity;
        quint64 sessionId = 0;
        bool platform = false;
        bool active = false;
        bool updatesStarted = false;
        bool rawBindingAllowed = true;
        quint64 generation = 0;
        quint64 backendRevision = 0;
    };

    SourceBinding& _binding(SelectedSource kind) const { return *_bindings[static_cast<size_t>(kind)]; }

    void _retireRegistration(int kind, quint64 token);
    bool _canBindSource(SelectedSource kind, QObject* source, GPSSourceHealth* health = nullptr) const;
    void _setBinding(SelectedSource kind, QObject* source, GPSSourceHealth* health = nullptr, quint64 sessionId = 0);
    void _setPositionSource(SelectedSource source);
    void _selectPositionSource();
    SelectedSource _choosePositionSource();
    QObject* _sourceFor(SelectedSource source) const;
    void _refreshSourceBindings();
    void _bindingChanged(SelectedSource kind);
    void _backendError(SelectedSource kind, QGeoPositionInfoSource::Error error);
    void _clearPendingObservations();
    void _sourceObservationChanged(SelectedSource kind);
    void _updateSourceActivity();
    void _updateSelectionStatus();
    void _clearPosition();
    void _externalPositionChanged();
    void _publishPosition(const std::optional<GPSObservation>& observation);

    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _recoveryTask;
    std::array<std::unique_ptr<SourceBinding>, 5> _bindings;

    struct Recovery
    {
        std::optional<SelectedSource> candidate;
        qint64 sinceMs = 0;
    } _recovery;
    SelectedSource _selectedKind = SelectedSource::Internal;
    SourceMode _sourceMode = SourceMode::LegacyPriority;
    SelectedSource _selectedSource = SelectedSource::None;
    SourceStatus _sourceStatus = SourceStatus::NoSource;
    SourceStatus _platformStatus = SourceStatus::NoSource;
    QString _selectionReason;
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

    struct PublishedPosition : NotifiedPosition
    {
        QGeoPositionInfo info;
        QDateTime timestamp;
    } _published;

    quint64 _sourceGeneration = 0;
    quint64 _selectedBindingRevision = 0;
    quint64 _selectionObservationRevision = 0;
    bool _selectedObservationAuthorized = false;
    quint64 _positionRevision = 0;
    QPointer<QObject> _currentSource;
};
