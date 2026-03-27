#include "PositionManager.h"
#include "AudioOutput.h"
#include "SatelliteModel.h"
#include "GPSEvent.h"
#include "GPSManager.h"
#include "NmeaStreamSplitter.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "SimulatedPosition.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "GcsGpsSettings.h"

#include "DeviceInfo.h"
#include "UdpIODevice.h"

#include <cmath>

#include <QtCore/QApplicationStatic>
#include <QtCore/QPermissions>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtPositioning/QNmeaSatelliteInfoSource>

#ifndef QGC_NO_SERIAL_LINK
#include "QGCSerialPortInfo.h"
#include <QtSerialPort/QSerialPort>
#endif

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "PositionManager.QGCPositionManager")

Q_APPLICATION_STATIC(QGCPositionManager, _positionManager);

QGCPositionManager::QGCPositionManager(QObject *parent)
    : QObject(parent)
    , _gcsSatelliteModel(new SatelliteModel(this))
    , _nmeaUdpSocket(new UdpIODevice(this))
{
    qCDebug(QGCPositionManagerLog) << this;

    _stalenessTimer.setInterval(kPositionStaleTimeoutMs);
    _stalenessTimer.setSingleShot(false);
    (void) connect(&_stalenessTimer, &QTimer::timeout, this, &QGCPositionManager::_checkStaleness);
}

QGCPositionManager::~QGCPositionManager()
{
    if (_currentSource) {
        _currentSource->stopUpdates();
        (void) disconnect(_currentSource);
    }

    _destroySatelliteSource();
    _closeNmeaDevice();

    qCDebug(QGCPositionManagerLog) << this;
}

QGCPositionManager *QGCPositionManager::instance()
{
    return _positionManager();
}

void QGCPositionManager::init()
{
    if (qgcApp()->runningUnitTests()) {
        _simulatedSource = new SimulatedPosition(this);
        _setPositionSource(QGCPositionSource::Simulated);
    } else {
        _checkPermission();
    }

    (void) connect(GPSManager::instance(), &GPSManager::positionSourceChanged, this, [this]() {
        auto *source = GPSManager::instance()->gpsPositionSource();
        setExternalGPSSource(source);
    });

    (void) connect(&_nmeaPollTimer, &QTimer::timeout, this, &QGCPositionManager::_pollNmeaDevice);
    _nmeaPollTimer.start(kNmeaPollIntervalMs);

    auto *compass = QGCDeviceInfo::QGCCompass::instance();
    if (compass->init()) {
        (void) connect(compass, &QGCDeviceInfo::QGCCompass::positionUpdated,
                       this, &QGCPositionManager::_compassUpdated);
        qCDebug(QGCPositionManagerLog) << "Device compass connected for GCS heading";
    }
}

void QGCPositionManager::_setupPositionSources()
{
    _defaultSource = QGCCorePlugin::instance()->createPositionSource(this);
    if (_defaultSource) {
        _usingPluginSource = true;
    } else {
        qCDebug(QGCPositionManagerLog) << QGeoPositionInfoSource::availableSources();

        _defaultSource = QGeoPositionInfoSource::createDefaultSource(this);
        if (!_defaultSource) {
            qCWarning(QGCPositionManagerLog) << "No default source available";
            return;
        }
    }

    _setPositionSource(QGCPositionSource::InternalGPS);

    _createDefaultSatelliteSource();
}

void QGCPositionManager::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
{
    if (permissionStatus == Qt::PermissionStatus::Granted) {
        _locationPermissionDenied = false;
        emit locationPermissionDeniedChanged();
        _setupPositionSources();
    } else {
        qCWarning(QGCPositionManagerLog) << "Location Permission Denied — falling back to simulated position";
        _locationPermissionDenied = true;
        emit locationPermissionDeniedChanged();
        _simulatedSource = new SimulatedPosition(this);
        _setPositionSource(QGCPositionSource::Simulated);
    }
}

void QGCPositionManager::_checkPermission()
{
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);

    const Qt::PermissionStatus permissionStatus = QCoreApplication::instance()->checkPermission(locationPermission);
    if (permissionStatus == Qt::PermissionStatus::Undetermined) {
        QCoreApplication::instance()->requestPermission(locationPermission, this, [this](const QPermission &permission) {
            _handlePermissionStatus(permission.status());
        });
    } else {
        _handlePermissionStatus(permissionStatus);
    }
}

