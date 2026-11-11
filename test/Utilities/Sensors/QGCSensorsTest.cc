#include "QGCSensorsTest.h"

#include <limits>
#include <type_traits>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoPositionInfo>
#include <QtSensors/QSensorBackend>
#include <QtSensors/QSensorManager>
#include <QtTest/QSignalSpy>

#include "QGCSensors.h"
#include "TestFixtures.h"

namespace {

constexpr char kSensorsLogCategory[] = "Utilities.QGCSensors";
constexpr char kFakeSensorIdentifier[] = "qgc.test.fake";

/// A Qt Sensors backend the test drives: it publishes the readings the test gives it and can refuse to start.
template <typename Reading>
class FakeSensorBackend : public QSensorBackend
{
public:
    FakeSensorBackend(QSensor* sensor, bool startable)
        : QSensorBackend(sensor)
        , _startable(startable)
    {
        (void) setReading<Reading>(&_reading);
        addDataRate(1, 50);
    }

    void start() override
    {
        if (!_startable) {
            sensorStopped();
        }
    }

    void stop() override {}

    bool isFeatureSupported(QSensor::Feature feature) const override { return feature == QSensor::SkipDuplicates; }

    /// The reading newReadingAvailable() publishes.
    Reading& nextReading() { return _reading; }

private:
    Reading _reading;
    const bool _startable;
};

/// Makes the fake backend the one Qt creates for sensors of one type, for as long as this lives.
template <typename Reading>
class FakeSensorRegistration : public QSensorBackendFactory
{
public:
    explicit FakeSensorRegistration(const QByteArray& sensorType, bool startable = true)
        : _sensorType(sensorType)
        , _startable(startable)
        , _previousDefault(QSensor::defaultSensorForType(sensorType))
    {
        QSensorManager::registerBackend(_sensorType, kFakeSensorIdentifier, this);
        // A host with a sensor of this type would otherwise keep its own as the default.
        QSensorManager::setDefaultBackend(_sensorType, kFakeSensorIdentifier);
    }

    ~FakeSensorRegistration() override
    {
        QSensorManager::unregisterBackend(_sensorType, kFakeSensorIdentifier);
        if (!_previousDefault.isEmpty()) {
            QSensorManager::setDefaultBackend(_sensorType, _previousDefault);
        }
    }

    QSensorBackend* createBackend(QSensor* sensor) override
    {
        _backend = new FakeSensorBackend<Reading>(sensor, _startable);
        return _backend;
    }

    /// The backend of the sensor created last, which owns it.
    FakeSensorBackend<Reading>* backend() const { return _backend; }

private:
    Q_DISABLE_COPY_MOVE(FakeSensorRegistration)

    const QByteArray _sensorType;
    const bool _startable;
    const QByteArray _previousDefault;
    QPointer<FakeSensorBackend<Reading>> _backend;
};

/// What tells the three sensors apart: the Qt sensor they wrap, the names they log and the reading they report.
template <typename Sensor>
struct SensorTraits;

template <>
struct SensorTraits<QGCSensors::QGCAmbientTemperature>
{
    using Reading = QAmbientTemperatureReading;

    static constexpr const char* backendName = "ambient temperature";
    static constexpr const char* initFailure = "Error Initializing Ambient Temperature Sensor";
    static constexpr const char* sensorError = "QAmbientTemperature error:";
    static constexpr auto updated = &QGCSensors::QGCAmbientTemperature::temperatureUpdated;

    static QByteArray qtSensorType() { return QAmbientTemperatureSensor::sensorType; }

    static void setReading(Reading& reading, bool plausible) { reading.setTemperature(plausible ? 21.5 : 500.0); }

    static bool reportedPlausibleReading(const QGCSensors::QGCAmbientTemperature& sensor, const QVariantList& update)
    {
        return (update == QVariantList{21.5}) && (sensor.temperature() == 21.5);
    }
};

template <>
struct SensorTraits<QGCSensors::QGCPressure>
{
    using Reading = QPressureReading;

    static constexpr const char* backendName = "pressure";
    static constexpr const char* initFailure = "Error Initializing Pressure Sensor";
    static constexpr const char* sensorError = "QPressure error:";
    static constexpr auto updated = &QGCSensors::QGCPressure::pressureUpdated;

    static QByteArray qtSensorType() { return QPressureSensor::sensorType; }

