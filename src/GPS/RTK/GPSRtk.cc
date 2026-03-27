#include "GPSRtk.h"
#include "GPSPositionSource.h"
#include "GPSProvider.h"
#include "GPSSatelliteInfoSource.h"
#include "GPSRTKFactGroup.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"
#include "SatelliteModel.h"
#include "GcsGpsSettings.h"
#include "SettingsManager.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QtGlobal>

QGC_LOGGING_CATEGORY(GPSRtkLog, "GPS.GPSRtk")

namespace {

GPSProvider::rtk_data_s rtkDataFromSettings(GcsGpsSettings &settings)
{
    GPSProvider::rtk_data_s d{};
    d.surveyInAccMeters      = settings.surveyInAccuracyLimit()->rawValue().toDouble();
    d.surveyInDurationSecs   = settings.surveyInMinObservationDuration()->rawValue().toInt();
    d.useFixedBaseLocation   = static_cast<GPSProvider::rtk_data_s::BaseMode>(settings.useFixedBasePosition()->rawValue().toInt());
    d.fixedBaseLatitude      = settings.fixedBasePositionLatitude()->rawValue().toDouble();
    d.fixedBaseLongitude     = settings.fixedBasePositionLongitude()->rawValue().toDouble();
    d.fixedBaseAltitudeMeters = settings.fixedBasePositionAltitude()->rawValue().toFloat();
    d.fixedBaseAccuracyMeters = settings.fixedBasePositionAccuracy()->rawValue().toFloat();
    d.outputMode             = static_cast<uint8_t>(qBound(0u, settings.gpsOutputMode()->rawValue().toUInt(), 255u));
    d.ubxMode                = static_cast<uint8_t>(qBound(0u, settings.ubxMode()->rawValue().toUInt(), 255u));
    d.ubxDynamicModel        = static_cast<uint8_t>(qBound(0u, settings.ubxDynamicModel()->rawValue().toUInt(), 255u));
    d.ubxUart2Baudrate       = settings.ubxUart2Baudrate()->rawValue().toInt();
    d.ubxPpkOutput           = settings.ubxPpkOutput()->rawValue().toBool();
    d.ubxDgnssTimeout        = static_cast<uint16_t>(qBound(0u, settings.ubxDgnssTimeout()->rawValue().toUInt(), 65535u));
    d.ubxMinCno              = static_cast<uint8_t>(qBound(0u, settings.ubxMinCno()->rawValue().toUInt(), 255u));
    d.ubxMinElevation        = static_cast<uint8_t>(qBound(0u, settings.ubxMinElevation()->rawValue().toUInt(), 255u));
    d.ubxOutputRate          = static_cast<uint16_t>(qBound(0u, settings.ubxOutputRate()->rawValue().toUInt(), 65535u));
    d.ubxJamDetSensitivityHi = settings.ubxJamDetSensitivityHi()->rawValue().toBool();
    d.gnssSystems            = settings.gnssSystems()->rawValue().toInt();
    d.headingOffset          = settings.headingOffset()->rawValue().toFloat();
    return d;
}

} // namespace

GPSRtk::GPSRtk(QObject *parent)
    : QObject(parent)
    , _gpsRtkFactGroup(new GPSRTKFactGroup(this))
    , _satelliteModel(new SatelliteModel(this))
    , _positionSource(new GPSPositionSource(this))
    , _satelliteInfoSource(new GPSSatelliteInfoSource(this))
{
    qCDebug(GPSRtkLog) << this;
}

GPSRtk::~GPSRtk()
{
    // Destructor must guarantee the worker thread is gone before members are destroyed,
    // so use the blocking variant. The async disconnectGPS() is only for GUI-initiated
    // teardown where responsiveness matters.
    _teardownThread(/*waitForFinish=*/true);
    _resetState();

    qCDebug(GPSRtkLog) << this;
}

void GPSRtk::_resetState()
{
    _gpsRtkFactGroup->resetToDefaults();
    _satelliteModel->clear();
}

void GPSRtk::connectGPS(const QString &device, QStringView gps_type)
{
#ifndef QGC_NO_SERIAL_LINK
    auto *transport = new SerialGPSTransport(device);
    _devicePath = device;
    connectGPS(transport, gps_type);
#else
    Q_UNUSED(device)
    Q_UNUSED(gps_type)
    qCWarning(GPSRtkLog) << "Serial link support disabled, cannot connect GPS by device path";
#endif
}

