#include "PositionManagerTest.h"

#include <utility>

#include <QtCore/QIODevice>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "LogManager.h"
#include "ManualScheduler.h"
#include "NMEASourceManager.h"
#include "NMEAUtils.h"
#include "PositionManager.h"
#include "QGCLoggingCategoryManager.h"
#include "SequentialTestDevice.h"
#include "SettingsManager.h"
#include "SimulatedPosition.h"
#include "Vehicle.h"

namespace {

// Fix at 53.361337, -6.50562 (RMC provides the date, GGA provides HDOP for accuracy)
constexpr const char* kNmeaSentences =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";

constexpr double kExpectedLat = 53.361337;
constexpr double kExpectedLon = -6.50562;
constexpr double kCoordEpsilon = 0.0001;

}  // namespace

void PositionManagerTest::init()
{
    UnitTest::init();
    _previousNmeaInput = QGCPositionManager::instance()->nmeaInput();
    // Headless CI has no real position source, so the internal GPS fallback
    // times out waiting for updates. Expected and benign in this fixture.
    ignoreLogMessage("GPS.PositionManager.GPSPositionService", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UpdateTimeoutError")));
}

void PositionManagerTest::cleanup()
{
    delete std::exchange(_nmeaInput, nullptr);
    QGCPositionManager::instance()->setNmeaInput(_previousNmeaInput);
    delete _nmeaDevice;
    _nmeaDevice = nullptr;

    UnitTest::cleanup();
}

void PositionManagerTest::_nmeaSourceProducesGcsPosition()
{
    QGCPositionManager* pm = QGCPositionManager::instance();
    auto* device = new SequentialTestDevice();
    _nmeaDevice = device;

    _nmeaInput = new NMEASourceManager(nullptr, pm, this);
    _nmeaInput->_startDecoder(device);
    device->feed(kNmeaSentences);

    QTRY_VERIFY_WITH_TIMEOUT(pm->gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(pm->gcsPosition().latitude() - kExpectedLat) < kCoordEpsilon);
    QVERIFY(qAbs(pm->gcsPosition().longitude() - kExpectedLon) < kCoordEpsilon);
    QVERIFY(pm->gcsPositionHorizontalAccuracy() < 100.);
}

void PositionManagerTest::_resetNmeaSourceTearsDownAndClearsState()
{
    QGCPositionManager* pm = QGCPositionManager::instance();
    auto* device = new SequentialTestDevice();
    _nmeaDevice = device;

    _nmeaInput = new NMEASourceManager(nullptr, pm, this);
    _nmeaInput->_startDecoder(device);
    device->feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm->gcsPosition().isValid(), TestTimeout::mediumMs());

    QSignalSpy positionInfoSpy(pm, &QGCPositionManager::gcsPositionChanged);
    QVERIFY(positionInfoSpy.isValid());

    _nmeaInput->stop();

    // Stale GCS state must be cleared on teardown
    QVERIFY(!pm->gcsPosition().isValid());
    QVERIFY(qIsInf(pm->gcsPositionHorizontalAccuracy()));
    QVERIFY(!positionInfoSpy.isEmpty());

    // The NMEA source is gone: further data must not resurrect the position
    positionInfoSpy.clear();
    device->feed(kNmeaSentences);
    QVERIFY(!positionInfoSpy.wait(TestTimeout::shortMs()));
    QVERIFY(!pm->gcsPosition().isValid());

    // Second reset with no NMEA source is a no-op
    _nmeaInput->stop();
}

