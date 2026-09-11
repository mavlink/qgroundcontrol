#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtQmlIntegration/QtQmlIntegration>

#include <array>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>

#include "GPSPositionSourceAdapter.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSPositionSourceSelector.h"
#include "GPSSourceHealth.h"
#include "ScheduledTask.h"

class GPSPositionService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

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

    QGeoCoordinate gcsPosition() const { return _gcsPosition; }

    qreal gcsHeading() const { return _gcsHeading; }

    qreal gcsPositionHorizontalAccuracy() const { return _gcsPositionHorizontalAccuracy; }

    QGeoPositionInfo geoPositionInfo() const { return _geoPositionInfo; }

    std::optional<GPSObservation> acceptedObservation(GPSObservation::PositionUse use) const;

    QGeoPositionInfoSource::Error gcsPositioningError() const { return _gcsPositioningError; }

    /// Local arrival time of the last position update which passed the accuracy gates and was
    /// copied into gcsPosition. Invalid until the first such update arrives. This is the local
    /// clock rather than the position source's own timestamp, which on some platforms (e.g.
    /// Android) is offset from the system clock.
    ///     @return Arrival time, in UTC, of the last position update applied to gcsPosition.
    QDateTime gcsPositionTimestamp() const { return _gcsPositionTimestamp; }

    int updateInterval() const { return _updateInterval; }

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

private slots:
    void _positionError(QGeoPositionInfoSource::Error gcsPositioningError);

private:
    struct SourceBinding
    {
        QPointer<QObject> source;
        QPointer<GPSSourceHealth> health;
        quint64 session = 0;
        quint64 token = 0;
        QMetaObject::Connection destroyedConnection;
        std::unique_ptr<GPSPositionSourceAdapter> adapter;
    };

    SourceBinding& _binding(SelectedSource kind) { return _bindings[static_cast<size_t>(kind)]; }

    const SourceBinding& _binding(SelectedSource kind) const { return _bindings[static_cast<size_t>(kind)]; }
    void _retireRegistration(int kind, quint64 token);
    void _setBinding(SelectedSource kind, QObject* source, GPSSourceHealth* health = nullptr, quint64 sessionId = 0);
    void _setPositionSource(SelectedSource source);
    void _selectPositionSource();
    SelectedSource _choosePositionSource();
    QObject* _sourceFor(SelectedSource source) const;
    void _refreshSourceAdapters();
    void _updateSourceActivity();
    void _updateSelectionStatus();
    void _clearPosition();
    void _externalPositionChanged();
    void _publishPosition(const std::optional<GPSObservation>& observation);

    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _recoveryTask;
    std::array<SourceBinding, 5> _bindings;
    GPSPositionSourceSelector _selector;
    SelectedSource _selectedKind = SelectedSource::Internal;
    SourceMode _sourceMode = SourceMode::LegacyPriority;
    SelectedSource _selectedSource = SelectedSource::None;
    SourceStatus _sourceStatus = SourceStatus::NoSource;
    SourceStatus _platformStatus = SourceStatus::NoSource;
    QString _selectionReason;
    static constexpr std::chrono::milliseconds RECOVERY_DELAY{5000};
    bool _selectingSource = false;
    bool _selectionPending = false;
    bool _forceSourceRefresh = false;
    bool _usingPluginSource = false;
    int _updateInterval = 0;
    std::optional<GPSObservation> _acceptedSourceObservation(SelectedSource source,
                                                             GPSObservation::PositionUse use) const;
    QPointer<GPSSourceHealth> _currentHealth;
    QMetaObject::Connection _healthConnection;
    QMetaObject::Connection _healthDestroyedConnection;

    QGeoPositionInfo _geoPositionInfo;
    QGeoPositionInfoSource::Error _gcsPositioningError = QGeoPositionInfoSource::NoError;

    QGeoCoordinate _gcsPosition;
    QDateTime _gcsPositionTimestamp;
    qreal _gcsHeading = qQNaN();
    qreal _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsPositionVerticalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsPositionAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsDirectionAccuracy = std::numeric_limits<qreal>::infinity();

    quint64 _sourceGeneration = 0;
    quint64 _positionRevision = 0;
    QPointer<QObject> _currentSource;
};
