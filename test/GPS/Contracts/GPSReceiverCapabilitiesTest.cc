#include "GPSReceiverCapabilitiesTest.h"

#include <algorithm>

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

void GPSReceiverCapabilitiesTest::_settingDescriptors()
{
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    auto ubx = GPSReceiverCapabilities::forType(GPSType::u_blox);
    const auto descriptors = ubx.settings();
    QCOMPARE(descriptors.size(), 4);
    for (const auto& descriptor : descriptors) {
        QVERIFY(descriptor.requiresReconnect);
        QCOMPARE(descriptor.values.size(), descriptor.labels.size());
        if (descriptor.id == GPSReceiverSetting::ConstellationMask) {
            QCOMPARE(descriptor.requiredMask, 1);
        }
    }
    GPSReceiverConfig distinct;
    distinct.constellationMask = 9;
    distinct.dynamicModel = 4;
    distinct.outputRateHz = 2;
    distinct.headingOffsetDeg = 12;
    auto reordered = descriptors;
    std::reverse(reordered.begin(), reordered.end());
    for (const auto& descriptor : reordered) {
        const double value = GPSReceiverSettings::value(descriptor.id, distinct);
        switch (descriptor.id) {
            case GPSReceiverSetting::ConstellationMask:
                QCOMPARE(value, 9.0);
                QCOMPARE(GPSReceiverSettings::key(descriptor.id), QStringLiteral("constellationMask"));
                break;
            case GPSReceiverSetting::DynamicModel:
                QCOMPARE(value, 4.0);
                QCOMPARE(GPSReceiverSettings::key(descriptor.id), QStringLiteral("dynamicModel"));
                break;
            case GPSReceiverSetting::OutputRateHz:
                QCOMPARE(value, 2.0);
                QCOMPARE(GPSReceiverSettings::key(descriptor.id), QStringLiteral("outputRateHz"));
                break;
            case GPSReceiverSetting::HeadingOffsetDeg:
                QCOMPARE(value, 12.0);
                QCOMPARE(GPSReceiverSettings::key(descriptor.id), QStringLiteral("headingOffsetDeg"));
                break;
            case GPSReceiverSetting::Unknown:
                QFAIL("Descriptor must identify a known setting");
        }
    }
    config.constellationMask = 5;
    config.dynamicModel = 4;
    config.outputRateHz = 5;
    QVERIFY(ubx.validationError(config).isEmpty());
    QVERIFY(!GPSReceiverCapabilities::forType(GPSType::femto).validationError(config).isEmpty());
    QVERIFY(!GPSReceiverCapabilities::forType(GPSType::septentrio).validationError(config).isEmpty());
    ubx.outputRateSelection = GPSReceiverCapabilities::Support::Unsupported;
    QVERIFY(!ubx.validationError(config).isEmpty());
    config.outputRateHz = 0;
    config.constellationMask = 2;
    QVERIFY(!ubx.validationError(config).isEmpty());
    config.constellationMask = 0;
    config.dynamicModel = 1;
    QVERIFY(!ubx.validationError(config).isEmpty());
    config.dynamicModel = 0;
    config.headingOffsetDeg = 12;
    QVERIFY(!ubx.validationError(config).isEmpty());
    QVERIFY(GPSReceiverCapabilities::forType(GPSType::septentrio).validationError(config).isEmpty());
    config.role = GPSReceiverConfig::Role::RTKBase;
    QVERIFY(!GPSReceiverCapabilities::forType(GPSType::septentrio).validationError(config).isEmpty());
}
