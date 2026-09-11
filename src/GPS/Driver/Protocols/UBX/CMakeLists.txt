add_library(
    QGCGPSUBX STATIC
    GPSDriverUBX.cc
    GPSDriverUBX.h
    UBXConfiguration.cc
    UBXDecoder.cc
    UBXMessageSchema.h
    UBXMessages.h
    UBXNavigationEpoch.h
    UBXPrivate.h
    UBXReceiverProfile.h
    UBXWire.h
)
set_target_properties(
    QGCGPSUBX
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSUBX PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSUBX ALIAS QGCGPSUBX)
