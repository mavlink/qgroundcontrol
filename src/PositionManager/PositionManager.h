#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtPositioning/QGeoSatelliteInfoSource>
#include <QtQmlIntegration/QtQmlIntegration>

#include "PositionHealth.h"

Q_DECLARE_LOGGING_CATEGORY(QGCPositionManagerLog)

class GcsGpsSettings;
class QNmeaPositionInfoSource;
class QNmeaSatelliteInfoSource;
class QSerialPort;
class UdpIODevice;
class SatelliteModel;
class NmeaStreamSplitter;

class QGCPositionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QGeoCoordinate gcsPosition                   READ gcsPosition                    NOTIFY gcsPositionChanged)
    Q_PROPERTY(qreal          gcsHeading                    READ gcsHeading                     NOTIFY gcsHeadingChanged)
    Q_PROPERTY(qreal          gcsPositionHorizontalAccuracy READ gcsPositionHorizontalAccuracy  NOTIFY gcsPositionHorizontalAccuracyChanged)
    Q_PROPERTY(qreal          gcsPositionVerticalAccuracy   READ gcsPositionVerticalAccuracy    NOTIFY gcsPositionVerticalAccuracyChanged)
    Q_PROPERTY(qreal          gcsPositionAccuracy           READ gcsPositionAccuracy            NOTIFY gcsPositionAccuracyChanged)
    Q_PROPERTY(qreal          gcsDirectionAccuracy          READ gcsDirectionAccuracy           NOTIFY gcsDirectionAccuracyChanged)
    Q_PROPERTY(int            gcsSatellitesInView           READ gcsSatellitesInView            NOTIFY gcsSatellitesInViewChanged)
    Q_PROPERTY(int            gcsSatellitesInUse            READ gcsSatellitesInUse             NOTIFY gcsSatellitesInUseChanged)
    Q_PROPERTY(SatelliteModel *gcsSatelliteModel             READ gcsSatelliteModel              CONSTANT)
    Q_PROPERTY(bool           positionStale                 READ positionStale                  NOTIFY positionStaleChanged)
    Q_PROPERTY(bool           locationPermissionDenied      READ locationPermissionDenied       NOTIFY locationPermissionDeniedChanged)
    Q_PROPERTY(QString        sourceDescription             READ sourceDescription              NOTIFY sourceDescriptionChanged)
    /// Locale-safe: sourceDescription is tr()d, don't string-match it.
    Q_PROPERTY(bool           usingNmeaSource               READ usingNmeaSource                NOTIFY usingNmeaSourceChanged)
    Q_PROPERTY(PositionHealth positionHealth                READ positionHealth                 NOTIFY positionHealthChanged)

public:
    explicit QGCPositionManager(QObject *parent = nullptr);
    ~QGCPositionManager();

    static QGCPositionManager *instance();

    void init();
    QGeoCoordinate gcsPosition() const { return _gcsPosition; }
    qreal gcsHeading() const { return _gcsHeading; }
    qreal gcsPositionHorizontalAccuracy() const { return _gcsPositionHorizontalAccuracy; }
    qreal gcsPositionVerticalAccuracy() const { return _gcsPositionVerticalAccuracy; }
    qreal gcsPositionAccuracy() const { return _gcsPositionAccuracy; }
    qreal gcsDirectionAccuracy() const { return _gcsDirectionAccuracy; }
    QGeoPositionInfo geoPositionInfo() const { return _geoPositionInfo; }
    QGeoPositionInfoSource::Error gcsPositioningError() const { return _gcsPositioningError; }
    int gcsSatellitesInView() const;
    int gcsSatellitesInUse() const;
    SatelliteModel *gcsSatelliteModel() const { return _gcsSatelliteModel; }
    bool positionStale() const { return _positionStale; }
    bool locationPermissionDenied() const { return _locationPermissionDenied; }
    QString sourceDescription() const { return _sourceDescription; }
    bool usingNmeaSource() const { return _positionSource == NmeaGPS; }

    [[nodiscard]] PositionHealth positionHealth() const;

    int updateInterval() const { return _updateInterval; }

    void setExternalGPSSource(QGeoPositionInfoSource *source);