void QGCPositionManager::setExternalGPSSource(QGeoPositionInfoSource *source)
{
    if (_externalSource == source) {
        return;
    }

    if (_externalSource && _currentSource == _externalSource) {
        _externalSource->stopUpdates();
        (void) disconnect(_externalSource);
        _currentSource = nullptr;
    }

    // Disconnect previous external satellite source
    if (_externalSatSource) {
        (void) disconnect(_externalSatSource);
        _externalSatSource = nullptr;
        _destroySatelliteSource();
    }

    // We do NOT own the external source — it is owned by GPSRtk
    _externalSource = source;

    if (_externalSource) {
        qCDebug(QGCPositionManagerLog) << "External GPS position source available";

        auto *satSource = GPSManager::instance()->gpsSatelliteInfoSource();
        if (satSource) {
            _destroySatelliteSource();
            _externalSatSource = satSource;
            _connectSatelliteModel(satSource);
            satSource->startUpdates();
            qCDebug(QGCPositionManagerLog) << "External GPS satellite source connected";
        }
    } else {
        qCDebug(QGCPositionManagerLog) << "External GPS removed";
    }

    _selectBestSource();
}

// -- NMEA device management ------------------------------------------------

void QGCPositionManager::_closeSerialPort()
{
#ifndef QGC_NO_SERIAL_LINK
    if (_nmeaSerialPort) {
        _nmeaSerialPort->close();
        delete _nmeaSerialPort;
        _nmeaSerialPort = nullptr;
        _nmeaSerialPortName.clear();
        _nmeaSerialBaud = 0;
    }
#endif
}

void QGCPositionManager::_pollUdpNmeaDevice(GcsGpsSettings *settings)
{
    const uint16_t udpPort = settings->nmeaUdpPort()->rawValue().toUInt();
    if (_nmeaUdpSocket->localPort() != udpPort || _nmeaUdpSocket->state() != UdpIODevice::BoundState) {
        _openNmeaUdpPort(udpPort);
    }
    _closeSerialPort();
}

#ifndef QGC_NO_SERIAL_LINK
void QGCPositionManager::_pollSerialNmeaDevice(GcsGpsSettings *settings, const QString &portSetting)
{
    _nmeaUdpSocket->close();

    const uint32_t baud = settings->autoConnectNmeaBaud()->cookedValue().toUInt();

    // Already open on the right port — just check baud change
    if (_nmeaSerialPort && _nmeaSerialPortName == portSetting) {
        if (_nmeaSerialBaud != baud) {
            _nmeaSerialBaud = baud;
            _nmeaSerialPort->setBaudRate(static_cast<qint32>(baud));
            qCDebug(QGCPositionManagerLog) << "NMEA baud changed:" << baud;
        }
        return;
    }

    // Check if the configured port is physically present
    bool portFound = false;
    const auto ports = QGCSerialPortInfo::availablePorts();
    for (const QGCSerialPortInfo &port : ports) {
        if (port.systemLocation() == portSetting) {
            // Skip if port is actively used by GPSManager (RTK base station)
            if (GPSManager::instance()->isDeviceRegistered(portSetting)) {
                qCDebug(QGCPositionManagerLog) << "Skipping NMEA open — port in use by RTK GPS:" << portSetting;
                return;
            }
            portFound = true;
            break;
        }
    }

    if (portFound) {
        _openNmeaSerialPort(portSetting, baud);
    } else if (_nmeaSerialPort && _nmeaSerialPortName == portSetting) {
        // Port disappeared — close it
        qCDebug(QGCPositionManagerLog) << "NMEA port disappeared:" << portSetting;
        _closeNmeaDevice();
    }
}
#endif

void QGCPositionManager::_pollNmeaDevice()
{
    GcsGpsSettings *settings = SettingsManager::instance()->gcsGpsSettings();

    // Only engage NMEA when the operator has selected it as the source type.
    // Enum: 0=None, 1=NMEA, 2=RTK.
    const bool nmeaSelected = settings->positionSourceType()->rawValue().toInt() == 1;
    const QString portSetting = nmeaSelected ? settings->autoConnectNmeaPort()->cookedValueString()
                                             : QStringLiteral("Disabled");

    static constexpr QLatin1StringView kDisabled("Disabled");
    static constexpr QLatin1StringView kUdpPort("UDP Port");

    if (portSetting.isEmpty() || portSetting == kDisabled) {
        if (_nmeaSerialPort || _nmeaUdpSocket->isOpen() || _nmeaSource) {
            _closeNmeaDevice();
        }
        return;
    }

    if (portSetting == kUdpPort) {
        _pollUdpNmeaDevice(settings);
        return;
    }

#ifndef QGC_NO_SERIAL_LINK
    _pollSerialNmeaDevice(settings, portSetting);
#endif
}

