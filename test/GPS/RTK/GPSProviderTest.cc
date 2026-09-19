#include "GPSProviderTest.h"

#include <cmath>
#include <cstring>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "GPSProvider.h"
#include "GPSReceiverConfigValidation.h"
#include "GPSTransport.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

Q_DECLARE_METATYPE(GPSBaseStationConfig)
Q_DECLARE_METATYPE(GPSSurveyReport)

void GPSProviderTest::_queuedPayloadsOwnSnapshots()
{
    GPSProvider provider({}, GPSType::ublox, {});
    for (const auto name : {"GPSSatelliteReport", "GPSPositionReport", "GPSConnectionError", "GPSSurveyInStatus"}) {
        QVERIFY2(QMetaType::fromName(name).isValid(), name);
    }
    GPSSatelliteReport satellites;
    GPSPositionReport position;
    GPSSurveyInStatus survey;
    GPSConnectionError error = GPSConnectionError::None;
    QObject receiver;
    connect(
        &provider, &GPSProvider::satelliteInfoUpdate, &receiver,
        [&](const GPSSatelliteReport& value) { satellites = value; }, Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::sensorGpsUpdate, &receiver, [&](const GPSPositionReport& value) { position = value; },
        Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::surveyInStatus, &receiver, [&](const GPSSurveyInStatus& value) { survey = value; },
        Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::connectionError, &receiver, [&](GPSConnectionError value) { error = value; },
        Qt::QueuedConnection);
    auto worker = std::unique_ptr<QThread>(QThread::create([&]() {
        GPSSatelliteReport snapshot;
        snapshot.count = 1;
        snapshot.satellites[0].used = true;
        emit provider.satelliteInfoUpdate(snapshot);
        snapshot.satellites[0].used = false;
        GPSPositionReport fix;
        fix.latitudeDegrees = 47;
        fix.integrity.jamming = GPSIntegrityReport::JammingState::Warning;
        fix.integrity.noisePerMillisecond = 0;
        emit provider.sensorGpsUpdate(fix);
        fix.latitudeDegrees = 0;
        fix.integrity.noisePerMillisecond = 99;
        GPSSurveyReport progress;
        progress.duration = std::chrono::seconds(4294967295LL);
        progress.meanAccuracyMeters = 1.234;
        provider._handleSurveyIn(progress);
        progress.meanAccuracyMeters = 0;
        emit provider.connectionError(GPSConnectionError::DeviceError);
    }));
    worker->start();
    const bool timely = worker->wait(TestTimeout::shortMs());
    if (!timely) {
        worker->wait();
    }
    QVERIFY(timely);
    QCOMPARE(satellites.count, 0);
    QCOMPARE(survey.duration.count(), 0);
    QCoreApplication::sendPostedEvents(&receiver, QEvent::MetaCall);
    QCOMPARE(satellites.count, 1);
    QCOMPARE(satellites.satellites[0].used, std::optional<bool>{true});
    QCOMPARE(position.latitudeDegrees, 47);
    QCOMPARE(position.integrity.jamming, GPSIntegrityReport::JammingState::Warning);
    QCOMPARE(position.integrity.noisePerMillisecond, std::optional<int32_t>{0});
    QCOMPARE(position.integrity.timestampUs, uint64_t{0});
    QCOMPARE(survey.duration.count(), 4294967295LL);
    QCOMPARE(survey.meanAccuracyMeters.value(), 1.234);
    QCOMPARE(error, GPSConnectionError::DeviceError);
}

void GPSProviderTest::_surveyReportProjection_data()
{
    QTest::addColumn<GPSSurveyReport>("report");
    QTest::addColumn<bool>("coordinateValid");

    QTest::newRow("unknown") << GPSSurveyReport{} << false;
    QTest::newRow("fixed") << GPSSurveyReport{.latitudeDegrees = -47.123,
                                              .longitudeDegrees = 128.456,
                                              .altitudeEllipsoidMeters = 512.25f,
                                              .meanAccuracyMeters = 1.234,
                                              .duration = std::chrono::seconds(4294967295LL),
                                              .valid = true,
                                              .active = false}
                           << true;
    QTest::newRow("survey-zero-accuracy") << GPSSurveyReport{.latitudeDegrees = 47,
                                                             .longitudeDegrees = -8,
                                                             .altitudeEllipsoidMeters = -30,
                                                             .meanAccuracyMeters = 0,
                                                             .duration = std::chrono::seconds(180),
                                                             .valid = false,
                                                             .active = true}
                                          << true;
    QTest::newRow("valid-active-unknown-accuracy")
        << GPSSurveyReport{.latitudeDegrees = 0, .longitudeDegrees = 0, .valid = true, .active = true} << true;
    QTest::newRow("invalid-latitude") << GPSSurveyReport{.latitudeDegrees = 91, .longitudeDegrees = 8} << false;
    QTest::newRow("invalid-longitude") << GPSSurveyReport{.latitudeDegrees = 47, .longitudeDegrees = -181} << false;
}

