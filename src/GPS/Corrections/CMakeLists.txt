add_library(
    QGCGPSCorrections STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/RTCMMavlinkPacket.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RTCMMavlinkPacket.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionRouter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionRouter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionFrame.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionDiagnostics.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionDiagnostics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionEventModel.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionEventModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RTCMFrame.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RTCMFramer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RTCMFrameDecoder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RTCMFrameDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionSourceRegistration.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSCorrectionSourceRegistration.h
)
add_library(QGC::GPSCorrections ALIAS QGCGPSCorrections)
set_target_properties(QGCGPSCorrections PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCorrections PUBLIC cxx_std_20)
target_include_directories(QGCGPSCorrections PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSCorrections
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory
)
target_include_directories(QGCGPSCorrections PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Math")
