/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "DeviceInfo.h"
#include <QGCLoggingCategory.h>

#include <QtCore/qapplicationstatic.h>
#include <QtNetwork/QNetworkInformation>
#ifdef QGC_ENABLE_BLUETOOTH
#    include <QtBluetooth/QBluetoothLocalDevice>
#endif

QGC_LOGGING_CATEGORY(QGCDeviceInfoLog, "qgc.utilities.deviceinfo")

namespace QGCDeviceInfo
{

//  TODO:
//    - reachabilityChanged()
//    - Allow to select by transportMedium()

bool isInternetAvailable()
{
    if(QNetworkInformation::availableBackends().isEmpty()) return false;

    if(!QNetworkInformation::loadDefaultBackend()) return false;

    // Note: Qt6.7 will do this automatically
    if(!QNetworkInformation::instance()->supports(QNetworkInformation::Feature::Reachability))
    {
        if(!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) return false;
    }

    const QNetworkInformation::Reachability reachability = QNetworkInformation::instance()->reachability();

    return (reachability == QNetworkInformation::Reachability::Online);
}

bool isNetworkWired()
{
    if(QNetworkInformation::availableBackends().isEmpty()) return false;

    if(!QNetworkInformation::loadDefaultBackend()) return false;

    return (QNetworkInformation::instance()->transportMedium() == QNetworkInformation::TransportMedium::Ethernet);
}

bool isBluetoothAvailable()
{
    #ifdef QGC_ENABLE_BLUETOOTH
        const QList<QBluetoothHostInfo> devices = QBluetoothLocalDevice::allDevices();
        return !devices.isEmpty();
    #else
        return false;
    #endif
}

////////////////////////////////////////////////////////////////////

Q_APPLICATION_STATIC(QGCAmbientTemperature, s_ambientTemperature);

QGCAmbientTemperature* QGCAmbientTemperature::instance()
{
    return s_ambientTemperature();
}

QGCAmbientTemperature::QGCAmbientTemperature(QObject* parent)
    : QObject(parent)
    , _ambientTemperature(new QAmbientTemperatureSensor(this))
    , _ambientTemperatureFilter(std::make_shared<QGCAmbientTemperatureFilter>())
{
    connect(_ambientTemperature, &QAmbientTemperatureSensor::sensorError, this, [](int error) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "QAmbientTemperature error:" << error;
    });

    if (!init()) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Error Initializing Ambient Temperature Sensor";
    }

    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

QGCAmbientTemperature::~QGCAmbientTemperature()
{
    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

bool QGCAmbientTemperature::init()
{
    _ambientTemperature->addFilter(_ambientTemperatureFilter.get());

    const bool connected = _ambientTemperature->connectToBackend();
    if(!connected) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Failed to connect to ambient temperature backend";
        return false;
    } else {
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Connected to ambient temperature backend:" << _ambientTemperature->identifier();
    }

    if (_ambientTemperature->isFeatureSupported(QSensor::SkipDuplicates)) {
        _ambientTemperature->setSkipDuplicates(true);
    }

    const qrangelist dataRates = _ambientTemperature->availableDataRates();
    if (!dataRates.isEmpty()) {
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Available Data Rates:" << dataRates;
        // _ambientTemperature->setDataRate(dataRates.first().first);
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Selected Data Rate:" << _ambientTemperature->dataRate();
    }

    const qoutputrangelist outputRanges = _ambientTemperature->outputRanges();
    if (!outputRanges.isEmpty()) {
        // qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Output Ranges:" << outputRanges;
        // _ambientTemperature->setOutputRange(outputRanges.first().first);
        const int outputRangeIndex = _ambientTemperature->outputRange();
        if (outputRangeIndex < outputRanges.size()) {
            const qoutputrange outputRange = outputRanges.at(_ambientTemperature->outputRange());
            qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Selected Output Range:" << outputRange.minimum << outputRange.maximum << outputRange.accuracy;
        }
    }

    _readingChangedConnection = connect(_ambientTemperature, &QAmbientTemperatureSensor::readingChanged, this, [this]() {
        QAmbientTemperatureReading* reading = _ambientTemperature->reading();
        if (!reading) {
            return;
        }

        _temperatureC = reading->temperature();

        emit temperatureUpdated(_temperatureC);
    });

    // _ambientTemperature->setActive(true);
    const bool started = _ambientTemperature->start();
    if (!started) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Failed to start ambient temperature";
        return false;
    }

    return true;
}

