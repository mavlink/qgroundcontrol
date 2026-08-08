# ============================================================================
# AutoPilot Plugins Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        APM/APMFreshFlashParamsTest.cc
        APM/APMFreshFlashParamsTest.h
        PX4/AirframeComponentAirframesTest.cc
        PX4/AirframeComponentAirframesTest.h
        PX4/FailureInjectionTest.cc
        PX4/FailureInjectionTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/PX4)

add_qgc_test(AirframeComponentAirframesTest LABELS Unit)
add_qgc_test(APMFreshFlashParamsTest LABELS Integration Vehicle TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(FailureInjectionTest LABELS Unit)
