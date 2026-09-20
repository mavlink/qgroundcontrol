qt_add_library(QGCGPSRTCM STATIC RTCMFrameDecoder.cc)
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
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory
)
target_sources(QGCGPSRTCM PUBLIC FILE_SET HEADERS FILES RTCMDecodedFrame.h RTCMFrameDecoder.h RTCMFramer.h)
