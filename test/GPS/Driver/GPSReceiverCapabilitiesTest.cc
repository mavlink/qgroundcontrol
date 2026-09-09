#include "GPSReceiverCapabilitiesTest.h"

#include "GPSDriver.h"
#include "GPSReceiverCapabilities.h"

void GPSReceiverCapabilitiesTest::_familyResolution_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("manufacturer");
    QTest::newRow("usb-ublox") << QStringLiteral("u-blox GNSS receiver") << 0 << 4;
    QTest::newRow("ubx") << QStringLiteral("UBX") << 0 << 4;
    QTest::newRow("trimble") << QStringLiteral("Trimble GPS") << 1 << 1;
    QTest::newRow("ashtech") << QStringLiteral("Ashtech") << 1 << 1;
    QTest::newRow("septentrio") << QStringLiteral("SEPTENTRIO") << 2 << 2;
    QTest::newRow("femtomes") << QStringLiteral("Femtomes") << 3 << 3;
    QTest::newRow("unrecognized") << QStringLiteral("Other GNSS receiver") << -1 << -1;
    QTest::newRow("empty") << QString() << -1 << -1;
}

void GPSReceiverCapabilitiesTest::_familyResolution()
{
    QFETCH(QString, name);
    QFETCH(int, type);
    QFETCH(int, manufacturer);
    const auto resolved = GPSReceiverCapabilities::typeForName(name);
    QCOMPARE(resolved.has_value(), type >= 0);
    if (resolved) {
        QCOMPARE(static_cast<int>(*resolved), type);
        const auto capabilities = GPSReceiverCapabilities::forType(*resolved);
        QVERIFY(capabilities.recognized());
        QCOMPARE(capabilities.manufacturerId, manufacturer);
        QVERIFY(!capabilities.name.isEmpty());
    }
}

void GPSReceiverCapabilitiesTest::_configurationSupport_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("protocol");
    QTest::addColumn<bool>("supported");
    QTest::newRow("unknown-ublox-model-can-be-probed") << 0 << 0 << 0 << true;
    QTest::newRow("ublox-nmea-position") << 0 << 1 << 1 << true;
    QTest::newRow("nmea-base-invalid") << 0 << 0 << 1 << false;
    QTest::newRow("trimble-nmea-not-configurable") << 1 << 1 << 1 << false;
    QTest::newRow("septentrio-position") << 2 << 1 << 0 << true;
    QTest::newRow("femto-base") << 3 << 0 << 0 << true;
    QTest::newRow("invalid-type") << 4 << 0 << 0 << false;
    QTest::newRow("invalid-role") << 0 << 2 << 0 << false;
    QTest::newRow("invalid-protocol") << 0 << 1 << 2 << false;
}

void GPSReceiverCapabilitiesTest::_configurationSupport()
{
    QFETCH(int, type);
    QFETCH(int, role);
    QFETCH(int, protocol);
    QFETCH(bool, supported);
    GPSReceiverConfig config;
    config.role = static_cast<GPSReceiverConfig::Role>(role);
    config.outputProtocol = static_cast<GPSReceiverConfig::OutputProtocol>(protocol);
    QCOMPARE(GPSReceiverCapabilities::forType(static_cast<GPSType>(type)).validationError(config).isEmpty(), supported);
}

void GPSReceiverCapabilitiesTest::_detectedCapabilities()
{
    auto capabilities = GPSReceiverCapabilities::forType(GPSType::u_blox);
    QCOMPARE(capabilities.rtkBase, GPSReceiverCapabilities::Support::Unknown);
    GPSReceiverConfig config;
    capabilities.model = QStringLiteral("NEO-M9N");
    capabilities.rtkBase = GPSReceiverCapabilities::Support::Unsupported;
    QVERIFY(!capabilities.validationError(config).isEmpty());
    config.role = GPSReceiverConfig::Role::Position;
    QVERIFY(capabilities.validationError(config).isEmpty());
    config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
    QVERIFY(capabilities.validationError(config).isEmpty());
    capabilities.model = QStringLiteral("ZED-F9P");
    capabilities.rtkBase = GPSReceiverCapabilities::Support::Supported;
    config.role = GPSReceiverConfig::Role::RTKBase;
    config.outputProtocol = GPSReceiverConfig::OutputProtocol::Native;
    QVERIFY(capabilities.validationError(config).isEmpty());
}

UT_REGISTER_TEST(GPSReceiverCapabilitiesTest, TestLabel::Unit)
