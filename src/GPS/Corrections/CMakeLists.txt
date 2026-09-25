if(NOT TARGET QGCClock)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing/Clock" "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()
if(NOT TARGET QGCTiming)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing" "${CMAKE_CURRENT_BINARY_DIR}/Timing")
endif()

qt_add_library(QGCGPSCorrections STATIC GPSCorrectionEventModel.cc GPSCorrectionLedger.cc GPSCorrectionRouter.cc
               GPSCorrectionSourceRegistration.cc
)
add_library(QGC::GPSCorrections ALIAS QGCGPSCorrections)
set_target_properties(QGCGPSCorrections PROPERTIES AUTOMOC ON POSITION_INDEPENDENT_CODE ON)
target_compile_features(QGCGPSCorrections PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCorrections
    PUBLIC Qt6::Core QGCGPSNotifications QGCGPSRTCM QGCClock
    PRIVATE QGCLoggingCategory
)
target_sources(
    QGCGPSCorrections
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSCorrectionDiagnostics.h
           GPSCorrectionDiagnosticsModel.h
           GPSCorrectionEventModel.h
           GPSCorrectionFrame.h
           GPSCorrectionLedger.h
           GPSCorrectionRouter.h
           GPSCorrectionSelector.h
           GPSCorrectionSourceRegistration.h
)

# Correction routing service: UDP input and output around the router, and MAVLink forwarding.
qt_add_library(QGCGPSCorrectionService STATIC GPSCorrectionManager.cc RTCMUdpInput.cc)
add_library(QGC::GPSCorrectionService ALIAS QGCGPSCorrectionService)
set_target_properties(QGCGPSCorrectionService PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCorrectionService PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCorrectionService
    PUBLIC Qt6::Network QGCGPSCorrections QGCGPSRTCMMavlink QGCNetworkIO QGCTiming
    PRIVATE QGCLoggingCategory
)
target_sources(QGCGPSCorrectionService PUBLIC FILE_SET HEADERS FILES GPSCorrectionManager.h RTCMUdpInput.h)