    static void setReading(Reading& reading, bool plausible)
    {
        reading.setPressure(plausible ? 101325.0 : 10.0);
        reading.setTemperature(21.5);
    }

    static bool reportedPlausibleReading(const QGCSensors::QGCPressure& sensor, const QVariantList& update)
    {
        return (update == QVariantList{101325.0, 21.5}) && (sensor.pressure() == 101325.0) &&
               (sensor.temperature() == 21.5);
    }
};

template <>
struct SensorTraits<QGCSensors::QGCCompass>
{
    using Reading = QCompassReading;

    static constexpr const char* backendName = "compass";
    static constexpr const char* initFailure = "Error Initializing Compass Sensor";
    static constexpr const char* sensorError = "Compass error:";
    static constexpr auto updated = &QGCSensors::QGCCompass::compassUpdated;

    static QByteArray qtSensorType() { return QCompass::sensorType; }

    static void setReading(Reading& reading, bool plausible)
    {
        reading.setAzimuth(123.0);
        reading.setCalibrationLevel(plausible ? 1.0 : 0.1);
    }

    static bool reportedPlausibleReading(const QGCSensors::QGCCompass&, const QVariantList& update)
    {
        return update == QVariantList{123.0};
    }
};

enum class SensorKind
{
    AmbientTemperature,
    Pressure,
    Compass,
};

void addSensorKindRows()
{
    QTest::addColumn<int>("kind");

    QTest::newRow("ambient temperature") << static_cast<int>(SensorKind::AmbientTemperature);
    QTest::newRow("pressure") << static_cast<int>(SensorKind::Pressure);
    QTest::newRow("compass") << static_cast<int>(SensorKind::Compass);
}

/// Calls @a check with the sensor class of @a kind.
template <typename Check>
void forSensorKind(int kind, Check check)
{
    switch (static_cast<SensorKind>(kind)) {
        case SensorKind::AmbientTemperature:
            check(std::type_identity<QGCSensors::QGCAmbientTemperature>());
            break;
        case SensorKind::Pressure:
            check(std::type_identity<QGCSensors::QGCPressure>());
            break;
        case SensorKind::Compass:
            check(std::type_identity<QGCSensors::QGCCompass>());
            break;
    }
}

/// A pattern for the end of a log message, so that it matches whatever the message starts with.
QRegularExpression messageEnding(const QString& ending)
{
    return QRegularExpression(QRegularExpression::escape(ending) + QLatin1Char('$'));
}

}  // namespace

void QGCSensorsTest::_testAmbientTemperatureFilter_data()
{
    QTest::addColumn<qreal>("temperatureC");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("room temperature") << 21.5 << true;
    QTest::newRow("coldest valid") << -40.0 << true;
    QTest::newRow("below coldest valid") << -40.01 << false;
    QTest::newRow("hottest valid") << 85.0 << true;
    QTest::newRow("above hottest valid") << 85.01 << false;
    QTest::newRow("not a number") << std::numeric_limits<qreal>::quiet_NaN() << false;
    QTest::newRow("infinite") << std::numeric_limits<qreal>::infinity() << false;
}

void QGCSensorsTest::_testAmbientTemperatureFilter()
{
    QFETCH(qreal, temperatureC);
    QFETCH(bool, accepted);

    QAmbientTemperatureReading reading;
    reading.setTemperature(temperatureC);

    QGCSensors::QGCAmbientTemperatureFilter filter;
    QCOMPARE(filter.filter(&reading), accepted);
}

void QGCSensorsTest::_testPressureFilter_data()
{
    QTest::addColumn<qreal>("pressurePa");
    QTest::addColumn<qreal>("temperatureC");
    QTest::addColumn<bool>("accepted");

    constexpr qreal seaLevelPa = 101325.0;
    constexpr qreal roomC = 21.5;

    QTest::newRow("sea level at room temperature") << seaLevelPa << roomC << true;
    QTest::newRow("lowest valid pressure") << 45000.0 << roomC << true;
    QTest::newRow("below lowest valid pressure") << 44999.9 << roomC << false;
    QTest::newRow("highest valid pressure") << 110000.0 << roomC << true;
    QTest::newRow("above highest valid pressure") << 110000.1 << roomC << false;
    QTest::newRow("infinite pressure") << std::numeric_limits<qreal>::infinity() << roomC << false;
    QTest::newRow("pressure not a number") << std::numeric_limits<qreal>::quiet_NaN() << roomC << false;
    QTest::newRow("coldest valid temperature") << seaLevelPa << -40.0 << true;
    QTest::newRow("below coldest valid temperature") << seaLevelPa << -40.01 << false;
    QTest::newRow("hottest valid temperature") << seaLevelPa << 85.0 << true;
    QTest::newRow("above hottest valid temperature") << seaLevelPa << 85.01 << false;
    QTest::newRow("temperature not a number") << seaLevelPa << std::numeric_limits<qreal>::quiet_NaN() << false;
}

