target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSBaseStationConfigTest.cc
            BlockedTransportGate.h
            GPSProviderTest.cc
            GPSProviderTest.h
            GPSRtkTest.cc
            GPSRtkTest.h
            GPSReceiverSettingsTest.cc
            GPSReceiverSettingsTest.h
            ScriptedProvider.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSProviderTest LABELS Unit GPS)
add_qgc_test(GPSBaseStationConfigTest LABELS Unit GPS RTK)
add_qgc_test(GPSRtkTest LABELS Unit GPS)
add_qgc_test(GPSReceiverSettingsTest LABELS Unit GPS RTK)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTKConnectionPolicyTest.cc RTKConnectionPolicyTest.h)
    add_qgc_test(RTKConnectionPolicyTest LABELS Unit GPS)
endif()
