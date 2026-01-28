#include "FactSystemTestBase.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "ParameterManager.h"
#include "AutoPilotPlugin.h"
#include "MAVLinkProtocol.h"
#include "Fact.h"
#include "FactMetaData.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void FactSystemTestBase::_init(MAV_AUTOPILOT autopilot)
{
    UnitTest::init();
    MultiVehicleManager::instance()->init();

    _connectMockLink(autopilot);

    _plugin = MultiVehicleManager::instance()->activeVehicle()->autopilotPlugin();
    Q_ASSERT(_plugin);
}

void FactSystemTestBase::_cleanup()
{
    _disconnectMockLink();
    UnitTest::cleanup();
}

// ============================================================================
// Parameter Access Tests
// ============================================================================

void FactSystemTestBase::_parameter_default_component_id_test()
{
    QCOMPARE_EQ(_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE"), true);
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);
    QVariant factValue = fact->rawValue();
    QCOMPARE_EQ(factValue.isValid(), true);

    QCOMPARE_EQ(factValue.toInt(), 3);
}

void FactSystemTestBase::_parameter_specific_component_id_test()
{
    QCOMPARE_EQ(_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_AUTOPILOT1, "RC_MAP_THROTTLE"), true);
    Fact* fact = _vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);
    QVariant factValue = fact->rawValue();
    QCOMPARE_EQ(factValue.isValid(), true);
    QCOMPARE_EQ(factValue.toInt(), 3);
}

void FactSystemTestBase::_parameter_not_exists_test()
{
    QCOMPARE_EQ(_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "NONEXISTENT_PARAM"), false);
}

// ============================================================================
// Fact Value Tests
// ============================================================================

void FactSystemTestBase::_fact_raw_value_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    QVariant rawValue = fact->rawValue();
    QVERIFY(rawValue.isValid());
    QCOMPARE_EQ(rawValue.toInt(), 3);
}

void FactSystemTestBase::_fact_cooked_value_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    QVariant cookedValue = fact->cookedValue();
    QVERIFY(cookedValue.isValid());
}

void FactSystemTestBase::_fact_value_change_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    int originalValue = fact->rawValue().toInt();

    // Change the value
    fact->setRawValue(originalValue + 1);
    QCOMPARE_EQ(fact->rawValue().toInt(), originalValue + 1);

    // Restore original value
    fact->setRawValue(originalValue);
    QCOMPARE_EQ(fact->rawValue().toInt(), originalValue);
}

void FactSystemTestBase::_fact_value_equals_default_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    // This tests the valueEqualsDefault property - result depends on current value vs default
    // Just verify the method doesn't crash
    (void)fact->valueEqualsDefault();
}

// ============================================================================
// Fact Type Tests
// ============================================================================

void FactSystemTestBase::_fact_type_int_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    // RC_MAP_THROTTLE is an integer type
    FactMetaData::ValueType_t type = fact->type();
    QVERIFY(type == FactMetaData::valueTypeInt8 ||
            type == FactMetaData::valueTypeInt16 ||
            type == FactMetaData::valueTypeInt32 ||
            type == FactMetaData::valueTypeInt64 ||
            type == FactMetaData::valueTypeUint8 ||
            type == FactMetaData::valueTypeUint16 ||
            type == FactMetaData::valueTypeUint32 ||
            type == FactMetaData::valueTypeUint64);
}

void FactSystemTestBase::_fact_type_float_test()
{
    // Find a float parameter if available
    if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "NAV_ACC_RAD")) {
        Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "NAV_ACC_RAD");
        VERIFY_NOT_NULL(fact);

        FactMetaData::ValueType_t type = fact->type();
        QVERIFY(type == FactMetaData::valueTypeFloat || type == FactMetaData::valueTypeDouble);
    }
}

void FactSystemTestBase::_fact_type_is_string_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    // RC_MAP_THROTTLE is not a string type
    QVERIFY(!fact->typeIsString());
}

void FactSystemTestBase::_fact_type_is_bool_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    // RC_MAP_THROTTLE is not a bool type
    QVERIFY(!fact->typeIsBool());
}

// ============================================================================
// Fact Metadata Tests
// ============================================================================

