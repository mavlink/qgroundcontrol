#include <array>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <optional>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>
#include <QtTest/QTest>

#include "GPSDriver.h"
#include "GPSProtocolFeatures.h"
#include "GPSTransport.h"
#include "UnitTest.h"

namespace {
const std::atomic_bool NEVER_STOP{false};
const QByteArray POSITION = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
const QByteArray NEXT_POSITION = "$GPGGA,123520,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*4D\r\n";
const QByteArray SATELLITES = "$GPGSV,1,1,01,01,10,20,30*79\r\n";

class ReentrancyTransport : public GPSTransport
{
public:
    ReentrancyTransport()
        : GPSTransport(NEVER_STOP)
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t* buffer, int length, int) override
    {
        ++reads;
        if (incoming.isEmpty()) {
            return {GPSReadStatus::TimedOut};
        }
        const int count = qMin(length, static_cast<int>(incoming.size()));
        std::memcpy(buffer, incoming.constData(), static_cast<size_t>(count));
        incoming.remove(0, count);
        return {GPSReadStatus::Data, count};
    }

    GPSWriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer) override
    {
        ++writes;
        if (acknowledgeFemto) {
            const QByteArray command(reinterpret_cast<const char*>(buffer), length);
            incoming = '<' + command.split(' ').first().trimmed() + " OK" + char(0);
        }
        return {GPSWriteStatus::Completed, length, length};
    }

    bool setBaudrate(unsigned) override
    {
        ++baudChanges;
        return baudOk;
    }

    QByteArray incoming;
    int reads = 0;
    int writes = 0;
    int baudChanges = 0;
    bool baudOk = true;
    bool acknowledgeFemto = false;
};
}  // namespace

class GPSDriverReentrancyTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _factoryCoverage();
    void _recursiveConfiguration_data();
    void _recursiveConfiguration();
    void _recursiveReceive();
    void _configurationCallbacks();
    void _satelliteExpiry_data();
    void _satelliteExpiry();
};

void GPSDriverReentrancyTest::_factoryCoverage()
{
    const std::array expected{
        std::pair{GPSType::ublox, bool(QGC_GPS_ENABLE_UBX)},
        std::pair{GPSType::trimble, bool(QGC_GPS_ENABLE_ASHTECH)},
        std::pair{GPSType::septentrio, bool(QGC_GPS_ENABLE_SBF)},
        std::pair{GPSType::femto, bool(QGC_GPS_ENABLE_FEMTO)},
        std::pair{GPSType::unicore, bool(QGC_GPS_ENABLE_UNICORE)},
        std::pair{GPSType::quectel, bool(QGC_GPS_ENABLE_QUECTEL)},
        std::pair{GPSType::passive, bool(QGC_GPS_ENABLE_PASSIVE)},
    };
    for (const auto& [type, enabled] : expected) {
        QCOMPARE(GPSDriver::supportsType(type), enabled);
    }
    QVERIFY(!GPSDriver::supportsType(static_cast<GPSType>(-1)));
    QVERIFY(!GPSDriver::supportsType(static_cast<GPSType>(255)));
}

void GPSDriverReentrancyTest::_recursiveConfiguration_data()
{
    QTest::addColumn<bool>("failAfterCallback");
    QTest::newRow("valid-later-configuration") << false;
    QTest::newRow("failed-later-configuration") << true;
}

void GPSDriverReentrancyTest::_recursiveConfiguration()
{
    if (!GPSDriver::supportsType(GPSType::passive)) {
        QSKIP("Passive receiver support is disabled");
    }
    QFETCH(bool, failAfterCallback);
    ReentrancyTransport transport;
    GPSDriver* driverPointer = nullptr;
    bool nestedResult = true;
    QString nestedError;
    int positions = 0;
    GPSDriverSinks sinks;
    sinks.onPosition = [&](const GPSPositionReport&) {
        ++positions;
        transport.baudOk = !failAfterCallback;
        nestedResult = driverPointer->configure();
        nestedError = driverPointer->configurationError();
    };
    GPSDriver driver(GPSType::passive, transport, {.role = GPSReceiverConfig::Role::Passive, .baudRate = 115200},
                     std::move(sinks));
    driverPointer = &driver;
    QVERIFY(driver.configure());
    transport.incoming = POSITION;
    expectLogMessage(
        "GPS.GPSDriver", QtWarningMsg,
        QRegularExpression(QStringLiteral("Receiver operation already in progress; configuration rejected")));
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Data);
    verifyExpectedLogMessage();
    QCOMPARE(positions, 1);
    QVERIFY(!nestedResult);
    QVERIFY(nestedError.contains(QStringLiteral("already in progress")));
    QCOMPARE(transport.baudChanges, 1);
    QCOMPARE(transport.reads, 1);
    QCOMPARE(transport.writes, 0);
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Idle);

    if (failAfterCallback) {
        expectLogMessage("GPS.Drivers", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Could not set the passive input baud rate")));
        expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Driver configuration failed for type")));
        QVERIFY(!driver.configure());
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
        QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::NotConfigured);
        transport.baudOk = true;
    }
    QVERIFY(driver.configure());
    QVERIFY(driver.configurationError().isEmpty());
}

