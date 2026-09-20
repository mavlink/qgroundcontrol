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