void FactSystemTestBase::_fact_name_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    QCOMPARE(fact->name(), QStringLiteral("RC_MAP_THROTTLE"));
}

void FactSystemTestBase::_fact_component_id_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    QCOMPARE_EQ(fact->componentId(), static_cast<int>(MAV_COMP_ID_AUTOPILOT1));
}

void FactSystemTestBase::_fact_min_max_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    // Verify min/max accessors don't crash
    QVariant minVal = fact->cookedMin();
    QVariant maxVal = fact->cookedMax();

    // Min should be less than or equal to max
    if (minVal.isValid() && maxVal.isValid()) {
        QVERIFY(minVal.toDouble() <= maxVal.toDouble());
    }
}

void FactSystemTestBase::_fact_decimal_places_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    int decimalPlaces = fact->decimalPlaces();
    QVERIFY(decimalPlaces >= 0 || decimalPlaces == FactMetaData::kUnknownDecimalPlaces);
}

// ============================================================================
// Fact Signal Tests
// ============================================================================

void FactSystemTestBase::_fact_value_changed_signal_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    QSignalSpy spy(fact, &Fact::valueChanged);
    QVERIFY(spy.isValid());

    int originalValue = fact->rawValue().toInt();
    fact->setRawValue(originalValue + 1);

    QVERIFY(spy.count() >= 1);

    // Restore original
    fact->setRawValue(originalValue);
}

void FactSystemTestBase::_fact_raw_value_changed_signal_test()
{
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    VERIFY_NOT_NULL(fact);

    QSignalSpy spy(fact, &Fact::rawValueChanged);
    QVERIFY(spy.isValid());

    int originalValue = fact->rawValue().toInt();
    fact->setRawValue(originalValue + 1);

    QVERIFY(spy.count() >= 1);

    // Restore original
    fact->setRawValue(originalValue);
}

// ============================================================================
// FactMetaData Type Tests
// ============================================================================

void FactSystemTestBase::_metadata_value_type_uint8_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeUint8), 0);
}

void FactSystemTestBase::_metadata_value_type_int8_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeInt8), 1);
}

void FactSystemTestBase::_metadata_value_type_uint16_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeUint16), 2);
}

void FactSystemTestBase::_metadata_value_type_int16_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeInt16), 3);
}

void FactSystemTestBase::_metadata_value_type_uint32_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeUint32), 4);
}

void FactSystemTestBase::_metadata_value_type_int32_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeInt32), 5);
}

void FactSystemTestBase::_metadata_value_type_uint64_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeUint64), 6);
}

void FactSystemTestBase::_metadata_value_type_int64_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeInt64), 7);
}

void FactSystemTestBase::_metadata_value_type_float_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeFloat), 8);
}

void FactSystemTestBase::_metadata_value_type_double_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeDouble), 9);
}

void FactSystemTestBase::_metadata_value_type_string_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeString), 10);
}

void FactSystemTestBase::_metadata_value_type_bool_test()
{
    QCOMPARE_EQ(static_cast<int>(FactMetaData::valueTypeBool), 11);
}

// ============================================================================
// FactMetaData Constants Tests
// ============================================================================

void FactSystemTestBase::_metadata_default_decimal_places_test()
{
    QCOMPARE_EQ(FactMetaData::kDefaultDecimalPlaces, 3);
}

void FactSystemTestBase::_metadata_unknown_decimal_places_test()
{
    QCOMPARE_EQ(FactMetaData::kUnknownDecimalPlaces, -1);
}

void FactSystemTestBase::_metadata_default_category_test()
{
    QCOMPARE(FactMetaData::defaultCategory(), QStringLiteral("Other"));
}

void FactSystemTestBase::_metadata_default_group_test()
{
    QCOMPARE(FactMetaData::defaultGroup(), QStringLiteral("Misc"));
}

// ============================================================================
// QML Tests (Skipped)
// ============================================================================

void FactSystemTestBase::_qml_test()
{
    QSKIP("QML widget testing not yet implemented");
}

void FactSystemTestBase::_qmlUpdate_test()
{
    QSKIP("QML widget testing not yet implemented");
}
