if(NOT TARGET Qt6::Core)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core)
endif()
if(NOT TARGET QGCLoggingCategory)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Logging/Category"
                     "${CMAKE_CURRENT_BINARY_DIR}/LoggingCategory"
    )
endif()
if(NOT TARGET QGCWire)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Parsing/Wire" "${CMAKE_CURRENT_BINARY_DIR}/Wire")
endif()
if(NOT TARGET QGCClock)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Timing/Clock" "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()
block(SCOPE_FOR VARIABLES)
set(QGC_GPS_COMPONENTS_ENABLED NMEAProtocol RTCM Receiver Transport)
if(NOT TARGET QGCGPSTransport)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Transport" "${CMAKE_CURRENT_BINARY_DIR}/Transport")
endif()
if(NOT TARGET QGCGPSNMEAProtocol)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../NMEA" "${CMAKE_CURRENT_BINARY_DIR}/NMEA")
endif()
if(NOT TARGET QGCGPSRTCM)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../RTCM" "${CMAKE_CURRENT_BINARY_DIR}/RTCM")
endif()
if(NOT TARGET QGCGPSReceiver)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Receiver" "${CMAKE_CURRENT_BINARY_DIR}/Receiver")
endif()
endblock()
add_subdirectory(Contracts)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Geo/GeographicLib.cmake")

set(_native_headers
    GPSProtocol.h
    GPSProtocolTime.h
    GPSAsciiProtocol.h
    GPSSurveyClock.h
    GPSNMEAReport.h
    Ashtech/GPSDriverAshtech.h
    Femto/FemtoMessages.h
    Femto/GPSDriverFemto.h
    Quectel/GPSDriverQuectel.h
    SBF/GPSDriverSBF.h
    SBF/SBFMessages.h
    UBX/GPSDriverUBX.h
    UBX/UBXConfigurationValues.h
    UBX/UBXFrameDecoder.h
    UBX/UBXMessageCodec.h
    UBX/UBXMessageSchema.h
    UBX/UBXMessages.h
    UBX/UBXNavigationEpoch.h
    UBX/UBXReceiverController.h
    UBX/UBXReceiverProfile.h
    Unicore/GPSDriverUnicore.h
)
qt_add_library(
    QGCGPSNative
    STATIC
    ${_native_headers}
    GPSProtocol.cc
    GPSAsciiProtocol.cc
    Ashtech/AshtechConfiguration.cc
    Ashtech/AshtechDecoder.cc
    Femto/FemtoConfiguration.cc
    Femto/FemtoDecoder.cc
    Quectel/GPSDriverQuectel.cc
    Quectel/QuectelCodec_p.h
    Quectel/QuectelDecoder.cc
    SBF/SBFConfiguration.cc
    SBF/SBFDecoder.cc
    UBX/GPSDriverUBX.cc
    UBX/UBXConfiguration.cc
    UBX/UBXConfiguration_p.h
    UBX/UBXDecoder.cc
    UBX/UBXLegacyConfiguration.cc
    Unicore/GPSDriverUnicore.cc
)
add_library(QGC::GPSNative ALIAS QGCGPSNative)
target_sources(QGCGPSNative PUBLIC FILE_SET HEADERS FILES ${_native_headers})
set_target_properties(
    QGCGPSNative
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_include_directories(QGCGPSNative PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                               "${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Math"
)
target_link_libraries(
    QGCGPSNative
    PUBLIC QGCWire QGCClock QGCGPSNativeContracts QGCGPSNMEAProtocol QGCGPSRTCM
    PRIVATE GeographicLib::GeographicLib QGCLoggingCategory
)
