#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <optional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSSourceHealth.h"
#include "ScheduledTask.h"

/// A receiver binding of PositionManager. Releasing the last copy, on the manager's thread, retires that binding
/// only, never a newer one.
using GPSPositionSourceRegistration = std::shared_ptr<const void>;

/// Chooses the ground-station position between a GNSS receiver and this device's position source, and publishes it.
/// Automatic mode prefers a usable receiver, returning to it only after it has stayed usable for RECOVERY_DELAY; the
/// pinned modes publish only observations made after the selection. init() requests location permission and binds the
/// platform source; tests bind their own with setInternalPositionSource() instead. Use from the manager's thread only.
///
/// Two receivers may register, and Configuration::receiver names the one that is the receiver source: the RTK
/// receiver, or the position receiver while one is set up, whether or not that one is connected or has a position.
/// The other receiver's position is never published. A base station's antenna position therefore does not stand in
/// for a position receiver that is silent or disconnected: Automatic mode publishes this device's position then, and
/// ReceiverOnly none. Freshness, accuracy and session are checked alike for both receivers.
class PositionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by QGroundControl")

    Q_PROPERTY(QString selectedSourceName READ selectedSourceName NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QString sourceStatusText READ sourceStatusText NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QGeoCoordinate gcsPosition READ gcsPosition NOTIFY gcsPositionChanged FINAL)
    Q_PROPERTY(qreal gcsHeading READ gcsHeading NOTIFY gcsHeadingChanged FINAL)
    Q_PROPERTY(qreal gcsPositionHorizontalAccuracy READ gcsPositionHorizontalAccuracy NOTIFY
                   gcsPositionHorizontalAccuracyChanged FINAL)

    friend class PositionManagerTest;

public:
    /// Values of RTKSettings::gcsPositionSource.
    enum class SourceMode
    {
        Automatic = 0,
        ReceiverOnly = 1,
        InternalOnly = 2,
    };
    Q_ENUM(SourceMode)

    enum class SelectedSource
    {
        None,
        Receiver,
        Internal,
    };
    Q_ENUM(SelectedSource)

    /// The receivers that register their position.
    enum class Receiver
    {
        /// The receiver of the GNSS Receiver settings: an RTK base station, or a passive receiver.
        Rtk,
        /// The position receiver, which is never configured and only provides the ground station position.
        Position,
    };
    Q_ENUM(Receiver)

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

    /// Creates a position source parented to @a parent, or returns null.
    using SourceFactory = std::function<QGeoPositionInfoSource*(QObject* parent)>;

    struct Configuration
    {
        SourceMode sourceMode = SourceMode::Automatic;
        /// The receiver that is the receiver source, connected or not.
        Receiver receiver = Receiver::Rtk;
        bool operator==(const Configuration&) const = default;
    };

    static constexpr std::chrono::milliseconds RECOVERY_DELAY{5000};

    explicit PositionManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~PositionManager() override;

    /// Ignored after shutdown().
    void setConfiguration(const Configuration& configuration);

    const Configuration& configuration() const { return _configuration; }

    /// Creates the platform source once location permission is granted; without a factory, or when it returns null,
    /// Qt's default source is used.
    void setPlatformSourceFactory(SourceFactory factory);

    void init();
    /// Releases the position sources and ignores later configuration; init() does not restart them.
    void shutdown();

    SelectedSource selectedSource() const { return _selectedSource; }

    QString selectedSourceName() const;

    SourceStatus sourceStatus() const { return _sourceStatus; }

    QString sourceStatusText() const;

    /// Borrows this device's position source, which runs unless the mode is ReceiverOnly. The application owns
    /// permission requests and source creation; @a custom marks a plugin-provided source. @a altitudeDatum is the
    /// datum of the source's altitudes, applied to fixes that report their vertical accuracy.
    void setInternalPositionSource(QGeoPositionInfoSource* source, SourceStatus status, bool custom = false,
                                   GPSAltitudeDatum altitudeDatum = GPSAltitudeDatum::Unknown);

    /// The receiver publishes observations through @a producer and runs independently of selection; its observations
    /// must carry @a sessionId when it is nonzero. The binding lasts until the registration is reset, and replaces
    /// that of the same @a receiver only.
    GPSPositionSourceRegistration registerReceiver(GPSSourceHealth* producer, quint64 sessionId = 0,
                                                   Receiver receiver = Receiver::Rtk);

    QGeoCoordinate gcsPosition() const { return _published.position; }

    qreal gcsHeading() const { return _published.heading; }

    qreal gcsPositionHorizontalAccuracy() const { return _published.horizontalAccuracy; }

    std::optional<GPSObservation> acceptedObservation(
        GPSObservation::PositionUse use = GPSObservation::PositionUse::GroundStation) const;

