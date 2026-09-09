# ============================================================================
# RTCM Module
# RTCM3 framing/parsing, UDP correction input, and MAVLink injection.
# UDP socket helpers live in Utilities/Network.
# ============================================================================

qt_add_library(
    QGCGPSCorrections
    STATIC
    GPSCorrectionRouter.cc
    GPSCorrectionRouter.h
    GPSCorrectionFrame.h
    RTCMParser.cc
    RTCMParser.h
)
target_include_directories(QGCGPSCorrections PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSCorrections
    PUBLIC Qt6::Core
    PRIVATE QGCLogging
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSCorrectionFrame.h RTCMMavlink.cc RTCMMavlink.h RTCMUdpInput.cc
                                             RTCMUdpInput.h
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core QGCGPSCorrections)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
