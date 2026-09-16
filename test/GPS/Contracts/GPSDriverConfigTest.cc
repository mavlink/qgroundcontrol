#include "GPSDriverConfigTest.h"

#include <atomic>

#include <QtCore/QRegularExpression>

#include "GPSDriver.h"
#include "GPSTransport.h"

Q_DECLARE_METATYPE(GPSReceiverConfig)

namespace {
class UnusedTransport : public GPSTransport
{
public:
    explicit UnusedTransport(const std::atomic_bool& stop)
        : GPSTransport(stop)
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Error}; }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t*, int, int) override
    {
        ++operations;
        return {GPSReadStatus::Error};
    }

    GPSWriteResult write(const uint8_t*, int) override
    {
        ++operations;
        return {GPSWriteStatus::Error};
    }

    bool setBaudrate(unsigned) override
    {
        ++operations;
        return false;
    }

    int operations = 0;
};
}  // namespace

void GPSDriverConfigTest::_rejectUnsupportedConfiguration_data()
{
    QTest::addColumn<GPSReceiverConfig>("config");
    const GPSReceiverConfig base{.base = {.surveyInAccMeters = 1, .surveyInDurationSecs = 60}};
    auto config = base;
    config.role = GPSReceiverConfig::Role::Position;
    QTest::newRow("position") << config;
    config = base;
    config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
    QTest::newRow("nmea") << config;
    config = base;
    config.constellationMask = 1;
    QTest::newRow("constellations") << config;
    config = base;
    config.dynamicModel = 4;
    QTest::newRow("dynamic-model") << config;
    config = base;
    config.outputRateHz = 5;
    QTest::newRow("output-rate") << config;
    config = base;
    config.headingOffsetDeg = 12;
    QTest::newRow("heading-offset") << config;
    config.headingOffsetDeg = qQNaN();
    QTest::newRow("unknown-heading") << config;
}

void GPSDriverConfigTest::_rejectUnsupportedConfiguration()
{
    QFETCH(GPSReceiverConfig, config);
    const std::atomic_bool stop = false;
    UnusedTransport transport(stop);
    GPSDriver driver(GPSType::ublox, transport, config, {});
    expectLogMessage(
        "GPS.GPSDriver", QtWarningMsg,
        QRegularExpression(QStringLiteral("RTK driver does not support the requested receiver configuration")));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(transport.operations, 0);
}

UT_REGISTER_TEST(GPSDriverConfigTest, TestLabel::Unit)