void QGCAmbientTemperature::quit()
{
    // _ambientTemperature->setActive(false);
    _ambientTemperature->stop();
    _ambientTemperature->disconnect(_readingChangedConnection);
}

QGCAmbientTemperatureFilter::QGCAmbientTemperatureFilter()
    : QAmbientTemperatureFilter()
{
    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

QGCAmbientTemperatureFilter::~QGCAmbientTemperatureFilter()
{
    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

bool QGCAmbientTemperatureFilter::filter(QAmbientTemperatureReading *reading)
{
    if (!reading) {
        return false;
    }

    const qreal temperature = reading->temperature();
    return ((temperature >= s_minValidTemperatureC) && (temperature <= s_maxValidTemperatureC));
}

////////////////////////////////////////////////////////////////////

Q_APPLICATION_STATIC(QGCPressure, s_pressure);

QGCPressure* QGCPressure::instance()
{
    return s_pressure();
}

QGCPressure::QGCPressure(QObject* parent)
    : QObject(parent)
    , _pressure(new QPressureSensor(this))
    , _pressureFilter(std::make_shared<QGCPressureFilter>())
{
    connect(_pressure, &QPressureSensor::sensorError, this, [](int error) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "QPressure error:" << error;
    });

    if (!init()) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Error Initializing Pressure Sensor";
    }

    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

QGCPressure::~QGCPressure()
{
    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

bool QGCPressure::init()
{
    _pressure->addFilter(_pressureFilter.get());

    const bool connected = _pressure->connectToBackend();
    if(!connected) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Failed to connect to pressure backend";
        return false;
    } else {
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Connected to pressure backend:" << _pressure->identifier();
    }

    if (_pressure->isFeatureSupported(QSensor::SkipDuplicates)) {
        _pressure->setSkipDuplicates(true);
    }

    const qrangelist dataRates = _pressure->availableDataRates();
    if (!dataRates.isEmpty()) {
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Available Data Rates:" << dataRates;
        // _pressure->setDataRate(dataRates.first().first);
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Selected Data Rate:" << _pressure->dataRate();
    }

    const qoutputrangelist outputRanges = _pressure->outputRanges();
    if (!outputRanges.isEmpty()) {
        // qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Output Ranges:" << outputRanges;
        // _pressure->setOutputRange(outputRanges.first().first);
        const int outputRangeIndex = _pressure->outputRange();
        if (outputRangeIndex < outputRanges.size()) {
            const qoutputrange outputRange = outputRanges.at(_pressure->outputRange());
            qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Selected Output Range:" << outputRange.minimum << outputRange.maximum << outputRange.accuracy;
        }
    }

    _readingChangedConnection = connect(_pressure, &QPressureSensor::readingChanged, this, [this]() {
        QPressureReading* reading = _pressure->reading();
        if (!reading) {
            return;
        }

        _temperatureC = reading->temperature();
        _pressurePa = reading->pressure();
        emit pressureUpdated(_pressurePa, _temperatureC);
    });

    // _pressure->setActive(true);
    const bool started = _pressure->start();
    if (!started) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Failed to start pressure";
        return false;
    }

    return true;
}

void QGCPressure::quit()
{
    // _pressure->setActive(false);
    _pressure->stop();
    _pressure->disconnect(_readingChangedConnection);
}