signals:
    void gcsPositionChanged(QGeoCoordinate gcsPosition);
    void gcsHeadingChanged(qreal gcsHeading);
    void positionInfoUpdated(QGeoPositionInfo update);
    void gcsPositionHorizontalAccuracyChanged(qreal gcsPositionHorizontalAccuracy);
    void gcsPositionVerticalAccuracyChanged();
    void gcsPositionAccuracyChanged();
    void gcsDirectionAccuracyChanged();
    void gcsSatellitesInViewChanged();
    void gcsSatellitesInUseChanged();
    void positionStaleChanged();
    void locationPermissionDeniedChanged();
    void sourceDescriptionChanged();
    void usingNmeaSourceChanged();
    void positionHealthChanged();

private slots:
    void _positionUpdated(const QGeoPositionInfo &update);
    void _compassUpdated(const QGeoPositionInfo &update);
    void _positionError(QGeoPositionInfoSource::Error gcsPositioningError);

private:
    enum QGCPositionSource {
        Simulated,
        InternalGPS,
        NmeaGPS,
        ExternalGPS
    };

    void _selectBestSource();
    void _setPositionSource(QGCPositionSource source);
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    void _setGCSHeading(qreal newGCSHeading);
    void _setGCSPosition(const QGeoCoordinate &newGCSPosition);
    void _checkStaleness();

    void _pollNmeaDevice();
    void _pollUdpNmeaDevice(GcsGpsSettings *settings);
#ifndef QGC_NO_SERIAL_LINK
    void _pollSerialNmeaDevice(GcsGpsSettings *settings, const QString &portSetting);
#endif
    void _openNmeaSerialPort(const QString &portName, uint32_t baud);
    void _openNmeaUdpPort(uint16_t port);
    void _closeNmeaDevice();
    void _setNmeaSource(QIODevice *device);

    void _connectSatelliteModel(QGeoSatelliteInfoSource *source);
    void _createSatelliteSource(QIODevice *nmeaDevice);
    void _destroySatelliteSource();
    void _createDefaultSatelliteSource();
    void _closeSerialPort();

    bool _usingPluginSource = false;
    int _updateInterval = 0;

    QGeoPositionInfo _geoPositionInfo;
    QGeoPositionInfoSource::Error  _gcsPositioningError = QGeoPositionInfoSource::NoError;

    QGeoCoordinate _gcsPosition;
    qreal _gcsHeading = qQNaN();
    qreal _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsPositionVerticalAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsPositionAccuracy = std::numeric_limits<qreal>::infinity();
    qreal _gcsDirectionAccuracy = std::numeric_limits<qreal>::infinity();

    QGeoPositionInfoSource *_currentSource = nullptr;
    QGeoPositionInfoSource *_defaultSource = nullptr;
    QNmeaPositionInfoSource *_nmeaSource = nullptr;
    QGeoPositionInfoSource *_simulatedSource = nullptr;
    QGeoPositionInfoSource *_externalSource = nullptr;

    // External satellite source (not owned — owned by GPSRtk)
    QGeoSatelliteInfoSource *_externalSatSource = nullptr;

    // Satellite info
    SatelliteModel *_gcsSatelliteModel = nullptr;
    QList<QGeoSatelliteInfo> _cachedInView;
    QList<QGeoSatelliteInfo> _cachedInUse;
    QGeoSatelliteInfoSource *_satelliteSource = nullptr;
    NmeaStreamSplitter *_nmeaSplitter = nullptr;

    // Position staleness
    QElapsedTimer _lastPositionTime;
    QTimer _stalenessTimer;
    bool _positionStale = true;
    bool _locationPermissionDenied = false;
    QString _sourceDescription;
    QGCPositionSource _positionSource = InternalGPS;

    // NMEA device management
    QTimer _nmeaPollTimer;
    UdpIODevice *_nmeaUdpSocket = nullptr;
    QSerialPort *_nmeaSerialPort = nullptr;
    QString _nmeaSerialPortName;
    uint32_t _nmeaSerialBaud = 0;
    // Suppress duplicate warnings while a port stays in a failing state (another
    // process holds it, permission denied, etc.) — emit once per (port,error) pair.
    QString _nmeaLastOpenFailPort;
    QString _nmeaLastOpenFailError;

    static constexpr int kMinUpdateIntervalMs = 1000;
    static constexpr int kNmeaPollIntervalMs = 1000;
    static constexpr int kPositionStaleTimeoutMs = 5000;
    static constexpr qreal kMinHorizontalAccuracyMeters = 100.;
    static constexpr qreal kMinVerticalAccuracyMeters = 10.;
    static constexpr qreal kMinDirectionAccuracyDegrees = 30.;
};
