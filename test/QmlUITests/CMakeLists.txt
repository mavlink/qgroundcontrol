# ============================================================================
# QML UI Smoke Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlUITestBase.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlUITestBase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TopLevelViewsTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/TopLevelViewsTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4VehicleConfigUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4VehicleConfigUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/APMVehicleConfigUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/APMVehicleConfigUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ToolbarIndicatorUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ToolbarIndicatorUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PlanViewUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PlanViewUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4SensorsCalibrationUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4SensorsCalibrationUITest.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_qgc_test(TopLevelViewsTest LABELS Integration NoSanitizer TIMEOUT 120)
add_qgc_test(PX4VehicleConfigUITest LABELS Integration NoSanitizer TIMEOUT 180)
add_qgc_test(APMVehicleConfigUITest LABELS Integration NoSanitizer TIMEOUT 360)
add_qgc_test(ToolbarIndicatorUITest LABELS Integration NoSanitizer TIMEOUT 180)
add_qgc_test(PlanViewUITest LABELS Integration NoSanitizer TIMEOUT 120)
add_qgc_test(PX4SensorsCalibrationUITest LABELS Integration NoSanitizer TIMEOUT 180)