QGCPressureFilter::QGCPressureFilter()
    : QPressureFilter()
{
    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

QGCPressureFilter::~QGCPressureFilter()
{
    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

bool QGCPressureFilter::filter(QPressureReading *reading)
{
    if (!reading) {
        return false;
    }

    const qreal temperature = reading->temperature();
    if ((temperature < s_minValidTemperatureC) || (temperature > s_maxValidTemperatureC)) {
        return false;
    }

    const qreal pressure = reading->pressure();
    if ((pressure < s_minValidPressurePa) || (pressure > s_maxValidPressurePa)) {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////

Q_APPLICATION_STATIC(QGCCompass, s_compass);

QGCCompass* QGCCompass::instance()
{
    return s_compass();
}

QGCCompass::QGCCompass(QObject* parent)
    : QObject(parent)
    , _compass(new QCompass(this))
    , _compassFilter(std::make_shared<QGCCompassFilter>())
{
    // qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;

    (void) connect(_compass, &QCompass::sensorError, this, [](int error) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Compass error:" << error;
    });

    if (!init()) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Error Initializing Compass Sensor";
    }
}

QGCCompass::~QGCCompass()
{
    // qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

bool QGCCompass::init()
{
    _compass->addFilter(_compassFilter.get());

    const bool connected = _compass->connectToBackend();
    if (!connected) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Failed to connect to compass backend";
        return false;
    } else {
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Connected to compass backend:" << _compass->identifier();
    }

    if (_compass->isFeatureSupported(QSensor::SkipDuplicates)) {
        _compass->setSkipDuplicates(true);
    }

    const qrangelist dataRates = _compass->availableDataRates();
    if (!dataRates.isEmpty()) {
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Available Data Rates:" << dataRates;
        // _compass->setDataRate(dataRates.first().first);
        qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Selected Data Rate:" << _compass->dataRate();
    }

    const qoutputrangelist outputRanges = _compass->outputRanges();
    if (!outputRanges.isEmpty()) {
        // qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Output Ranges:" << outputRanges;
        // _compass->setOutputRange(outputRanges.first().first);
        const int outputRangeIndex = _compass->outputRange();
        if (outputRangeIndex < outputRanges.size()) {
            const qoutputrange outputRange = outputRanges.at(_compass->outputRange());
            qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << "Selected Output Range:" << outputRange.minimum << outputRange.maximum << outputRange.accuracy;
        }
    }

    _readingChangedConnection = connect(_compass, &QCompass::readingChanged, this, [this]() {
        QCompassReading* const reading = _compass->reading();
        if (!reading) {
            return;
        }

        _calibrationLevel = reading->calibrationLevel();
        _azimuth = reading->azimuth();

        emit compassUpdated(_azimuth);

        QGeoPositionInfo update;
        update.setAttribute(QGeoPositionInfo::Attribute::Direction, _azimuth);
        update.setAttribute(QGeoPositionInfo::Attribute::DirectionAccuracy, _calibrationLevel);
        update.setTimestamp(QDateTime::currentDateTimeUtc());
        emit positionUpdated(update);
    });

    // _compass->setActive(true);
    const bool started = _compass->start();
    if (!started) {
        qCWarning(QGCDeviceInfoLog) << Q_FUNC_INFO << "Failed to start compass";
        return false;
    }

    return true;
}

void QGCCompass::quit()
{
    // _compass->setActive(false);
    _compass->stop();
    _compass->disconnect(_readingChangedConnection);
}

QGCCompassFilter::QGCCompassFilter()
    : QCompassFilter()
{
    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

QGCCompassFilter::~QGCCompassFilter()
{
    qCDebug(QGCDeviceInfoLog) << Q_FUNC_INFO << this;
}

bool QGCCompassFilter::filter(QCompassReading *reading)
{
    if (!reading) {
        return false;
    }

    const qreal calibration = reading->calibrationLevel();
    return (calibration >= s_minCompassCalibrationLevel);
}

////////////////////////////////////////////////////////////////////

} // namespace QGCDeviceInfo
