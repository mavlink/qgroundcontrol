#pragma once

#include "FactSystemTestBase.h"

/// Unit Test for Fact System on PX4 autopilot
class FactSystemTestPX4 : public FactSystemTestBase
{
    Q_OBJECT

public:
    FactSystemTestPX4() = default;

private slots:
    void init() override;
    void cleanup() override { _cleanup(); }

    // Parameter access tests
    void parameter_default_component_id_test() { _parameter_default_component_id_test(); }
    void parameter_specific_component_id_test() { _parameter_specific_component_id_test(); }
    void parameter_not_exists_test() { _parameter_not_exists_test(); }

    // Fact value tests
    void fact_raw_value_test() { _fact_raw_value_test(); }
    void fact_cooked_value_test() { _fact_cooked_value_test(); }
    void fact_value_change_test() { _fact_value_change_test(); }
    void fact_value_equals_default_test() { _fact_value_equals_default_test(); }

    // Fact type tests
    void fact_type_int_test() { _fact_type_int_test(); }
    void fact_type_float_test() { _fact_type_float_test(); }
    void fact_type_is_string_test() { _fact_type_is_string_test(); }
    void fact_type_is_bool_test() { _fact_type_is_bool_test(); }

    // Fact metadata tests
    void fact_name_test() { _fact_name_test(); }
    void fact_component_id_test() { _fact_component_id_test(); }
    void fact_min_max_test() { _fact_min_max_test(); }
    void fact_decimal_places_test() { _fact_decimal_places_test(); }

    // Fact signal tests
    void fact_value_changed_signal_test() { _fact_value_changed_signal_test(); }
    void fact_raw_value_changed_signal_test() { _fact_raw_value_changed_signal_test(); }

    // FactMetaData type tests
    void metadata_value_type_uint8_test() { _metadata_value_type_uint8_test(); }
    void metadata_value_type_int8_test() { _metadata_value_type_int8_test(); }
    void metadata_value_type_uint16_test() { _metadata_value_type_uint16_test(); }
    void metadata_value_type_int16_test() { _metadata_value_type_int16_test(); }
    void metadata_value_type_uint32_test() { _metadata_value_type_uint32_test(); }
    void metadata_value_type_int32_test() { _metadata_value_type_int32_test(); }
    void metadata_value_type_uint64_test() { _metadata_value_type_uint64_test(); }
    void metadata_value_type_int64_test() { _metadata_value_type_int64_test(); }
    void metadata_value_type_float_test() { _metadata_value_type_float_test(); }
    void metadata_value_type_double_test() { _metadata_value_type_double_test(); }
    void metadata_value_type_string_test() { _metadata_value_type_string_test(); }
    void metadata_value_type_bool_test() { _metadata_value_type_bool_test(); }

    // FactMetaData constants tests
    void metadata_default_decimal_places_test() { _metadata_default_decimal_places_test(); }
    void metadata_unknown_decimal_places_test() { _metadata_unknown_decimal_places_test(); }
    void metadata_default_category_test() { _metadata_default_category_test(); }
    void metadata_default_group_test() { _metadata_default_group_test(); }

    // QML tests (skipped)
    void qml_test() { _qml_test(); }
    void qmlUpdate_test() { _qmlUpdate_test(); }
};
