# ============================================================================
# RTK Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSRTKFactGroupTest.cc
        GPSRTKFactGroupTest.h
        MockGPSRtk.h
        RTCMMavlinkTest.cc
        RTCMMavlinkTest.h
        RTCMPipelineTest.cc
        RTCMPipelineTest.h
        RTCMRouterTest.cc
        RTCMRouterTest.h
        RTKEndToEndTest.cc
        RTKEndToEndTest.h
        RTKIntegrationTest.cc
        RTKIntegrationTest.h
        RTKSatelliteModelTest.cc
        RTKSatelliteModelTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
