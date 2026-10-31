target_sources(
    QGCUnitTests
    PRIVATE
        AmovUomDeviceAdapterTest.cc
        AmovUomDeviceAdapterTest.h
)
add_qgc_test(AmovUomDeviceAdapterTest LABELS Integration Vehicle UOM)
add_test(
    NAME AmovActivationCodec
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/protocol/amov_device_bridge/test_protocol.py" --cxx
            "${CMAKE_CXX_COMPILER}"
)
set_tests_properties(AmovActivationCodec PROPERTIES LABELS "Unit;UOM;Protocol")
add_test(
    NAME AmovActivationMavlink
    COMMAND
        Python3::Interpreter "${CMAKE_SOURCE_DIR}/protocol/amov_rid_uom/test_protocol.py" --definitions
        "${CPM_PACKAGE_mavlink_SOURCE_DIR}/message_definitions/v1.0" --pymavlink-root
        "${CPM_PACKAGE_mavlink_SOURCE_DIR}" --cc "${CMAKE_C_COMPILER}"
)
set_tests_properties(
    AmovActivationMavlink
    PROPERTIES
        LABELS "Unit;UOM;Protocol"
        TIMEOUT 120
)
