#include "QGCSensors.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QApplicationStatic>

QGC_LOGGING_CATEGORY(QGCSensorsLog, "Utilities.QGCSensors")

namespace QGCSensors
{

namespace {

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
        // sensor->setDataRate(dataRates.first().first);
        qCDebug(QGCSensorsLog) << "Selected Data Rate:" << sensor->dataRate();
    }

    const qoutputrangelist outputRanges = sensor->outputRanges();
    if (!outputRanges.isEmpty()) {
        // qCDebug(QGCSensorsLog) << "Output Ranges:" << outputRanges;
        // sensor->setOutputRange(outputRanges.first().first);
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
    // sensor->setActive(true);
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
    , _ambientTemperatureFilter(std::make_shared<QGCAmbientTemperatureFilter>())
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
    if (!connectSensor(_ambientTemperature, _ambientTemperatureFilter.get(), "ambient temperature")) {
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
    // _ambientTemperature->setActive(false);
    _ambientTemperature->stop();
    _ambientTemperature->disconnect(_readingChangedConnection);
}

QGCAmbientTemperatureFilter::QGCAmbientTemperatureFilter()
    : QAmbientTemperatureFilter()
{
    qCDebug(QGCSensorsLog) << this;
}

QGCAmbientTemperatureFilter::~QGCAmbientTemperatureFilter()
{
    qCDebug(QGCSensorsLog) << this;
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
    if (!connectSensor(_pressure, _pressureFilter.get(), "pressure")) {
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
    // _pressure->setActive(false);
    _pressure->stop();
    _pressure->disconnect(_readingChangedConnection);
}

QGCPressureFilter::QGCPressureFilter()
    : QPressureFilter()
{
    qCDebug(QGCSensorsLog) << this;
}

QGCPressureFilter::~QGCPressureFilter()
{
    qCDebug(QGCSensorsLog) << this;
}

bool QGCPressureFilter::filter(QPressureReading *reading)
{
    if (!reading) {
        return false;
    }

    // Tested as inside the range, so that NaN, which fails every comparison, is rejected
    const qreal temperature = reading->temperature();
    const qreal pressure = reading->pressure();
    return ((temperature >= s_minValidTemperatureC) && (temperature <= s_maxValidTemperatureC) &&
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
    , _compassFilter(std::make_shared<QGCCompassFilter>())
{
    // qCDebug(QGCSensorsLog) << Q_FUNC_INFO << this;

    (void) connect(_compass, &QCompass::sensorError, this,
                   [](int error) { qCWarning(QGCSensorsLog) << "Compass error:" << error; });

    if (!init()) {
        qCWarning(QGCSensorsLog) << "Error Initializing Compass Sensor";
    }
}

QGCCompass::~QGCCompass()
{
    // qCDebug(QGCSensorsLog) << Q_FUNC_INFO << this;
}

bool QGCCompass::init()
{
    if (!connectSensor(_compass, _compassFilter.get(), "compass")) {
        return false;
    }

    // A connection left by an earlier init() would report every reading again
    (void) disconnect(_readingChangedConnection);
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

    return startSensor(_compass, "compass");
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
    qCDebug(QGCSensorsLog) << this;
}

QGCCompassFilter::~QGCCompassFilter()
{
    qCDebug(QGCSensorsLog) << this;
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
