qt_add_library(QGCGPSRTCMMavlink STATIC RTCMMavlink.cc RTCMMavlinkPacket.cc)
add_library(QGC::GPSRTCMMavlink ALIAS QGCGPSRTCMMavlink)
set_target_properties(QGCGPSRTCMMavlink PROPERTIES AUTOMOC ON POSITION_INDEPENDENT_CODE ON)
target_compile_features(QGCGPSRTCMMavlink PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSRTCMMavlink
    PUBLIC Qt6::Core QGCIO
    PRIVATE QGCLoggingCategory
)
target_sources(QGCGPSRTCMMavlink PUBLIC FILE_SET HEADERS FILES RTCMMavlink.h RTCMMavlinkPacket.h)
