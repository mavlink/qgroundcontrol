qt_add_library(
    QGCGPSUBX
    STATIC
    GPSDriverUBX.cc
    GPSDriverUBX.h
    UBXConfiguration.cc
    UBXConfigurationValues.h
    UBXDecoder.cc
    UBXFrameDecoder.h
    UBXMessageCodec.h
    UBXMessageSchema.h
    UBXMessages.h
    UBXNavigationEpoch.h
    UBXReceiverController.h
    UBXReceiverProfile.h
)
set_target_properties(
    QGCGPSUBX
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSUBX PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSUBX ALIAS QGCGPSUBX)