void GPSProviderTest::_surveyReportProjection()
{
    QFETCH(GPSSurveyReport, report);
    QFETCH(bool, coordinateValid);
    GPSProvider provider({}, GPSType::ublox, {});
    QSignalSpy reports(&provider, &GPSProvider::surveyInStatus);

    provider._handleSurveyIn(report);

    QCOMPARE(reports.size(), 1);
    const auto status = qvariant_cast<GPSSurveyInStatus>(reports.first().first());
    QCOMPARE(status.coordinate.isValid(), coordinateValid);
    if (coordinateValid) {
        QCOMPARE(status.coordinate.latitude(), report.latitudeDegrees);
        QCOMPARE(status.coordinate.longitude(), report.longitudeDegrees);
        QCOMPARE(status.coordinate.type(), QGeoCoordinate::Coordinate2D);
    }
    QVERIFY(std::isnan(status.coordinate.altitude()));
    if (std::isnan(report.altitudeEllipsoidMeters)) {
        QVERIFY(std::isnan(status.altitudeEllipsoidMeters));
    } else {
        QCOMPARE(status.altitudeEllipsoidMeters, report.altitudeEllipsoidMeters);
    }
    QCOMPARE(status.altitudeDatum, GPSAltitudeDatum::Ellipsoid);
    QCOMPARE(status.meanAccuracyMeters, report.meanAccuracyMeters);
    QCOMPARE(status.duration, report.duration);
    QCOMPARE(status.valid, report.valid);
    QCOMPARE(status.active, report.active);
}

namespace {
struct TransportTrace
{
    QThread* constructedOn = nullptr;
    QThread* openedOn = nullptr;
    QThread* destroyedOn = nullptr;
    std::weak_ptr<int> factoryLifetime;
    bool factoryAliveDuringDestruction = false;
};

class TestTransport : public GPSTransport
{
public:
    TestTransport(const std::atomic_bool& requestStop, TransportTrace& trace, std::function<void()> stop,
                  bool openResult, bool cancelInOpen)
        : GPSTransport(requestStop)
        , _trace(trace)
        , _stop(stop)
        , _openResult(openResult)
        , _cancelInOpen(cancelInOpen)
    {
        _trace.constructedOn = QThread::currentThread();
    }

    ~TestTransport() override
    {
        _trace.destroyedOn = QThread::currentThread();
        _trace.factoryAliveDuringDestruction = !_trace.factoryLifetime.expired();
    }

    GPSOpenResult open() override
    {
        _trace.openedOn = QThread::currentThread();
        // Stop before receiver configuration; this test exercises transport ownership only.
        if (_cancelInOpen) {
            _stop();
        }
        return {_openResult ? GPSOpenStatus::Opened : GPSOpenStatus::Error};
    }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t*, int, int) override { return {GPSReadStatus::Error}; }

    GPSWriteResult write(const uint8_t*, int) override { return {GPSWriteStatus::Error}; }

    bool setBaudrate(unsigned) override { return true; }

private:
    TransportTrace& _trace;
    std::function<void()> _stop;
    bool _openResult;
    bool _cancelInOpen;
};
}  // namespace

void GPSProviderTest::_transportLifetimeStaysOnWorker_data()
{
    QTest::addColumn<bool>("openResult");
    QTest::addColumn<bool>("cancelInOpen");
    QTest::newRow("opened") << true << true;
    QTest::newRow("cancelled-during-open") << false << true;
    QTest::newRow("open-failed") << false << false;
}