void PositionManagerTest::_nmeaUpdatesStayHealthyUntilStale()
{
    ManualScheduler scheduler;
    SequentialTestDevice device(&scheduler);
    QGCPositionManager pm(nullptr, &scheduler);
    NMEASourceManager input(nullptr, &pm);
    input._startDecoder(&device);
    pm._currentHealth->setFreshnessTimeoutMs(300);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(pm.sourceStatus(), GPSPositionService::SourceStatus::WaitingForFix);
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    int nextSecond = 0;
    const auto feed = [&]() {
        const QByteArray time =
            QDateTime::currentDateTimeUtc().addSecs(nextSecond++).toString(QStringLiteral("hhmmss.zzz")).toLatin1();
        QByteArray sentences;
        for (QByteArray line : QByteArray(kNmeaSentences).split('\n')) {
            if (!line.trimmed().isEmpty()) {
                line.replace("092750.000", time);
                sentences += NMEAUtils::repairChecksum(line);
            }
        }
        device.feed(sentences);
    };
    feed();
    QTRY_VERIFY_WITH_TIMEOUT((scheduler.advanceBy(std::chrono::microseconds::zero()), pm.gcsPosition().isValid()),
                             TestTimeout::mediumMs());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(299)));
    QVERIFY(pm.gcsPosition().isValid());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY(!pm.gcsPosition().isValid());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::UpdateTimeoutError);
    QVERIFY(!pm.acceptedObservation());
    QVERIFY(qIsInf(pm.gcsPositionHorizontalAccuracy()));
    feed();
    QTRY_VERIFY_WITH_TIMEOUT((scheduler.advanceBy(std::chrono::microseconds::zero()), pm.gcsPosition().isValid()),
                             TestTimeout::mediumMs());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    input.stop();
    QVERIFY(!pm.gcsPosition().isValid());
}

UT_REGISTER_TEST(PositionManagerTest, TestLabel::Unit)

void PositionManagerTest::_qmlPositionProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QGC
        QtObject {
            required property GPSPositionService service
            readonly property real latitude: service.gcsPosition.latitude
            readonly property bool usable: service.gcsPosition.isValid
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(component.isReady(), TestTimeout::mediumMs());
    QGCPositionManager service;
    std::unique_ptr<QObject> object(
        component.createWithInitialProperties({{QStringLiteral("service"), QVariant::fromValue(&service)}}));
    QVERIFY2(object, qPrintable(component.errorString()));
    SequentialTestDevice device;
    NMEASourceManager input(nullptr, &service);
    input._startDecoder(&device);
    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(object->property("usable").toBool(), TestTimeout::mediumMs());
    QVERIFY(qAbs(object->property("latitude").toDouble() - kExpectedLat) < kCoordEpsilon);
    input.stop();
    QVERIFY(!object->property("usable").toBool());
}

void PositionManagerTest::_destructionDoesNotPublishPosition()
{
    SequentialTestDevice device;
    auto service = std::make_unique<QGCPositionManager>();
    NMEASourceManager input(nullptr, service.get());
    input._startDecoder(&device);
    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(service->gcsPosition().isValid(), TestTimeout::mediumMs());
    bool notified = false;
    connect(service.get(), &QGCPositionManager::gcsPositionChanged, this, [&]() { notified = true; });
    service.reset();
    QVERIFY(!notified);
}

void PositionManagerTest::_deviceDestructionRetiresNmea()
{
    auto device = std::make_unique<SequentialTestDevice>();
    QGCPositionManager service;
    NMEASourceManager input(nullptr, &service);
    input._startDecoder(device.get());
    device->feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(service.gcsPosition().isValid(), TestTimeout::mediumMs());
    device.reset();
    QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::None);
    QVERIFY(!service.gcsPosition().isValid());
}

void PositionManagerTest::_nmeaLifecycleDiagnostics_data()
{
    QTest::addColumn<int>("retirement");
    QTest::addColumn<QString>("reason");
    QTest::newRow("explicit-reset") << 0 << QStringLiteral("stop requested");
    QTest::newRow("device-closed") << 1 << QStringLiteral("device closed");
    QTest::newRow("device-destroyed") << 2 << QStringLiteral("device destroyed");
    QTest::newRow("device-replaced") << 3 << QStringLiteral("device replacement");
    QTest::newRow("device-cleared") << 4 << QStringLiteral("device cleared");
    QTest::newRow("shutdown") << 5 << QStringLiteral("position manager shutdown");
}