void QGCSensorsTest::_testPressureFilter()
{
    QFETCH(qreal, pressurePa);
    QFETCH(qreal, temperatureC);
    QFETCH(bool, accepted);

    QPressureReading reading;
    reading.setPressure(pressurePa);
    reading.setTemperature(temperatureC);

    QGCSensors::QGCPressureFilter filter;
    QCOMPARE(filter.filter(&reading), accepted);
}

void QGCSensorsTest::_testCompassFilter_data()
{
    QTest::addColumn<qreal>("calibrationLevel");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("fully calibrated") << 1.0 << true;
    QTest::newRow("lowest usable calibration") << 0.65 << true;
    QTest::newRow("below lowest usable calibration") << 0.64 << false;
    QTest::newRow("uncalibrated") << 0.0 << false;
    QTest::newRow("not a number") << std::numeric_limits<qreal>::quiet_NaN() << false;
}

void QGCSensorsTest::_testCompassFilter()
{
    QFETCH(qreal, calibrationLevel);
    QFETCH(bool, accepted);

    QCompassReading reading;
    reading.setAzimuth(123.0);
    reading.setCalibrationLevel(calibrationLevel);

    QGCSensors::QGCCompassFilter filter;
    QCOMPARE(filter.filter(&reading), accepted);
}

template <typename Sensor>
void QGCSensorsTest::_checkSensorAbsent()
{
    using Traits = SensorTraits<Sensor>;
    if (!QSensor::sensorsForType(Traits::qtSensorType()).isEmpty()) {
        QSKIP("This host has a sensor of this type");
    }

    const QRegularExpression connectFailure =
        messageEnding(QStringLiteral("Failed to connect to %1 backend").arg(QLatin1StringView(Traits::backendName)));

    expectLogMessage(kSensorsLogCategory, QtWarningMsg, connectFailure);
    expectLogMessage(kSensorsLogCategory, QtWarningMsg, messageEnding(QLatin1StringView(Traits::initFailure)));
    Sensor sensor;
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    expectLogMessage(kSensorsLogCategory, QtWarningMsg, connectFailure);
    QVERIFY(!sensor.init());
    verifyExpectedLogMessage();
}

template <typename Sensor>
void QGCSensorsTest::_checkSensorFailsToStart()
{
    using Traits = SensorTraits<Sensor>;
    FakeSensorRegistration<typename Traits::Reading> fake(Traits::qtSensorType(), false);

    expectLogMessage(kSensorsLogCategory, QtWarningMsg,
                     messageEnding(QStringLiteral("Failed to start %1").arg(QLatin1StringView(Traits::backendName))));
    expectLogMessage(kSensorsLogCategory, QtWarningMsg, messageEnding(QLatin1StringView(Traits::initFailure)));
    const Sensor sensor;
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QVERIFY(fake.backend());
    QVERIFY(!fake.backend()->sensor()->isActive());
}

template <typename Sensor>
void QGCSensorsTest::_checkSensorReportsReadings()
{
    using Traits = SensorTraits<Sensor>;
    FakeSensorRegistration<typename Traits::Reading> fake(Traits::qtSensorType());

    Sensor sensor;
    auto* const backend = fake.backend();
    QVERIFY(backend);
    QVERIFY(backend->sensor()->isActive());
    QVERIFY(backend->sensor()->skipDuplicates());

    QSignalSpy updates(&sensor, Traits::updated);
    const auto publish = [backend](bool plausible) {
        Traits::setReading(backend->nextReading(), plausible);
        backend->newReadingAvailable();
    };

    publish(true);
    QCOMPARE(updates.count(), 1);
    QVERIFY(Traits::reportedPlausibleReading(sensor, updates.constLast()));

    // The sensor's filter drops a reading the device cannot have measured.
    publish(false);
    QCOMPARE(updates.count(), 1);

    expectLogMessage(kSensorsLogCategory, QtWarningMsg,
                     messageEnding(QStringLiteral("%1 7").arg(QLatin1StringView(Traits::sensorError))));
    backend->sensorError(7);
    verifyExpectedLogMessage();

    sensor.quit();
    QVERIFY(!backend->sensor()->isActive());
    publish(true);
    QCOMPARE(updates.count(), 1);

    QVERIFY(sensor.init());
    QVERIFY(backend->sensor()->isActive());
    publish(true);
    QCOMPARE(updates.count(), 2);
}