void GPSProviderTest::_transportLifetimeStaysOnWorker()
{
    QFETCH(bool, openResult);
    QFETCH(bool, cancelInOpen);
    TransportTrace trace;
    auto lifetime = std::make_shared<int>(0);
    trace.factoryLifetime = lifetime;
    std::function<void()> stopProvider;
    GPSProvider provider(
        [&, lifetime = std::move(lifetime)](const std::atomic_bool& requestStop) {
            return std::make_unique<TestTransport>(requestStop, trace, stopProvider, openResult, cancelInOpen);
        },
        GPSType::ublox, GPSReceiverConfig{});
    stopProvider = [&provider]() { provider.stop(); };
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(trace.constructedOn, &provider);
    QCOMPARE(trace.openedOn, &provider);
    QCOMPARE(trace.destroyedOn, &provider);
    QVERIFY(trace.factoryAliveDuringDestruction);
    QVERIFY(trace.factoryLifetime.expired());
    QCOMPARE(errors.size(), cancelInOpen ? 0 : 1);
    if (!cancelInOpen) {
        QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::OpenFailed);
    }
}

void GPSProviderTest::_missingTransportReportsOpenFailure_data()
{
    QTest::addColumn<bool>("hasFactory");
    QTest::newRow("empty-factory") << false;
    QTest::newRow("null-transport") << true;
}

void GPSProviderTest::_missingTransportReportsOpenFailure()
{
    QFETCH(bool, hasFactory);
    GPSProvider::TransportFactory factory;
    if (hasFactory) {
        factory = [](const std::atomic_bool&) { return std::unique_ptr<GPSTransport>{}; };
    }
    GPSProvider provider(std::move(factory), GPSType::ublox, GPSReceiverConfig{});
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(errors.count(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::OpenFailed);
}

void GPSProviderTest::_cancelledProviderDoesNotCreateTransport()
{
    bool created = false;
    GPSProvider provider(
        [&](const std::atomic_bool&) {
            created = true;
            return std::unique_ptr<GPSTransport>{};
        },
        GPSType::ublox, GPSReceiverConfig{});
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.stop();
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QVERIFY(!created);
    QVERIFY(errors.isEmpty());
}

UT_REGISTER_TEST(GPSProviderTest, TestLabel::Unit)

namespace {
class FemtoAckTransport : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned) override { return true; }

    GPSWriteResult write(const uint8_t* bytes, int size) override
    {
        const QByteArray command(reinterpret_cast<const char*>(bytes), size);
        _reply = '<' + command.split(' ').first().trimmed() + " OK";
        _reply.append(char(0));
        return {GPSWriteStatus::Completed, size, size};
    }

    GPSReadResult read(uint8_t* bytes, int size, int) override
    {
        if (_reply.isEmpty()) {
            return {GPSReadStatus::Error};
        }
        const auto count = qMin(size, static_cast<int>(_reply.size()));
        std::memcpy(bytes, _reply.constData(), count);
        _reply.remove(0, count);
        return {GPSReadStatus::Data, count};
    }

private:
    QByteArray _reply;
};
}  // namespace

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss_data()
{
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::newRow("survey") << GPSBaseStationConfig{.surveyInAccMeters = 2, .surveyInDurationSecs = 180};
    QTest::newRow("minimum-survey") << GPSBaseStationConfig{.surveyInAccMeters = 0.0001, .surveyInDurationSecs = 1};
    QTest::newRow("maximum-survey") << GPSBaseStationConfig{.surveyInAccMeters = 429496.7295,
                                                            .surveyInDurationSecs = 4294967295LL};
    QTest::newRow("fixed") << GPSBaseStationConfig{.useFixedBase = true,
                                                   .fixedBaseLatitude = 47,
                                                   .fixedBaseLongitude = 8,
                                                   .fixedBaseAltitudeMeters = 500,
                                                   .fixedBaseAccuracyMeters = 1};
    QTest::newRow("fixed-wire-limits") << GPSBaseStationConfig{.useFixedBase = true,
                                                               .fixedBaseLatitude = 47,
                                                               .fixedBaseLongitude = 8,
                                                               .fixedBaseAltitudeMeters = 21474836.0f,
                                                               .fixedBaseAccuracyMeters = 429496.71875f};
    QTest::newRow("fixed-unknown-accuracy") << GPSBaseStationConfig{.useFixedBase = true,
                                                                    .fixedBaseLatitude = 47,
                                                                    .fixedBaseLongitude = 8,
                                                                    .fixedBaseAltitudeMeters = 500,
                                                                    .fixedBaseAccuracyMeters = 0};
}

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss()
{
    QFETCH(GPSBaseStationConfig, config);
    GPSProvider provider(
        [](const std::atomic_bool& requestStop) { return std::make_unique<FemtoAckTransport>(requestStop); },
        GPSType::femto, GPSReceiverConfig{.base = config});
    QSignalSpy ready(&provider, &GPSProvider::receiverReady);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    QSignalSpy surveys(&provider, &GPSProvider::surveyInStatus);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::mediumMs()));
    QCOMPARE(ready.size(), 1);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::DeviceError);
    if (config.useFixedBase) {
        QCOMPARE(surveys.size(), 1);
        const auto status = qvariant_cast<GPSSurveyInStatus>(surveys.first().first());
        QCOMPARE(status.coordinate.type(), QGeoCoordinate::Coordinate2D);
        QCOMPARE(status.coordinate.latitude(), config.fixedBaseLatitude);
        QCOMPARE(status.coordinate.longitude(), config.fixedBaseLongitude);
        QVERIFY(std::isnan(status.coordinate.altitude()));
        QCOMPARE(status.altitudeEllipsoidMeters, config.fixedBaseAltitudeMeters);
        QCOMPARE(status.altitudeDatum, GPSAltitudeDatum::Ellipsoid);
        QVERIFY(!status.meanAccuracyMeters.has_value());
        QCOMPARE(status.duration.count(), 0);
        QVERIFY(status.valid);
        QVERIFY(!status.active);
    }
}