void QGCPositionManager::_openNmeaSerialPort(const QString &portName, uint32_t baud)
{
#ifndef QGC_NO_SERIAL_LINK
    _closeNmeaDevice();

    auto *port = new QSerialPort(portName, this);
    port->setBaudRate(static_cast<qint32>(baud));

    if (!port->open(QIODevice::ReadOnly)) {
        const QString err = port->errorString();
        if (_nmeaLastOpenFailPort != portName || _nmeaLastOpenFailError != err) {
            qCWarning(QGCPositionManagerLog) << "Failed to open NMEA port" << portName << err;
            _nmeaLastOpenFailPort = portName;
            _nmeaLastOpenFailError = err;
        } else {
            qCDebug(QGCPositionManagerLog) << "NMEA port" << portName << "still failing:" << err;
        }
        delete port;
        return;
    }
    _nmeaLastOpenFailPort.clear();
    _nmeaLastOpenFailError.clear();

    _nmeaSerialPort = port;
    _nmeaSerialPortName = portName;
    _nmeaSerialBaud = baud;

    qCDebug(QGCPositionManagerLog) << "NMEA serial opened:" << portName << "baud:" << baud;
    _setNmeaSource(port);
#else
    Q_UNUSED(portName)
    Q_UNUSED(baud)
#endif
}

void QGCPositionManager::_openNmeaUdpPort(uint16_t port)
{
    _closeNmeaDevice();

    if (!_nmeaUdpSocket->bind(QHostAddress::AnyIPv4, port)) {
        qCWarning(QGCPositionManagerLog) << "Failed to bind NMEA UDP port" << port << _nmeaUdpSocket->errorString();
        return;
    }
    _nmeaUdpSocket->open(QIODevice::ReadOnly);

    qCDebug(QGCPositionManagerLog) << "NMEA UDP bound to port" << port;
    _setNmeaSource(_nmeaUdpSocket);
}

void QGCPositionManager::_closeNmeaDevice()
{
    _destroySatelliteSource();

    const bool wasCurrent = (_nmeaSource != nullptr) && (_currentSource == _nmeaSource);

    if (_nmeaSource) {
        _nmeaSource->stopUpdates();
        (void) disconnect(_nmeaSource);

        if (_currentSource == _nmeaSource) {
            _currentSource = nullptr;
        }

        delete _nmeaSource;
        _nmeaSource = nullptr;
    }

    delete _nmeaSplitter;
    _nmeaSplitter = nullptr;

    _closeSerialPort();

    _nmeaUdpSocket->close();

    // Configuration changed (port disabled / switched) — allow a fresh warning
    // if the next poll encounters a new open failure.
    _nmeaLastOpenFailPort.clear();
    _nmeaLastOpenFailError.clear();

    // If NMEA was the active position source, the last reported fix is now stale.
    // Clear published state so UI shows "no position" instead of a frozen value,
    // then fall back to the next best source (internal / simulated).
    if (wasCurrent) {
        _geoPositionInfo = QGeoPositionInfo();
        emit positionInfoUpdated(_geoPositionInfo);
        _setGCSPosition(QGeoCoordinate());
        _setGCSHeading(qQNaN());
        _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
        emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
        _stalenessTimer.stop();
        if (!_positionStale) {
            _positionStale = true;
            emit positionStaleChanged();
        }
        _selectBestSource();
    }
}

void QGCPositionManager::_setNmeaSource(QIODevice *device)
{
    if (_nmeaSource) {
        _nmeaSource->stopUpdates();
        (void) disconnect(_nmeaSource);
        if (_currentSource == _nmeaSource) {
            _currentSource = nullptr;
        }
        delete _nmeaSource;
        _nmeaSource = nullptr;
    }

    _destroySatelliteSource();

    // Split the NMEA stream so both position and satellite parsers get the data
    _nmeaSplitter = new NmeaStreamSplitter(device, this);

    _nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
    _nmeaSource->setDevice(_nmeaSplitter->positionPipe());
    _nmeaSource->setUserEquivalentRangeError(5.1);

    _createSatelliteSource(_nmeaSplitter->satellitePipe());

    qCDebug(QGCPositionManagerLog) << "NMEA source created with satellite splitter, device open:" << device->isOpen();

    _selectBestSource();
}

