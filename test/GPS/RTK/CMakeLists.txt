target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSBaseStationConfigTest.cc
            GPSProviderTest.cc
            GPSProviderTest.h
            GPSRtkTest.cc
            GPSRtkTest.h
            GPSReceiverSettingsTest.cc
            GPSReceiverSettingsTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSProviderTest LABELS Unit)
add_qgc_test(GPSBaseStationConfigTest LABELS Unit GPS RTK)
add_qgc_test(GPSRtkTest LABELS Unit RESOURCE_LOCK Settings)
add_qgc_test(
    GPSReceiverSettingsTest
    LABELS
    Unit
    GPS
    RTK
    RESOURCE_LOCK
    Settings
)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTKAutoConnectTest.cc RTKAutoConnectTest.h)
    add_qgc_test(RTKAutoConnectTest LABELS Unit RESOURCE_LOCK Settings)
endif()