void GPSRtk::connectGPS(GPSTransport *transport, QStringView gps_type)
{
    GcsGpsSettings* rtkSettings = SettingsManager::instance()->gcsGpsSettings();

    const GPSType type = gpsTypeFromString(gps_type);
    qCDebug(GPSRtkLog) << "Connecting" << gpsTypeDisplayName(type) << "device";

    // Reconnect path must wait for any previous thread to fully exit before
    // we spin up a new one — otherwise two provider threads could briefly coexist
    // touching the same transport.
    _teardownThread(/*waitForFinish=*/true);
    _resetState();
    emit connectedChanged();

    _deviceType = QString::fromUtf8(gpsTypeDisplayName(type));
    emit deviceTypeChanged();

    const GPSProvider::rtk_data_s rtkData = rtkDataFromSettings(*rtkSettings);

    _gpsThread = new QThread(this);
    _gpsProvider = new GPSProvider(transport, type, rtkData);
    transport->setParent(_gpsProvider);
    _gpsProvider->moveToThread(_gpsThread);

    (void) connect(_gpsThread.data(), &QThread::started, _gpsProvider.data(), &GPSProvider::process);
    (void) connect(_gpsProvider.data(), &GPSProvider::finished, _gpsThread.data(), &QThread::quit, Qt::QueuedConnection);
    // Auto-cleanup (non-blocking teardown path only): thread finished → delete provider → delete thread.
    // The blocking path in _teardownThread(true) disconnects these before deleting manually to avoid
    // racing the pending DeferredDelete events against the direct delete.
    (void) connect(_gpsThread.data(), &QThread::finished, _gpsProvider.data(), &QObject::deleteLater);
    (void) connect(_gpsThread.data(), &QThread::finished, _gpsThread.data(), &QObject::deleteLater);

    _connectProviderSignals();

    _gpsThread->start();
}