// -- Source selection -------------------------------------------------------

void QGCPositionManager::_selectBestSource()
{
    // Priority: External > NMEA > Internal > Simulated.
    //
    // RTK position is generally the most accurate source when present, so it
    // wins by default. NMEA may still be open in parallel (it is allowed to
    // coexist) so it can take over immediately if the external source goes
    // away, without waiting for the next NMEA poll tick.
    if (_externalSource) {
        _setPositionSource(ExternalGPS);
    } else if (_nmeaSource) {
        _setPositionSource(NmeaGPS);
    } else if (_defaultSource) {
        _setPositionSource(InternalGPS);
    } else if (_simulatedSource) {
        _setPositionSource(Simulated);
    }
}

void QGCPositionManager::_connectSatelliteModel(QGeoSatelliteInfoSource *source)
{
    // Cache in-view/in-use lists and forward to unified SatelliteModel
    (void) connect(source, &QGeoSatelliteInfoSource::satellitesInViewUpdated,
                   this, [this](const QList<QGeoSatelliteInfo> &sats) {
        _cachedInView = sats;
        _gcsSatelliteModel->updateFromQtPositioning(_cachedInView, _cachedInUse);
        emit gcsSatellitesInViewChanged();
    });
    (void) connect(source, &QGeoSatelliteInfoSource::satellitesInUseUpdated,
                   this, [this](const QList<QGeoSatelliteInfo> &sats) {
        _cachedInUse = sats;
        _gcsSatelliteModel->updateFromQtPositioning(_cachedInView, _cachedInUse);
        emit gcsSatellitesInUseChanged();
    });
}

void QGCPositionManager::_checkStaleness()
{
    const bool stale = !_lastPositionTime.isValid() || _lastPositionTime.elapsed() > kPositionStaleTimeoutMs;
    if (stale != _positionStale) {
        _positionStale = stale;
        emit positionStaleChanged();
        if (stale) {
            qCDebug(QGCPositionManagerLog) << "Position data is stale";
        }
    }
}

// -- Position processing ---------------------------------------------------

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo &update)
{
    _geoPositionInfo = update;
    _gcsPositioningError = QGeoPositionInfoSource::NoError;

    _lastPositionTime.restart();
    if (_positionStale) {
        _positionStale = false;
        emit positionStaleChanged();
    }
    _stalenessTimer.start();

    const QGeoCoordinate coord = update.coordinate();
    QGeoCoordinate newGCSPosition(_gcsPosition);

    const bool hasValidCoord = (qAbs(coord.latitude()) > 0.001)
                            && (qAbs(coord.longitude()) > 0.001);

    if (hasValidCoord) {
        if (update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
            _gcsPositionHorizontalAccuracy = update.attribute(QGeoPositionInfo::HorizontalAccuracy);
            emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);

            if (_gcsPositionHorizontalAccuracy <= kMinHorizontalAccuracyMeters) {
                newGCSPosition.setLatitude(coord.latitude());
                newGCSPosition.setLongitude(coord.longitude());
            }
        } else {
            qCDebug(QGCPositionManagerLog) << "Position accepted without accuracy attribute";
            newGCSPosition.setLatitude(coord.latitude());
            newGCSPosition.setLongitude(coord.longitude());
        }
    }

    if (update.hasAttribute(QGeoPositionInfo::VerticalAccuracy)) {
        _gcsPositionVerticalAccuracy = update.attribute(QGeoPositionInfo::VerticalAccuracy);
        emit gcsPositionVerticalAccuracyChanged();
        if (_gcsPositionVerticalAccuracy <= kMinVerticalAccuracyMeters) {
            newGCSPosition.setAltitude(coord.altitude());
        }
    } else if (hasValidCoord && !qIsNaN(coord.altitude())) {
        newGCSPosition.setAltitude(coord.altitude());
    }

    const qreal prevAccuracy = _gcsPositionAccuracy;
    _gcsPositionAccuracy = std::hypot(_gcsPositionHorizontalAccuracy, _gcsPositionVerticalAccuracy);
    if (_gcsPositionAccuracy != prevAccuracy) {
        emit gcsPositionAccuracyChanged();
    }

    _setGCSPosition(newGCSPosition);

    if (update.hasAttribute(QGeoPositionInfo::Direction)) {
        if (update.hasAttribute(QGeoPositionInfo::DirectionAccuracy)) {
            _gcsDirectionAccuracy = update.attribute(QGeoPositionInfo::DirectionAccuracy);
            emit gcsDirectionAccuracyChanged();
            if (_gcsDirectionAccuracy <= kMinDirectionAccuracyDegrees) {
                _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
            }
        } else {
            _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
        }
    }

    emit positionInfoUpdated(update);
    emit positionHealthChanged();
    qCDebug(QGCPositionManagerLog) << "positionHealth:"
                                   << "src=" << static_cast<int>(positionHealth().source())
                                   << "fix=" << static_cast<int>(positionHealth().fixType())
                                   << "acc=" << positionHealth().accuracyMeters() << "m"
                                   << "age=" << positionHealth().ageMs() << "ms";
}