void GPSDriverReentrancyTest::_recursiveReceive()
{
    if (!GPSDriver::supportsType(GPSType::passive)) {
        QSKIP("Passive receiver support is disabled");
    }
    ReentrancyTransport transport;
    GPSDriver* driverPointer = nullptr;
    std::optional<GPSReceiveResult> nestedResult;
    GPSDriverSinks sinks;
    sinks.onPosition = [&](const GPSPositionReport&) { nestedResult = driverPointer->receiveOutcome(0); };
    GPSDriver driver(GPSType::passive, transport, {.role = GPSReceiverConfig::Role::Passive, .baudRate = 115200},
                     std::move(sinks));
    driverPointer = &driver;
    QVERIFY(driver.configure());
    transport.incoming = POSITION;
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver operation already in progress; receive rejected")));
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Data);
    verifyExpectedLogMessage();
    QVERIFY(nestedResult);
    QCOMPARE(nestedResult->status, GPSReceiveStatus::Busy);
    QCOMPARE(nestedResult->errorCode, -EBUSY);
    QCOMPARE(nestedResult->updates, 0);
    QVERIFY(!nestedResult->terminal());
    QVERIFY(!nestedResult->detail.isEmpty());
    QCOMPARE(transport.reads, 1);
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Idle);
}

void GPSDriverReentrancyTest::_configurationCallbacks()
{
    if (!GPSDriver::supportsType(GPSType::femto)) {
        QSKIP("Femto receiver support is disabled");
    }
    ReentrancyTransport transport;
    transport.acknowledgeFemto = true;
    GPSDriver* driverPointer = nullptr;
    bool nestedConfiguration = true;
    std::optional<GPSReceiveResult> nestedReceive;
    int writesAtCallback = 0;
    GPSDriverSinks sinks;
    sinks.onSurveyIn = [&](const GPSSurveyReport&) {
        writesAtCallback = transport.writes;
        nestedConfiguration = driverPointer->configure();
        nestedReceive = driverPointer->receiveOutcome(0);
    };
    GPSDriver driver(GPSType::femto, transport,
                     {.base = {.useFixedBase = true,
                               .fixedPosition = {.latitudeDegrees = 0, .longitudeDegrees = 0, .altitudeMeters = 0}}},
                     std::move(sinks));
    driverPointer = &driver;
    expectLogMessage(
        "GPS.GPSDriver", QtWarningMsg,
        QRegularExpression(QStringLiteral("Receiver operation already in progress; configuration rejected")));
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver operation already in progress; receive rejected")));
    QVERIFY(driver.configure());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!nestedConfiguration);
    QVERIFY(nestedReceive);
    QCOMPARE(nestedReceive->status, GPSReceiveStatus::Busy);
    QCOMPARE(transport.writes, writesAtCallback);
    QVERIFY(transport.writes > 0);
    QVERIFY(!driver.configurationEvidence().empty());
    QVERIFY(driver.configurationError().isEmpty());
}

void GPSDriverReentrancyTest::_satelliteExpiry_data()
{
    QTest::addColumn<bool>("positionTraffic");
    QTest::newRow("idle") << false;
    QTest::newRow("position-only-traffic") << true;
}

void GPSDriverReentrancyTest::_satelliteExpiry()
{
    if (!GPSDriver::supportsType(GPSType::passive)) {
        QSKIP("Passive receiver support is disabled");
    }
    QFETCH(bool, positionTraffic);
    ReentrancyTransport transport;
    GPSSatelliteReport latest;
    int unavailableReports = 0;
    GPSDriverSinks sinks;
    sinks.onSatelliteInfo = [&](const GPSSatelliteReport& report) {
        latest = report;
        unavailableReports += report.timestampUs == 0;
    };
    GPSDriver driver(GPSType::passive, transport, {.role = GPSReceiverConfig::Role::Passive, .baudRate = 115200},
                     std::move(sinks));
    QVERIFY(driver.configure());
    transport.incoming = SATELLITES + POSITION;
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Data);
    QCOMPARE(latest.count, uint16_t{1});
    QVERIFY(latest.timestampUs != 0);
    QCOMPARE(unavailableReports, 0);

    GPSReceiveResult result;
    // Exercise the real facade clock across the shared five-second freshness window.
    constexpr int EXPIRY_TIMEOUT_MS = 15000;
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                 transport.incoming = positionTraffic ? POSITION : QByteArray{};
                                 result = driver.receiveOutcome(0);
                                 return unavailableReports != 0;
                             })(),
                             EXPIRY_TIMEOUT_MS);
    QCOMPARE(unavailableReports, 1);
    QCOMPARE(latest.timestampUs, uint64_t{0});
    QCOMPARE(latest.count, uint16_t{0});
    QCOMPARE(result.status, positionTraffic ? GPSReceiveStatus::Data : GPSReceiveStatus::Idle);
    if (!positionTraffic) {
        QCOMPARE(result.updates, 0);
    }
    transport.incoming.clear();
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Idle);
    QCOMPARE(unavailableReports, 1);

    transport.incoming = SATELLITES + NEXT_POSITION;
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Data);
    QCOMPARE(latest.count, uint16_t{1});
    QVERIFY(latest.timestampUs != 0);
    QCOMPARE(transport.writes, 0);
}

UT_REGISTER_TEST(GPSDriverReentrancyTest, TestLabel::Unit)
#include "GPSDriverReentrancyTest.moc"