void GPSRtk::_connectProviderSignals()
{
    if (_rtcmMavlink) {
        (void) connect(_gpsProvider.data(), &GPSProvider::RTCMDataUpdate, _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate, Qt::QueuedConnection);
    }

    (void) connect(_gpsProvider.data(), &GPSProvider::connectionEstablished, this, [this]() {
        _gpsRtkFactGroup->connected()->setRawValue(true);
        emit connectedChanged();
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider.data(), &GPSProvider::satelliteInfoUpdate, this, [this](const satellite_info_s &msg) {
        qCDebug(GPSRtkLog) << Q_FUNC_INFO << msg.count << "satellites";
        _gpsRtkFactGroup->numSatellites()->setRawValue(msg.count);
        _satelliteModel->updateFromDriverInfo(msg);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider.data(), &GPSProvider::satelliteInfoUpdate,
                   _satelliteInfoSource, &GPSSatelliteInfoSource::updateFromSatelliteInfo,
                   Qt::QueuedConnection);

    (void) connect(_gpsProvider.data(), &GPSProvider::sensorGpsUpdate, this, [this](const sensor_gps_s &msg) {
        qCDebug(GPSRtkLog) << "Base GPS: alt=" << msg.altitude_msl_m
            << "lon=" << msg.longitude_deg << "lat=" << msg.latitude_deg << "fix=" << msg.fix_type;
        _gpsRtkFactGroup->baseLatitude()->setRawValue(msg.latitude_deg);
        _gpsRtkFactGroup->baseLongitude()->setRawValue(msg.longitude_deg);
        _gpsRtkFactGroup->baseAltitude()->setRawValue(msg.altitude_msl_m);
        _gpsRtkFactGroup->baseFixType()->setRawValue(msg.fix_type);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider.data(), &GPSProvider::sensorGpsUpdate,
                   _positionSource, &GPSPositionSource::updateFromSensorGps,
                   Qt::QueuedConnection);

    (void) connect(_gpsProvider.data(), &GPSProvider::sensorGnssRelativeUpdate, this, [this](const sensor_gnss_relative_s &msg) {
        qCDebug(GPSRtkLog) << "GNSS relative: heading=" << msg.heading
            << "baseline=" << msg.position_length << "m fixed=" << msg.carrier_solution_fixed;
        _gpsRtkFactGroup->heading()->setRawValue(static_cast<double>(msg.heading));
        _gpsRtkFactGroup->headingValid()->setRawValue(msg.heading_valid);
        _gpsRtkFactGroup->baselineLength()->setRawValue(static_cast<double>(msg.position_length));
        _gpsRtkFactGroup->carrierFixed()->setRawValue(msg.carrier_solution_fixed);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider.data(), &GPSProvider::surveyInStatus, this, [this](const GPSProvider::SurveyInStatusData &status) {
        _gpsRtkFactGroup->currentDuration()->setRawValue(status.duration);
        _gpsRtkFactGroup->currentAccuracy()->setRawValue(static_cast<double>(status.accuracyMM) / 1000.0);
        _gpsRtkFactGroup->currentLatitude()->setRawValue(status.latitude);
        _gpsRtkFactGroup->currentLongitude()->setRawValue(status.longitude);
        _gpsRtkFactGroup->currentAltitude()->setRawValue(status.altitude);
        _gpsRtkFactGroup->valid()->setRawValue(status.valid);
        _gpsRtkFactGroup->active()->setRawValue(status.active);

        // Feed base station position into the position source for GCS location
        if (status.latitude != 0. && status.longitude != 0.) {
            sensor_gps_s gps{};
            gps.latitude_deg = status.latitude;
            gps.longitude_deg = status.longitude;
            gps.altitude_msl_m = status.altitude;
            gps.fix_type = status.valid ? sensor_gps_s::FIX_TYPE_3D : sensor_gps_s::FIX_TYPE_2D;
            gps.eph = static_cast<float>(status.accuracyMM / 1000.0);
            _positionSource->updateFromSensorGps(gps);
        }
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider.data(), &GPSProvider::error, this, [this](const QString &msg) {
        qCWarning(GPSRtkLog) << "GPS provider error:" << msg;
        _lastError = msg;
        emit lastErrorChanged();
        emit errorOccurred(msg);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider.data(), &GPSProvider::finished, this, [this]() {
        _resetState();
        emit connectedChanged();
    }, Qt::QueuedConnection);
}

void GPSRtk::disconnectGPS()
{
    // Kick off teardown but do NOT block the GUI — the QThread::finished signal
    // triggers deleteLater on both provider and thread; our QPointer members
    // auto-null once those objects are destroyed.
    _teardownThread(/*waitForFinish=*/false);

    _resetState();
    emit connectedChanged();
}

void GPSRtk::_teardownThread(bool waitForFinish)
{
    if (!_gpsThread) {
        return;
    }

    if (_gpsProvider) {
        _gpsProvider->requestStop();
        _gpsProvider->wakeFromReconnectWait();
        // Break all outgoing signal connections immediately so queued signals
        // stop mutating GUI-thread state; deleteLater will free the object.
        _gpsProvider->disconnect();
    }

    _gpsThread->requestInterruption();
    _gpsThread->quit();

    if (waitForFinish) {
        if (!_gpsThread->wait(kGPSThreadShutdownTimeoutMs)) {
            qCCritical(GPSRtkLog) << "GPS thread still running after"
                                  << kGPSThreadShutdownTimeoutMs << "ms — abandoning (leak)";
        }
        // Blocking path: the worker thread has fully exited, and QThread::finished
        // has already been emitted — meaning the two finished→deleteLater connections
        // (set up in connectGPS) have posted DeferredDelete events for both provider
        // and thread. We delete both manually here to avoid:
        //   1. QThread destroyed-while-still-running warnings from ~GPSRtk racing
        //      the deleteLater chain on a child QThread;
        //   2. Leaking _gpsProvider, whose DeferredDelete was posted to the now-dead
        //      worker thread's event queue and will never dispatch.
        // Sever the finished→deleteLater connections first so no stale delivery
        // fires on freed memory, then move the provider's affinity back to this
        // thread before deleting it (Qt requires objects be destroyed on their
        // owning thread).
        if (_gpsThread) {
            QObject::disconnect(_gpsThread.data(), &QThread::finished, nullptr, nullptr);
        }
        if (_gpsProvider) {
            _gpsProvider->moveToThread(QThread::currentThread());
            delete _gpsProvider.data();
        }
        delete _gpsThread.data();
    } else {
        // Non-blocking path: thread is still winding down. Detach from the
        // parent so ~GPSRtk does NOT delete it as a child — the deleteLater
        // wired to QThread::finished will handle destruction once run() returns.
        _gpsThread->setParent(nullptr);
    }

    // Drop our references. QPointer auto-nulls when the underlying object dies.
    _gpsProvider = nullptr;
    _gpsThread = nullptr;
}

void GPSRtk::injectRTCMData(const QByteArray &data)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (_gpsProvider) {
        QMetaObject::invokeMethod(_gpsProvider.data(), &GPSProvider::injectRTCMData,
                                  Qt::QueuedConnection, data);
    }
}

bool GPSRtk::connected() const
{
    return _gpsRtkFactGroup->connected()->rawValue().toBool();
}

FactGroup *GPSRtk::gpsRtkFactGroup()
{
    return _gpsRtkFactGroup;
}
