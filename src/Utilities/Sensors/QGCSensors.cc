#include "QGCSensors.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QApplicationStatic>

QGC_LOGGING_CATEGORY(QGCSensorsLog, "Utilities.QGCSensors")

namespace QGCSensors
{

namespace {

constexpr qreal minValidTemperatureC = -40.;
constexpr qreal maxValidTemperatureC = 85.;

/// Puts @a filter on the readings of @a sensor, connects it to its backend and logs what the backend offers.
/// @a name is what the log calls the sensor.
bool connectSensor(QSensor* sensor, QSensorFilter* filter, const char* name)
{
    // QSensor lists a filter once for each addFilter(), and a filter that is destroyed takes only one of its
    // entries out: a second one would be left for the sensor to use
    if (!sensor->filters().contains(filter)) {
        sensor->addFilter(filter);
    }

    if (!sensor->connectToBackend()) {
        qCWarning(QGCSensorsLog) << "Failed to connect to" << name << "backend";
        return false;
    }
    qCDebug(QGCSensorsLog) << "Connected to" << name << "backend:" << sensor->identifier();

    if (sensor->isFeatureSupported(QSensor::SkipDuplicates)) {
        sensor->setSkipDuplicates(true);
    }

    const qrangelist dataRates = sensor->availableDataRates();
    if (!dataRates.isEmpty()) {
        qCDebug(QGCSensorsLog) << "Available Data Rates:" << dataRates;
        qCDebug(QGCSensorsLog) << "Selected Data Rate:" << sensor->dataRate();
    }

    const qoutputrangelist outputRanges = sensor->outputRanges();
    if (!outputRanges.isEmpty()) {
        const int outputRangeIndex = sensor->outputRange();
        // The index is -1 unless an output range was selected
        if ((outputRangeIndex >= 0) && (outputRangeIndex < outputRanges.size())) {
            const qoutputrange outputRange = outputRanges.at(outputRangeIndex);
            qCDebug(QGCSensorsLog) << "Selected Output Range:" << outputRange.minimum << outputRange.maximum
                                   << outputRange.accuracy;
        }
    }

    return true;
}

bool startSensor(QSensor* sensor, const char* name)
{
    if (!sensor->start()) {
        qCWarning(QGCSensorsLog) << "Failed to start" << name;
        return false;
    }

    return true;
}

}  // namespace

////////////////////////////////////////////////////////////////////

Q_APPLICATION_STATIC(QGCAmbientTemperature, s_ambientTemperature);

QGCAmbientTemperature* QGCAmbientTemperature::instance()
{
    return s_ambientTemperature();
}

QGCAmbientTemperature::QGCAmbientTemperature(QObject* parent)
    : QObject(parent)
    , _ambientTemperature(new QAmbientTemperatureSensor(this))
{
    connect(_ambientTemperature, &QAmbientTemperatureSensor::sensorError, this,
            [](int error) { qCWarning(QGCSensorsLog) << "QAmbientTemperature error:" << error; });

    if (!init()) {
        qCWarning(QGCSensorsLog) << "Error Initializing Ambient Temperature Sensor";
    }

    qCDebug(QGCSensorsLog) << this;
}

QGCAmbientTemperature::~QGCAmbientTemperature()
{
    qCDebug(QGCSensorsLog) << this;
}

bool QGCAmbientTemperature::init()
{
    if (!connectSensor(_ambientTemperature, &_ambientTemperatureFilter, "ambient temperature")) {
        return false;
    }

    // A connection left by an earlier init() would report every reading again
    (void) disconnect(_readingChangedConnection);
    _readingChangedConnection = connect(_ambientTemperature, &QAmbientTemperatureSensor::readingChanged, this, [this]() {
        QAmbientTemperatureReading* reading = _ambientTemperature->reading();
        if (!reading) {
            return;
        }

        _temperatureC = reading->temperature();

        emit temperatureUpdated(_temperatureC);
    });

    return startSensor(_ambientTemperature, "ambient temperature");
}

void QGCAmbientTemperature::quit()
{
    _ambientTemperature->stop();
    _ambientTemperature->disconnect(_readingChangedConnection);
}

bool QGCAmbientTemperatureFilter::filter(QAmbientTemperatureReading *reading)
{
    if (!reading) {
        return false;
    }

    const qreal temperature = reading->temperature();
    return ((temperature >= minValidTemperatureC) && (temperature <= maxValidTemperatureC));
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
{
    connect(_pressure, &QPressureSensor::sensorError, this,
            [](int error) { qCWarning(QGCSensorsLog) << "QPressure error:" << error; });

    if (!init()) {
        qCWarning(QGCSensorsLog) << "Error Initializing Pressure Sensor";
    }

    qCDebug(QGCSensorsLog) << this;
}

QGCPressure::~QGCPressure()
{
    qCDebug(QGCSensorsLog) << this;
}

bool QGCPressure::init()
{
    if (!connectSensor(_pressure, &_pressureFilter, "pressure")) {
        return false;
    }

    // A connection left by an earlier init() would report every reading again
    (void) disconnect(_readingChangedConnection);
    _readingChangedConnection = connect(_pressure, &QPressureSensor::readingChanged, this, [this]() {
        QPressureReading* reading = _pressure->reading();
        if (!reading) {
            return;
        }

        _temperatureC = reading->temperature();
        _pressurePa = reading->pressure();
        emit pressureUpdated(_pressurePa, _temperatureC);
    });

    return startSensor(_pressure, "pressure");
}

void QGCPressure::quit()
{
    _pressure->stop();
    _pressure->disconnect(_readingChangedConnection);
}

bool QGCPressureFilter::filter(QPressureReading *reading)
{
    if (!reading) {
        return false;
    }

    // Tested as inside the range, so that NaN, which fails every comparison, is rejected
    const qreal temperature = reading->temperature();
    const qreal pressure = reading->pressure();
    return ((temperature >= minValidTemperatureC) && (temperature <= maxValidTemperatureC) &&
            (pressure >= s_minValidPressurePa) && (pressure <= s_maxValidPressurePa));
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
{
    (void) connect(_compass, &QCompass::sensorError, this,
                   [](int error) { qCWarning(QGCSensorsLog) << "Compass error:" << error; });

    if (!init()) {
        qCWarning(QGCSensorsLog) << "Error Initializing Compass Sensor";
    }
}

bool QGCCompass::init()
{
    if (!connectSensor(_compass, &_compassFilter, "compass")) {
        return false;
    }

    // A connection left by an earlier init() would report every reading again
    (void) disconnect(_readingChangedConnection);
    _readingChangedConnection = connect(_compass, &QCompass::readingChanged, this, [this]() {
        QCompassReading* const reading = _compass->reading();
        if (!reading) {
            return;
        }

        const qreal calibrationLevel = reading->calibrationLevel();
        const qreal azimuth = reading->azimuth();

        emit compassUpdated(azimuth);

        QGeoPositionInfo update;
        update.setAttribute(QGeoPositionInfo::Attribute::Direction, azimuth);
        update.setAttribute(QGeoPositionInfo::Attribute::DirectionAccuracy, calibrationLevel);
        update.setTimestamp(QDateTime::currentDateTimeUtc());
        emit positionUpdated(update);
    });

    return startSensor(_compass, "compass");
}

void QGCCompass::quit()
{
    _compass->stop();
    _compass->disconnect(_readingChangedConnection);
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

} // namespace QGCSensors
