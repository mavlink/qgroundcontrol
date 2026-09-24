# Linux test builds, including CI, keep the receiver qualification runner compiling and exercised.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_qgc_gps_hardware_tests_default ON)
else()
    set(_qgc_gps_hardware_tests_default OFF)
endif()
option(QGC_BUILD_GPS_HARDWARE_TESTS "Build the GPS receiver qualification runner and its scripted test"
       ${_qgc_gps_hardware_tests_default}
)

add_subdirectory(Protocols)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDriverTest.cc GPSDriverTest.h GPSDriverReentrancyTest.cc
                                             GPSNativeDataTest.cc ScriptedUBXReceiver.cc
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNative)
add_qgc_test(GPSDriverTest LABELS Unit GPS TIMEOUT 240)
add_qgc_test(GPSDriverReentrancyTest LABELS Unit GPS)
add_qgc_test(GPSNativeDataTest LABELS Unit GPS)

# The runner opens a physical receiver only when invoked manually; its CTest uses a scripted receiver.
if(QGC_BUILD_GPS_HARDWARE_TESTS)
    add_subdirectory(Hardware)
endif()
