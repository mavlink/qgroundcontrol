target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSCorrectionManager.cc
            GPSCorrectionManager.h
            GPSMavlinkOutput.cc
            GPSMavlinkOutput.h
            RTCMMavlink.cc
            RTCMMavlink.h
            RTCMUdpInput.cc
            RTCMUdpInput.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSCorrections QGCNetworkIO)
