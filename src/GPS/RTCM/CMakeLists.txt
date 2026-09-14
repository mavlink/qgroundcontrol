add_library(QGCGPSRTCMFramer INTERFACE)
add_library(QGC::GPSRTCMFramer ALIAS QGCGPSRTCMFramer)
target_compile_features(QGCGPSRTCMFramer INTERFACE cxx_std_20)
target_sources(QGCGPSRTCMFramer INTERFACE FILE_SET HEADERS FILES RTCMFramer.h)

if(DEFINED QGC_GPS_COMPONENTS_ENABLED AND NOT "RTCM" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    return()
endif()

add_library(QGCGPSRTCM STATIC RTCMFrameDecoder.cc RTCMMavlinkPacket.cc RTCMParser.cc)
add_library(QGC::GPSRTCM ALIAS QGCGPSRTCM)
set_target_properties(
    QGCGPSRTCM
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSRTCM PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSRTCM
    PUBLIC QGCGPSRTCMFramer Qt6::Core
    PRIVATE QGCLoggingCategory
)
target_sources(
    QGCGPSRTCM
    PUBLIC FILE_SET
           HEADERS
           FILES
           RTCMFrame.h
           RTCMFrameDecoder.h
           RTCMMavlinkPacket.h
           RTCMParser.h
)