PositionHealth QGCPositionManager::positionHealth() const
{
    PositionHealth::Source src = PositionHealth::Source::Unknown;
    switch (_positionSource) {
        case Simulated:   src = PositionHealth::Source::Simulated;   break;
        case InternalGPS: src = PositionHealth::Source::InternalGPS; break;
        case NmeaGPS:     src = PositionHealth::Source::NmeaGPS;     break;
        case ExternalGPS: src = PositionHealth::Source::ExternalGPS; break;
    }

    const bool valid = _geoPositionInfo.isValid();
    PositionHealth::FixType fix = PositionHealth::FixType::NoFix;
    if (valid) {
        fix = _geoPositionInfo.coordinate().type() == QGeoCoordinate::Coordinate3D
                  ? PositionHealth::FixType::Fix3D
                  : PositionHealth::FixType::Fix2D;
    }

    const qint64 age = _lastPositionTime.isValid() ? _lastPositionTime.elapsed() : -1;
    return PositionHealth(src, fix, _gcsPositionHorizontalAccuracy, age, valid && !_positionStale);
}

void QGCPositionManager::_positionError(QGeoPositionInfoSource::Error gcsPositioningError)
{
    _gcsPositioningError = gcsPositioningError;

    if (gcsPositioningError == QGeoPositionInfoSource::UpdateTimeoutError) {
        qCDebug(QGCPositionManagerLog) << "Position update timeout";
    } else {
        qCWarning(QGCPositionManagerLog) << "Positioning error:" << gcsPositioningError;
    }
}

void QGCPositionManager::_compassUpdated(const QGeoPositionInfo &update)
{
    if (!update.hasAttribute(QGeoPositionInfo::Direction)) {
        return;
    }

    // GPS course-over-ground takes priority when the device is moving.
    // Only use compass heading if the last GPS update didn't provide direction,
    // or if position data is stale (device stationary / no GPS fix).
    const bool gpsHasDirection = _geoPositionInfo.isValid()
                              && _geoPositionInfo.hasAttribute(QGeoPositionInfo::Direction)
                              && !_positionStale;
    if (gpsHasDirection) {
        return;
    }

    if (update.hasAttribute(QGeoPositionInfo::DirectionAccuracy)) {
        const qreal accuracy = update.attribute(QGeoPositionInfo::DirectionAccuracy);
        if (accuracy > kMinDirectionAccuracyDegrees) {
            return;
        }
        _gcsDirectionAccuracy = accuracy;
        emit gcsDirectionAccuracyChanged();
    }

    _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
}

void QGCPositionManager::_setGCSHeading(qreal newGCSHeading)
{
    if (newGCSHeading != _gcsHeading) {
        _gcsHeading = newGCSHeading;
        emit gcsHeadingChanged(_gcsHeading);
    }
}

void QGCPositionManager::_setGCSPosition(const QGeoCoordinate& newGCSPosition)
{
    if (newGCSPosition != _gcsPosition) {
        _gcsPosition = newGCSPosition;
        emit gcsPositionChanged(_gcsPosition);
    }
}

int QGCPositionManager::gcsSatellitesInView() const
{
    return _gcsSatelliteModel ? _gcsSatelliteModel->count() : 0;
}

int QGCPositionManager::gcsSatellitesInUse() const
{
    return _gcsSatelliteModel ? _gcsSatelliteModel->usedCount() : 0;
}

