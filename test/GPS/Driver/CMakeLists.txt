option(QGC_BUILD_GPS_HARDWARE_TESTS "Build opt-in GPS hardware validation runners" OFF)

add_subdirectory(Protocols)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDriverTest.cc GPSDriverTest.h GPSDriverReentrancyTest.cc
                                             GPSNativeDataTest.cc ScriptedUBXReceiver.cc
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNative)
add_qgc_test(GPSDriverTest LABELS Unit GPS TIMEOUT 240)
add_qgc_test(GPSDriverReentrancyTest LABELS Unit GPS)
add_qgc_test(GPSNativeDataTest LABELS Unit GPS)

# Hardware validation is opt-in and never opens a physical device during CTest.
if(QGC_BUILD_GPS_HARDWARE_TESTS)
    add_subdirectory(Hardware)
endif()
