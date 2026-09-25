if(NOT TARGET QGCTiming)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Timing" "${CMAKE_CURRENT_BINARY_DIR}/Timing")
endif()

# Correction routing service: UDP input and output around the router, and MAVLink forwarding.
qt_add_library(QGCGPSCorrectionService STATIC GPSCorrectionManager.cc RTCMUdpInput.cc)
add_library(QGC::GPSCorrectionService ALIAS QGCGPSCorrectionService)
set_target_properties(QGCGPSCorrectionService PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCorrectionService PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCorrectionService
    PUBLIC Qt6::Network QGCGPSCorrections QGCGPSRTCMMAVLink QGCNetworkIO QGCTiming
    PRIVATE QGCLoggingCategory
)
target_sources(QGCGPSCorrectionService PUBLIC FILE_SET HEADERS FILES GPSCorrectionManager.h RTCMUdpInput.h)