void PositionManagerTest::_nmeaLifecycleDiagnostics()
{
    QFETCH(int, retirement);
    QFETCH(QString, reason);
    const QString category = QStringLiteral("GPS.NMEA.NMEASourceManager");
    auto* logging = QGCLoggingCategoryManager::instance();
    const bool wasEnabled = logging->isCategoryEnabled(category);
    if (!wasEnabled) {
        logging->setCategoryEnabled(category, true);
    }
    const auto restoreLogging = qScopeGuard([logging, category, wasEnabled] {
        if (!wasEnabled) {
            logging->setCategoryEnabled(category, false);
        }
    });
    auto scheduler = std::make_unique<ManualScheduler>();
    auto device = std::make_unique<SequentialTestDevice>(scheduler.get());
    SequentialTestDevice replacement(scheduler.get());
    auto service = std::make_unique<QGCPositionManager>(nullptr, scheduler.get());
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("^NMEASourceManager\\(")));
    auto input = std::make_unique<NMEASourceManager>(nullptr, service.get());
    verifyExpectedLogMessage();
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA decoder installed:.*generation: 1.*registered: true")));
    input->_startDecoder(device.get());
    verifyExpectedLogMessage();
    const QPointer<GPSSourceHealth> originalHealth(input->health());
    QVERIFY(originalHealth);
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(
            QStringLiteral("NMEA decoder retired:.*%1.*generation: 2").arg(QRegularExpression::escape(reason))));
    if (retirement == 3) {
        expectLogMessage(
            "GPS.NMEA.NMEASourceManager", QtDebugMsg,
            QRegularExpression(QStringLiteral("NMEA decoder installed:.*generation: 2.*registered: true")));
    }
    switch (retirement) {
        case 0:
            input->stop();
            break;
        case 1:
            device->close();
            break;
        case 2:
            device.reset();
            break;
        case 3:
            input->_startDecoder(&replacement);
            break;
        case 4:
            input->_startDecoder(nullptr);
            break;
        case 5:
            service.reset();
            break;
    }
    QTRY_VERIFY_WITH_TIMEOUT(originalHealth.isNull(), TestTimeout::shortMs());
    verifyExpectedLogMessage();
    if (retirement == 3) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(input->health() != nullptr, retirement == 3);
    if (retirement != 3) {
        const auto logCount = LogManager::capturedMessages(category).size();
        input->stop();
        QCOMPARE(LogManager::capturedMessages(category).size(), logCount);
    }
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("^NMEA source manager shutdown: NMEASourceManager\\(")));
    if (retirement == 3) {
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("^NMEA decoder retired: shutdown generation: 3$")));
    }
    input.reset();
    verifyExpectedLogMessage();
    if (retirement == 3) {
        verifyExpectedLogMessage();
    }
}

void PositionManagerTest::_simulatedPosition_data()
{
    QTest::addColumn<int>("intervalMs");
    QTest::newRow("one-second") << 1000;
    QTest::newRow("two-seconds") << 2000;
}

void PositionManagerTest::_simulatedPosition()
{
    QFETCH(int, intervalMs);
    ManualScheduler scheduler;
    SimulatedPosition source(nullptr, &scheduler);
    GPSPositionService service(nullptr, &scheduler);
    service.setSimulatedPositionSource(&source);
    source.stopUpdates();
    source.setUpdateInterval(intervalMs);
    source.startUpdates();
    const auto origin = source.lastKnownPosition(false).coordinate();
    const auto before = QDateTime::currentDateTimeUtc();
    QSignalSpy positions(&source, &QGeoPositionInfoSource::positionUpdated);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(intervalMs)));
    QCOMPARE(positions.size(), 1);
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Simulated);
    QCOMPARE(service.gcsPosition().type(), QGeoCoordinate::Coordinate3D);
    QVERIFY(qAbs(origin.distanceTo(service.gcsPosition()) - 0.5 * intervalMs / 1000.0) < 0.001);
    QVERIFY(qAbs(service.gcsPosition().altitude() - origin.altitude() - 0.1 * intervalMs / 1000.0) < 0.001);
    const auto timestamp = service.acceptedObservation()->position.timestamp();
    QCOMPARE(timestamp.timeSpec(), Qt::UTC);
    QVERIFY(timestamp >= before);
    QVERIFY(timestamp <= QDateTime::currentDateTimeUtc());

    source.stopUpdates();
    const auto stopped = service.gcsPosition();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(positions.size(), 1);
    QVERIFY(!service.acceptedObservation());
    source.startUpdates();
    source.startUpdates();
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(intervalMs)));
    QCOMPARE(positions.size(), 2);
    QVERIFY(service.acceptedObservation());
    QVERIFY(qAbs(stopped.distanceTo(service.gcsPosition()) - 0.5 * intervalMs / 1000.0) < 0.001);
    connect(&source, &QGeoPositionInfoSource::positionUpdated, &source, &SimulatedPosition::stopUpdates);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(intervalMs)));
    QCOMPARE(positions.size(), 3);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(positions.size(), 3);
}