void GPSProviderTest::_unsupportedPositionRoleReportsConfigFailure_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::newRow("trimble") << GPSType::trimble;
    QTest::newRow("septentrio") << GPSType::septentrio;
    QTest::newRow("femto") << GPSType::femto;
}

void GPSProviderTest::_unsupportedPositionRoleReportsConfigFailure()
{
    QFETCH(GPSType, type);
    const GPSReceiverConfig config{.role = GPSReceiverConfig::Role::Position};
    const QString error = gpsReceiverConfigError(type, config);
    QVERIFY(!error.isEmpty());
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(QRegularExpression::escape(error)));
    GPSProvider provider(
        [](const std::atomic_bool& requestStop) { return std::make_unique<FemtoAckTransport>(requestStop); }, type,
        config);
    QSignalSpy ready(&provider, &GPSProvider::receiverReady);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
    QVERIFY(ready.isEmpty());
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::ConfigFailed);
}

void GPSProviderTest::_cancelledFactoryDoesNotOpenTransport()
{
    TransportTrace trace;
    std::function<void()> stopProvider;
    GPSProvider provider(
        [&](const std::atomic_bool& requestStop) {
            stopProvider();
            return std::make_unique<TestTransport>(requestStop, trace, []() {}, true, false);
        },
        GPSType::ublox, GPSReceiverConfig{});
    stopProvider = [&provider]() { provider.stop(); };
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(trace.constructedOn, &provider);
    QVERIFY(!trace.openedOn);
    QCOMPARE(trace.destroyedOn, &provider);
    QVERIFY(errors.isEmpty());
}

#ifndef QGC_NO_SERIAL_LINK
void GPSProviderTest::_finishedReceiverReleasesReservation_data()
{
    QTest::addColumn<bool>("cancelled");
    QTest::newRow("open-failed") << false;
    QTest::newRow("cancelled-before-start") << true;
}

void GPSProviderTest::_finishedReceiverReleasesReservation()
{
    QFETCH(bool, cancelled);
    QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/gps"), QStringLiteral("gps"), QGCSerialPortInfo::BoardTypeRTKGPS, QString()}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    ports.setSinglePortOnly(true);
    QCOMPARE(ports.availablePorts().size(), 1);
    auto reservation = ports.reservePort(QStringLiteral("/test/gps"));
    QVERIFY(reservation);
    GPSProvider provider(
        [reservation = std::move(reservation)](const std::atomic_bool&) { return std::unique_ptr<GPSTransport>{}; },
        GPSType::ublox, GPSReceiverConfig{});
    if (cancelled) {
        provider.stop();
    }
    QVERIFY(!ports.canReservePort(QStringLiteral("/test/mavlink")));
    inventory.clear();
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QVERIFY(!ports.anyPortReserved());
    QVERIFY(ports.reservePort(QStringLiteral("/test/mavlink")));
    QTRY_VERIFY_WITH_TIMEOUT(ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
}
#endif
