#include "GPSRtk.h"
#include "GPSPositionSource.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "QGCLoggingCategory.h"
#include "RTCMRouter.h"
#include "RTKSatelliteModel.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QtGlobal>

QGC_LOGGING_CATEGORY(GPSRtkLog, "GPS.GPSRtk")

GPSRtk::GPSRtk(QObject *parent)
    : QObject(parent)
    , _gpsRtkFactGroup(new GPSRTKFactGroup(this))
    , _satelliteModel(new RTKSatelliteModel(this))
    , _positionSource(new GPSPositionSource(this))
{
    qCDebug(GPSRtkLog) << this;
}

GPSRtk::~GPSRtk()
{
    disconnectGPS();

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
    RTKSettings* rtkSettings = SettingsManager::instance()->rtkSettings();

    int manufacturerId = 0;
    const GPSType type = gpsTypeFromString(gps_type, &manufacturerId);
    rtkSettings->baseReceiverManufacturers()->setRawValue(manufacturerId);
    qCDebug(GPSRtkLog) << "Connecting" << gpsTypeDisplayName(type) << "device";

    disconnectGPS();

    _deviceType = QString::fromUtf8(gpsTypeDisplayName(type));
    emit deviceTypeChanged();

    _requestGpsStop = false;
    const GPSProvider::rtk_data_s rtkData = GPSProvider::rtkDataFromSettings(*rtkSettings);

    _gpsThread = new QThread(this);
    _gpsProvider = new GPSProvider(transport, type, rtkData, _requestGpsStop);
    transport->setParent(_gpsProvider);
    _gpsProvider->moveToThread(_gpsThread);

    (void) connect(_gpsThread, &QThread::started, _gpsProvider, &GPSProvider::process);
    (void) connect(_gpsProvider, &GPSProvider::finished, _gpsThread, &QThread::quit, Qt::QueuedConnection);

    if (_rtcmRouter) {
        (void) connect(_gpsProvider, &GPSProvider::RTCMDataUpdate, _rtcmRouter, &RTCMRouter::routeToVehicles, Qt::QueuedConnection);
    }

    (void) connect(_gpsProvider, &GPSProvider::connectionEstablished, this, [this]() {
        _gpsRtkFactGroup->connected()->setRawValue(true);
        emit connectedChanged();
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider, &GPSProvider::satelliteInfoUpdate, this, [this](const satellite_info_s &msg) {
        qCDebug(GPSRtkLog) << Q_FUNC_INFO << QStringLiteral("%1 satellites").arg(msg.count);
        _gpsRtkFactGroup->numSatellites()->setRawValue(msg.count);
        _satelliteModel->update(msg);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider, &GPSProvider::sensorGpsUpdate, this, [this](const sensor_gps_s &msg) {
        qCDebug(GPSRtkLog) << QStringLiteral("Base GPS: alt=%1 lon=%2 lat=%3 fix=%4")
            .arg(msg.altitude_msl_m).arg(msg.longitude_deg).arg(msg.latitude_deg).arg(msg.fix_type);
        _gpsRtkFactGroup->baseLatitude()->setRawValue(msg.latitude_deg);
        _gpsRtkFactGroup->baseLongitude()->setRawValue(msg.longitude_deg);
        _gpsRtkFactGroup->baseAltitude()->setRawValue(msg.altitude_msl_m);
        _gpsRtkFactGroup->baseFixType()->setRawValue(msg.fix_type);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider, &GPSProvider::sensorGpsUpdate,
                   _positionSource, &GPSPositionSource::updateFromSensorGps,
                   Qt::QueuedConnection);

    (void) connect(_gpsProvider, &GPSProvider::sensorGnssRelativeUpdate, this, [this](const sensor_gnss_relative_s &msg) {
        qCDebug(GPSRtkLog) << QStringLiteral("GNSS relative: heading=%1 baseline=%2m fixed=%3")
            .arg(msg.heading).arg(msg.position_length).arg(msg.carrier_solution_fixed);
        _gpsRtkFactGroup->heading()->setRawValue(static_cast<double>(msg.heading));
        _gpsRtkFactGroup->headingValid()->setRawValue(msg.heading_valid);
        _gpsRtkFactGroup->baselineLength()->setRawValue(static_cast<double>(msg.position_length));
        _gpsRtkFactGroup->carrierFixed()->setRawValue(msg.carrier_solution_fixed);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider, &GPSProvider::surveyInStatus, this, [this](const GPSProvider::SurveyInStatusData &status) {
        _gpsRtkFactGroup->currentDuration()->setRawValue(status.duration);
        _gpsRtkFactGroup->currentAccuracy()->setRawValue(static_cast<double>(status.accuracyMM) / 1000.0);
        _gpsRtkFactGroup->currentLatitude()->setRawValue(status.latitude);
        _gpsRtkFactGroup->currentLongitude()->setRawValue(status.longitude);
        _gpsRtkFactGroup->currentAltitude()->setRawValue(status.altitude);
        _gpsRtkFactGroup->valid()->setRawValue(status.valid);
        _gpsRtkFactGroup->active()->setRawValue(status.active);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider, &GPSProvider::error, this, [this](const QString &msg) {
        qCWarning(GPSRtkLog) << "GPS provider error:" << msg;
        emit errorOccurred(msg);
    }, Qt::QueuedConnection);

    (void) connect(_gpsProvider, &GPSProvider::finished, this, [this]() {
        _resetState();
        emit connectedChanged();
    }, Qt::QueuedConnection);

    _gpsThread->start();
}

void GPSRtk::disconnectGPS()
{
    if (_gpsThread) {
        _requestGpsStop = true;

        if (_gpsProvider) {
            _gpsProvider->disconnect(this);
        }

        _gpsThread->requestInterruption();
        _gpsThread->quit();

        if (!_gpsThread->wait(kGPSThreadGracefulTimeout)) {
            qCWarning(GPSRtkLog) << "GPS thread did not exit within" << kGPSThreadGracefulTimeout << "ms, terminating";
            _gpsThread->terminate();
            if (!_gpsThread->wait(kGPSThreadTerminateTimeout)) {
                qCCritical(GPSRtkLog) << "GPS thread failed to terminate";
            }
        }

        delete _gpsProvider;
        _gpsProvider = nullptr;

        delete _gpsThread;
        _gpsThread = nullptr;
    }

    _resetState();
    emit connectedChanged();
}

void GPSRtk::injectRTCMData(const QByteArray &data)
{
    if (_gpsProvider) {
        _gpsProvider->injectRTCMData(data);
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