void QGCPositionManager::_createSatelliteSource(QIODevice *nmeaDevice)
{
    _destroySatelliteSource();

    auto *satSource = new QNmeaSatelliteInfoSource(QNmeaSatelliteInfoSource::UpdateMode::RealTimeMode, this);
    satSource->setDevice(nmeaDevice);
    _satelliteSource = satSource;

    _connectSatelliteModel(_satelliteSource);
    _satelliteSource->startUpdates();
    qCDebug(QGCPositionManagerLog) << "NMEA satellite source started";
}

void QGCPositionManager::_createDefaultSatelliteSource()
{
    _destroySatelliteSource();

    _satelliteSource = QGeoSatelliteInfoSource::createDefaultSource(this);
    if (!_satelliteSource) {
        qCDebug(QGCPositionManagerLog) << "No default satellite info source available";
        return;
    }

    _connectSatelliteModel(_satelliteSource);
    _satelliteSource->startUpdates();
    qCDebug(QGCPositionManagerLog) << "Default satellite source started:" << _satelliteSource->sourceName();
}

void QGCPositionManager::_destroySatelliteSource()
{
    if (_satelliteSource) {
        _satelliteSource->stopUpdates();
        (void) disconnect(_satelliteSource);
        delete _satelliteSource;
        _satelliteSource = nullptr;
    }

    // Lambda connections from _connectSatelliteModel are disconnected
    // via disconnect(_satelliteSource) above — no additional cleanup needed.
}

void QGCPositionManager::_setPositionSource(QGCPositionSource source)
{
    if (_currentSource) {
        _currentSource->stopUpdates();
        (void) disconnect(_currentSource);

        _geoPositionInfo = QGeoPositionInfo();
        emit positionInfoUpdated(_geoPositionInfo);

        _setGCSPosition(QGeoCoordinate());

        _setGCSHeading(qQNaN());

        _gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();
        emit gcsPositionHorizontalAccuracyChanged(_gcsPositionHorizontalAccuracy);
    }

    static constexpr const char *kSourceNames[] = {"Simulated", "InternalGPS", "NmeaGPS", "ExternalGPS"};

    switch (source) {
    case QGCPositionManager::Simulated:
        _currentSource = _simulatedSource;
        break;
    case QGCPositionManager::NmeaGPS:
        _currentSource = _nmeaSource;
        break;
    case QGCPositionManager::InternalGPS:
        _currentSource = _defaultSource;
        break;
    case QGCPositionManager::ExternalGPS:
        _currentSource = _externalSource;
        break;
    }

    static constexpr const char *kUserVisibleNames[] = {
        QT_TRANSLATE_NOOP("QGCPositionManager", "Simulated"),
        QT_TRANSLATE_NOOP("QGCPositionManager", "Internal GPS"),
        QT_TRANSLATE_NOOP("QGCPositionManager", "NMEA GPS"),
        QT_TRANSLATE_NOOP("QGCPositionManager", "RTK GPS"),
    };

    const QGCPositionSource oldSource = _positionSource;
    _positionSource = source;
    if ((oldSource == NmeaGPS) != (source == NmeaGPS)) {
        emit usingNmeaSourceChanged();
    }

    const QString oldDescription = _sourceDescription;
    _sourceDescription = tr(kUserVisibleNames[source]);
    qCDebug(QGCPositionManagerLog) << "Position source set to:" << kSourceNames[source];

    if (oldDescription != _sourceDescription) {
        emit sourceDescriptionChanged();

        if (!oldDescription.isEmpty()) {
            const QString msg = tr("Position source: %1").arg(_sourceDescription);
            GPSManager::instance()->logEvent(
                GPSEvent::info(GPSEvent::Source::GCS, msg));
            AudioOutput::instance()->say(msg);
        }
    }

    if (_currentSource) {
        _currentSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        _updateInterval = qMax(_currentSource->minimumUpdateInterval(), kMinUpdateIntervalMs);
        #if !defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)
            _currentSource->setUpdateInterval(_updateInterval);
        #endif

        (void) connect(_currentSource, &QGeoPositionInfoSource::positionUpdated, this, &QGCPositionManager::_positionUpdated);
        (void) connect(_currentSource, &QGeoPositionInfoSource::errorOccurred, this, &QGCPositionManager::_positionError);

        _currentSource->startUpdates();
    }
}