signals:
    void selectionChanged();
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void gcsPositionHorizontalAccuracyChanged(qreal gcsPositionHorizontalAccuracy);

private:
    struct Publication
    {
        QGeoCoordinate position;
        qreal heading = qQNaN();
        qreal horizontalAccuracy = std::numeric_limits<qreal>::infinity();
    };

    void _setInternalPositionStatus(SourceStatus status);
    /// Health of the source bound for @a kind, or null when none is.
    GPSSourceHealth* _sourceHealth(SelectedSource kind) const;
    /// Returns the token of this binding; the observers it notifies may already have replaced it.
    quint64 _setReceiver(Receiver receiver, GPSSourceHealth* producer, quint64 sessionId);
    void _retireReceiver(Receiver receiver, quint64 token);
    void _releaseDevice();
    void _setDeviceActive(bool active);
    void _devicePositionUpdated(const QGeoPositionInfo& position);
    void _deviceErrorOccurred(QGeoPositionInfoSource::Error error);
    void _bindingChanged(SelectedSource kind);
    /// Re-evaluates selection, publication and status, then notifies changes. @a reporter has a new observation.
    void _update(SelectedSource reporter = SelectedSource::None);
    SelectedSource _choose();
    void _republish();
    void _updateStatus();
    void _emitChanges();
    void _setPositioningError(QGeoPositionInfoSource::Error error);
    std::optional<GPSObservation> _accepted(
        SelectedSource kind, GPSObservation::PositionUse use = GPSObservation::PositionUse::GroundStation) const;
    void _setupPositionSources();
    /// The datum of the altitudes that Qt's position backend @a sourceName reports.
    static GPSAltitudeDatum _backendAltitudeDatum(const QString& sourceName);
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();

    /// What a receiver registered.
    struct ReceiverBinding
    {
        QPointer<GPSSourceHealth> health;
        quint64 session = 0;
        quint64 token = 0;
    };

    ReceiverBinding& _binding(Receiver receiver) { return _receivers[static_cast<size_t>(receiver)]; }

    /// The binding of the receiver that is the receiver source.
    const ReceiverBinding& _receiverSource() const { return _receivers[static_cast<size_t>(_configuration.receiver)]; }

    RuntimeScheduler* const _scheduler;
    ScheduledTask _recoveryTask;
    std::optional<qint64> _receiverUsableSinceMs;

    /// By Receiver.
    std::array<ReceiverBinding, 2> _receivers;

    QPointer<QGeoPositionInfoSource> _device;
    GPSSourceHealth* const _deviceHealth;
    bool _deviceActive = false;
    bool _usingPluginSource = false;
    GPSAltitudeDatum _deviceAltitudeDatum = GPSAltitudeDatum::Unknown;
    SourceStatus _platformStatus = SourceStatus::NoSource;

    Configuration _configuration;
    SelectedSource _selectedKind = SelectedSource::Internal;
    QPointer<GPSSourceHealth> _currentHealth;
    bool _reselect = false;
    quint64 _selectionRevision = 0;
    bool _authorized = false;

    SelectedSource _selectedSource = SelectedSource::None;
    SourceStatus _sourceStatus = SourceStatus::NoSource;
    QGeoPositionInfoSource::Error _gcsPositioningError = QGeoPositionInfoSource::NoError;
    Publication _published;

    struct Notified
    {
        struct Selection
        {
            SelectedSource source = SelectedSource::None;
            SourceStatus status = SourceStatus::NoSource;
            QString name;
            bool operator==(const Selection&) const = default;
        } selection;

        Publication publication;
    } _notified;

    SourceFactory _platformSourceFactory;
    bool _shutdown = false;
};
