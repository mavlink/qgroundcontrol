#include "VehicleEFIFactGroupTest.h"

#include "MAVLinkLib.h"
#include "VehicleEFIFactGroup.h"

namespace {

mavlink_efi_status_t validEfiStatus()
{
    mavlink_efi_status_t efi{};
    efi.health = 1;
    efi.rpm = 2200.0F;
    efi.engine_load = 42.0F;
    efi.throttle_position = 38.0F;
    efi.intake_manifold_temperature = 31.0F;
    efi.cylinder_head_temperature = 145.0F;
    efi.exhaust_gas_temperature = 610.0F;
    efi.ignition_voltage = 13.5F;
    efi.fuel_pressure = 250.0F;
    return efi;
}

mavlink_message_t efiStatusMessage(const mavlink_efi_status_t& efi)
{
    mavlink_message_t message{};
    mavlink_msg_efi_status_encode(1, 1, &message, &efi);
    return message;
}

}  // namespace

void VehicleEFIFactGroupTest::_validValuesPassThrough()
{
    VehicleEFIFactGroup efi;
    QVERIFY(!efi.telemetryAvailable());
    QVERIFY(qIsNaN(efi.cylinderTemp()->rawValue().toDouble()));

    efi.handleMessage(nullptr, efiStatusMessage(validEfiStatus()));

    QVERIFY(efi.telemetryAvailable());
    QCOMPARE(efi.rpm()->rawValue().toDouble(), 2200.0);
    QCOMPARE(efi.intakeTemp()->rawValue().toDouble(), 31.0);
    QCOMPARE(efi.cylinderTemp()->rawValue().toDouble(), 145.0);
    QCOMPARE(efi.exGasTemp()->rawValue().toDouble(), 610.0);
    QCOMPARE(efi.ignVoltage()->rawValue().toDouble(), 13.5);
    QCOMPARE(efi.fuelPressure()->rawValue().toDouble(), 250.0);
}

void VehicleEFIFactGroupTest::_unavailableValuesAreNaN_data()
{
    QTest::addColumn<QString>("factName");

    QTest::newRow("intake temperature at 0 K") << QStringLiteral("intakeTemp");
    QTest::newRow("cylinder head temperature at 0 K") << QStringLiteral("cylinderTemp");
    QTest::newRow("exhaust gas temperature at 0 K") << QStringLiteral("exGasTemp");
    QTest::newRow("ignition voltage zero") << QStringLiteral("ignVoltage");
    QTest::newRow("fuel pressure zero") << QStringLiteral("fuelPressure");
}

void VehicleEFIFactGroupTest::_unavailableValuesAreNaN()
{
    QFETCH(QString, factName);

    VehicleEFIFactGroup efi;
    efi.handleMessage(nullptr, efiStatusMessage(validEfiStatus()));

    // ArduPilot sends 0 K for unavailable temperatures; MAVLink defines zero as "unknown" for
    // ignition_voltage and fuel_pressure.
    mavlink_efi_status_t status = validEfiStatus();
    status.intake_manifold_temperature = -273.15F;
    status.cylinder_head_temperature = -273.15F;
    status.exhaust_gas_temperature = -273.15F;
    status.ignition_voltage = 0.0F;
    status.fuel_pressure = 0.0F;
    efi.handleMessage(nullptr, efiStatusMessage(status));

    Fact* const fact = efi.getFact(factName);
    QVERIFY(fact);
    QVERIFY2(qIsNaN(fact->rawValue().toDouble()), qPrintable(factName));
    QCOMPARE(efi.rpm()->rawValue().toDouble(), 2200.0);
}

void VehicleEFIFactGroupTest::_zeroReadingsStayZero()
{
    VehicleEFIFactGroup efi;

    mavlink_efi_status_t status = validEfiStatus();
    status.rpm = 0.0F;
    status.engine_load = 0.0F;
    status.throttle_position = 0.0F;
    status.ignition_voltage = 0.0001F;  // MAVLink: a real zero is sent as 0.0001
    efi.handleMessage(nullptr, efiStatusMessage(status));

    QCOMPARE(efi.rpm()->rawValue().toDouble(), 0.0);
    QCOMPARE(efi.engineLoad()->rawValue().toDouble(), 0.0);
    QCOMPARE(efi.throttlePos()->rawValue().toDouble(), 0.0);
    QVERIFY(!qIsNaN(efi.ignVoltage()->rawValue().toDouble()));
}

UT_REGISTER_TEST(VehicleEFIFactGroupTest, TestLabel::Unit, TestLabel::Vehicle)
