qt_add_executable(QGCGPSCorrectionRouterTest GPSCorrectionRouterTest.cc)
set_target_properties(QGCGPSCorrectionRouterTest PROPERTIES AUTOMOC ON)
target_link_libraries(QGCGPSCorrectionRouterTest PRIVATE QGC::GPSCorrections QGC::GPSRTCMFramer Qt6::Test)
add_test(NAME GPSCorrectionRouterTest COMMAND QGCGPSCorrectionRouterTest)
set_tests_properties(GPSCorrectionRouterTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)

if(COMMAND add_qgc_test)
    target_sources(
        ${CMAKE_PROJECT_NAME}
        PRIVATE GPSCorrectionManagerTest.cc
                GPSCorrectionManagerTest.h
                RTCMMavlinkTest.cc
                RTCMMavlinkTest.h
                RTCMUdpInputTest.cc
                RTCMUdpInputTest.h
                GPSMavlinkOutputTest.cc
                GPSMavlinkOutputTest.h
    )
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_qgc_test(GPSCorrectionManagerTest LABELS Unit RESOURCE_LOCK Settings)
    add_qgc_test(RTCMMavlinkTest LABELS Unit)
    add_qgc_test(RTCMUdpInputTest LABELS Unit)
    add_qgc_test(GPSMavlinkOutputTest LABELS Integration Vehicle)
endif()
