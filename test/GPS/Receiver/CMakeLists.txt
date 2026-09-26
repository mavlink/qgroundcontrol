target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSBaseStationConfigTest.cc
            BlockedTransportGate.h
            GPSProviderTest.cc
            GPSProviderTest.h
            GPSReceiverTest.cc
            GPSReceiverTest.h
            ScriptedProvider.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# The tests script receivers through the driver and open real receiver transports, which QGCGPSReceiver keeps private.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSDriver QGCGPSReceiverTransports)

add_qgc_test(GPSProviderTest LABELS Unit GPS)
add_qgc_test(GPSBaseStationConfigTest LABELS Unit GPS Receiver)
add_qgc_test(GPSReceiverTest LABELS Unit GPS)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverConnectionPolicyTest.cc GPSReceiverConnectionPolicyTest.h)
    add_qgc_test(GPSReceiverConnectionPolicyTest LABELS Unit GPS)
endif()