void PositionManagerTest::_facadeUsesInjectedScheduler()
{
    ManualScheduler scheduler;
    QGCPositionManager manager(nullptr, &scheduler);
    manager.init();
    QSignalSpy reports(&manager, &GPSPositionService::gcsPositionChanged);
    QVERIFY(!manager.acceptedObservation());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QVERIFY(reports.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(reports.size(), 1);
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::Simulated);
    QVERIFY(manager.acceptedObservation());
    QCOMPARE(manager.acceptedObservation()->monotonicTimestampUs, scheduler.nowUs());
    QVERIFY(manager.findChildren<RuntimeScheduler*>().isEmpty());

    QGCPositionManager defaultManager;
    defaultManager.init();
    QCOMPARE(defaultManager.findChildren<RuntimeScheduler*>().size(), 1);
    SequentialTestDevice device;
    NMEASourceManager input(nullptr, &defaultManager);
    input._startDecoder(&device);
    QVERIFY(input.health());
    auto* session = input.health()->parent();
    QVERIFY(session);
    QVERIFY(session->findChildren<RuntimeScheduler*>().isEmpty());
}

void PositionManagerTest::_sourceSettingSelectsMode()
{
    using Mode = GPSPositionService::SourceMode;
    TestFixtures::SettingsFixture saved;
    Fact* const setting = SettingsManager::instance()->autoConnectSettings()->gcsPositionSource();
    saved.setFactValue(setting, static_cast<int>(Mode::NmeaOnly));
    ManualScheduler scheduler;
    QGCPositionManager manager(nullptr, &scheduler);
    QCOMPARE(manager.sourceMode(), Mode::Automatic);
    manager.init();
    QCOMPARE(manager.sourceMode(), Mode::NmeaOnly);
    QCOMPARE(manager.sourceStatus(), GPSPositionService::SourceStatus::NoSource);
    QSignalSpy modes(&manager, &GPSPositionService::sourceModeChanged);
    manager.init();
    setting->setRawValue(static_cast<int>(Mode::InternalOnly));
    QCOMPARE(manager.sourceMode(), Mode::InternalOnly);
    QCOMPARE(modes.size(), 1);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::Simulated);
    setting->setRawValue(static_cast<int>(Mode::Automatic));
    QCOMPARE(manager.sourceMode(), Mode::Automatic);
    QCOMPARE(modes.size(), 2);
}

void PositionManagerTest::_simulatedHomeSelection_data()
{
    QTest::addColumn<bool>("latestAlreadyValid");
    QTest::addColumn<bool>("oldestFirst");
    QTest::newRow("pending-oldest-first") << false << true;
    QTest::newRow("pending-newest-first") << false << false;
    QTest::newRow("valid-oldest-first") << true << true;
    QTest::newRow("valid-newest-first") << true << false;
}

void PositionManagerTest::_simulatedHomeSelection()
{
    QFETCH(bool, latestAlreadyValid);
    QFETCH(bool, oldestFirst);
    ManualScheduler scheduler;
    SimulatedPosition source(nullptr, &scheduler);
    Vehicle first(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle second(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle latest(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    QGeoCoordinate latestHome(48, 9, 550);
    if (latestAlreadyValid) {
        latest._setHomePosition(latestHome);
    }
    const auto origin = source.lastKnownPosition(false).coordinate();
    for (auto* vehicle : {&first, &second, &latest}) {
        QVERIFY(QMetaObject::invokeMethod(&source, "_vehicleAdded", Qt::DirectConnection, Q_ARG(Vehicle*, vehicle)));
    }
    const auto updateOlderHomes = [&]() {
        QGeoCoordinate firstHome(46, 7, 450);
        QGeoCoordinate secondHome(47, 8, 500);
        first._setHomePosition(firstHome);
        second._setHomePosition(secondHome);
    };
    if (oldestFirst) {
        updateOlderHomes();
        QCOMPARE(source.lastKnownPosition(false).coordinate(), latestAlreadyValid ? latestHome : origin);
    }
    latest._setHomePosition(latestHome);
    QCOMPARE(source.lastKnownPosition(false).coordinate(), latestHome);
    if (!oldestFirst) {
        updateOlderHomes();
    }
    QGeoCoordinate changedHome(49, 10, 600);
    for (auto* vehicle : {&first, &second, &latest}) {
        vehicle->_setHomePosition(changedHome);
        QCOMPARE(source.lastKnownPosition(false).coordinate(), latestHome);
    }
}
