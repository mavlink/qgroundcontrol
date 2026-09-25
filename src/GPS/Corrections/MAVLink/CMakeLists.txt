qt_add_library(QGCGPSMavlinkPacket STATIC RTCMMavlinkPacket.cc)
add_library(QGC::GPSMavlinkPacket ALIAS QGCGPSMavlinkPacket)
set_target_properties(
    QGCGPSMavlinkPacket
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSMavlinkPacket PUBLIC cxx_std_20)
target_link_libraries(QGCGPSMavlinkPacket PUBLIC Qt6::Core)
target_sources(QGCGPSMavlinkPacket PUBLIC FILE_SET HEADERS FILES RTCMMavlinkPacket.h)

qt_add_library(QGCGPSRTCMMavlink STATIC RTCMMavlink.cc)
add_library(QGC::GPSRTCMMavlink ALIAS QGCGPSRTCMMavlink)
set_target_properties(QGCGPSRTCMMavlink PROPERTIES AUTOMOC ON POSITION_INDEPENDENT_CODE ON)
target_compile_features(QGCGPSRTCMMavlink PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSRTCMMavlink
    PUBLIC Qt6::Core QGCGPSMavlinkPacket QGCIO
    PRIVATE QGCLoggingCategory
)
target_sources(QGCGPSRTCMMavlink PUBLIC FILE_SET HEADERS FILES RTCMMavlink.h)