template <typename Sensor>
void QGCSensorsTest::_checkSensorLogsBackend()
{
    using Traits = SensorTraits<Sensor>;
    const TestFixtures::LoggingCategoryFixture debugLog(QString::fromLatin1(kSensorsLogCategory));
    // The sensor and its filter log their address when they are created and destroyed.
    ignoreLogMessage(kSensorsLogCategory, QtDebugMsg, QRegularExpression(QStringLiteral("0x[0-9a-f]+\\)?$")));

    FakeSensorRegistration<typename Traits::Reading> fake(Traits::qtSensorType());

    expectLogMessage(
        kSensorsLogCategory, QtDebugMsg,
        messageEnding(QStringLiteral("Connected to %1 backend: \"%2\"")
                          .arg(QLatin1StringView(Traits::backendName), QLatin1StringView(kFakeSensorIdentifier))));
    expectLogMessage(kSensorsLogCategory, QtDebugMsg,
                     messageEnding(QStringLiteral("Available Data Rates: QList(std::pair(1, 50))")));
    expectLogMessage(kSensorsLogCategory, QtDebugMsg, messageEnding(QStringLiteral("Selected Data Rate: 0")));
    const Sensor sensor;
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void QGCSensorsTest::_testSensorAbsent_data()
{
    addSensorKindRows();
}

void QGCSensorsTest::_testSensorAbsent()
{
    QFETCH(int, kind);
    forSensorKind(kind, [this]<typename Sensor>(std::type_identity<Sensor>) { _checkSensorAbsent<Sensor>(); });
}

void QGCSensorsTest::_testSensorFailsToStart_data()
{
    addSensorKindRows();
}

void QGCSensorsTest::_testSensorFailsToStart()
{
    QFETCH(int, kind);
    forSensorKind(kind, [this]<typename Sensor>(std::type_identity<Sensor>) { _checkSensorFailsToStart<Sensor>(); });
}

void QGCSensorsTest::_testSensorReportsReadings_data()
{
    addSensorKindRows();
}

void QGCSensorsTest::_testSensorReportsReadings()
{
    QFETCH(int, kind);
    forSensorKind(kind, [this]<typename Sensor>(std::type_identity<Sensor>) { _checkSensorReportsReadings<Sensor>(); });
}

void QGCSensorsTest::_testSensorLogsBackend_data()
{
    addSensorKindRows();
}

void QGCSensorsTest::_testSensorLogsBackend()
{
    QFETCH(int, kind);
    forSensorKind(kind, [this]<typename Sensor>(std::type_identity<Sensor>) { _checkSensorLogsBackend<Sensor>(); });
}

void QGCSensorsTest::_testCompassReportsDirection()
{
    FakeSensorRegistration<QCompassReading> fake(QCompass::sensorType);
    const QGCSensors::QGCCompass compass;
    auto* const backend = fake.backend();
    QVERIFY(backend);
    QSignalSpy positions(&compass, &QGCSensors::QGCCompass::positionUpdated);

    backend->nextReading().setAzimuth(123.0);
    backend->nextReading().setCalibrationLevel(0.8);
    backend->newReadingAvailable();

    QCOMPARE(positions.count(), 1);
    const auto position = positions.constFirst().constFirst().value<QGeoPositionInfo>();
    QCOMPARE(position.attribute(QGeoPositionInfo::Direction), 123.0);
    QCOMPARE(position.attribute(QGeoPositionInfo::DirectionAccuracy), 0.8);
    QVERIFY(position.timestamp().isValid());
}

UT_REGISTER_TEST(QGCSensorsTest, TestLabel::Unit, TestLabel::Utilities)
