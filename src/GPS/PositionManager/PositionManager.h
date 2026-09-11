#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtQmlIntegration/QtQmlIntegration>

#include <array>
#include <functional>
#include <memory>

#include "GPSPositionSourceAdapter.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSPositionSourceSelector.h"
#include "GPSSourceHealth.h"

class QGCCompass;

class QGCPositionManager : public QObject
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

    friend class PositionManagerTest;
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

    explicit QGCPositionManager(QObject* parent = nullptr);
    ~QGCPositionManager();

    /// Gets the ground-station position manager.
    ///     @return The singleton instance.
    static QGCPositionManager* instance();

    void init();

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

    /// Select a borrowed, connected receiver source ahead of NMEA and platform positioning.
    void setReceiverPositionSource(QObject* source, GPSSourceHealth* health = nullptr, quint64 sessionId = 0);
    void clearReceiverPositionSource(QObject* source);
    /// Borrow an NMEA position source; its owner manages the decoder and input device.
    void setNmeaPositionSource(QObject* source, GPSSourceHealth* health = nullptr, quint64 sessionId = 0);
    void clearNmeaPositionSource(QObject* source);

signals:
    void sourceModeChanged();
    void selectionChanged();
    void sourceHealthChanged();
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void positionInfoUpdated(QGeoPositionInfo update);
    void gcsPositionHorizontalAccuracyChanged(qreal gcsPositionHorizontalAccuracy);

private slots:
    void _positionUpdated(const QGeoPositionInfo& update);
    void _positionError(QGeoPositionInfoSource::Error gcsPositioningError);

private:
    enum QGCPositionSource
    {
        Simulated,
        InternalGPS,
        Log,
        NmeaGPS,
        ExternalGPS
    };

    void _retireRegistration(int kind, quint64 token);
    std::array<quint64, 5> _registrationTokens{};
    void _setPositionSource(QGCPositionSource source);
    void _selectPositionSource();
    QGCPositionSource _choosePositionSource();
    QObject* _sourceFor(QGCPositionSource source) const;
    void _refreshSourceAdapters();
    void _updateSourceActivity();
    void _updateSelectionStatus();
    bool _isExternalSource() const;
    void _setupPositionSources();
    void _setupPositionSources(
        QGeoPositionInfoSource* customSource,
        const std::function<QGeoPositionInfoSource*(const QString&, QObject*)>& createPlatformSource);
    static QString _platformSourceName();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    void _clearPosition();
    void _externalPositionChanged();
    void _publishPosition(const std::optional<GPSObservation>& observation);

    std::array<std::unique_ptr<GPSPositionSourceAdapter>, 5> _sourceAdapters;
    GPSPositionSourceSelector _selector;
    QGCPositionSource _selectedKind = InternalGPS;
    SourceMode _sourceMode = SourceMode::LegacyPriority;
    SelectedSource _selectedSource = SelectedSource::None;
    SourceStatus _sourceStatus = SourceStatus::NoSource;
    SourceStatus _platformStatus = SourceStatus::NoSource;
    QString _selectionReason;
    QTimer _recoveryTimer;
    bool _selectingSource = false;
    bool _selectionPending = false;
    bool _forceSourceRefresh = false;
    bool _usingPluginSource = false;
    int _updateInterval = 0;
    quint64 _receiverSession = 0;
    quint64 _nmeaSession = 0;
    std::optional<GPSObservation> _acceptedSourceObservation(QGCPositionSource source,
                                                             GPSObservation::PositionUse use) const;
    QPointer<GPSSourceHealth> _receiverHealth;
    QPointer<GPSSourceHealth> _nmeaHealth;
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

    QPointer<QObject> _receiverSource;
    QMetaObject::Connection _receiverDestroyedConnection;
    quint64 _sourceGeneration = 0;
    quint64 _positionRevision = 0;
    QMetaObject::Connection _nmeaDestroyedConnection;
    QPointer<QObject> _currentSource;
    QPointer<QGeoPositionInfoSource> _defaultSource;
    QPointer<QObject> _nmeaSource;
    QPointer<QGeoPositionInfoSource> _simulatedSource;

    QGCCompass* _compass = nullptr;
};
