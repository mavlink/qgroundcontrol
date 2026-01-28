#pragma once

#include "TestFixtures.h"

class AutoPilotPlugin;

/// Base class for FactSystemTest[PX4|Generic] unit tests.
/// Tests Fact, FactMetaData, and ParameterManager functionality.
class FactSystemTestBase : public UnitTest
{
    Q_OBJECT

public:
    FactSystemTestBase() = default;

protected:
    void _init(MAV_AUTOPILOT autopilot);
    void _cleanup();

    // Parameter access tests
    void _parameter_default_component_id_test();
    void _parameter_specific_component_id_test();
    void _parameter_not_exists_test();

    // Fact value tests
    void _fact_raw_value_test();
    void _fact_cooked_value_test();
    void _fact_value_change_test();
    void _fact_value_equals_default_test();

    // Fact type tests
    void _fact_type_int_test();
    void _fact_type_float_test();
    void _fact_type_is_string_test();
    void _fact_type_is_bool_test();

    // Fact metadata tests
    void _fact_name_test();
    void _fact_component_id_test();
    void _fact_min_max_test();
    void _fact_decimal_places_test();

    // Fact signal tests
    void _fact_value_changed_signal_test();
    void _fact_raw_value_changed_signal_test();

    // FactMetaData type tests
    void _metadata_value_type_uint8_test();
    void _metadata_value_type_int8_test();
    void _metadata_value_type_uint16_test();
    void _metadata_value_type_int16_test();
    void _metadata_value_type_uint32_test();
    void _metadata_value_type_int32_test();
    void _metadata_value_type_uint64_test();
    void _metadata_value_type_int64_test();
    void _metadata_value_type_float_test();
    void _metadata_value_type_double_test();
    void _metadata_value_type_string_test();
    void _metadata_value_type_bool_test();

    // FactMetaData constants tests
    void _metadata_default_decimal_places_test();
    void _metadata_unknown_decimal_places_test();
    void _metadata_default_category_test();
    void _metadata_default_group_test();

    // QML tests (skipped)
    void _qml_test();
    void _qmlUpdate_test();

    AutoPilotPlugin* _plugin = nullptr;
};
