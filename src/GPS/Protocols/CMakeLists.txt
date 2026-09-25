if(NOT TARGET Qt6::Core)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core)
endif()
if(NOT TARGET QGCLoggingCategory)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Logging/Category"
                     "${CMAKE_CURRENT_BINARY_DIR}/LoggingCategory"
    )
endif()
if(NOT TARGET QGCWire)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Parsing/Wire" "${CMAKE_CURRENT_BINARY_DIR}/Wire")
endif()
if(NOT TARGET QGCClock)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing/Clock" "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()
block(SCOPE_FOR VARIABLES)
set(QGC_GPS_COMPONENTS_ENABLED NMEAProtocol RTCM Receiver Transport)
if(NOT TARGET QGCGPSCommon)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Common" "${CMAKE_CURRENT_BINARY_DIR}/Common")
endif()
if(NOT TARGET QGCGPSTransport)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Transport" "${CMAKE_CURRENT_BINARY_DIR}/Transport")
endif()
if(NOT TARGET QGCGPSNMEAProtocol)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../NMEA" "${CMAKE_CURRENT_BINARY_DIR}/NMEA")
endif()
if(NOT TARGET QGCGPSRTCM)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../RTCM" "${CMAKE_CURRENT_BINARY_DIR}/RTCM")
endif()
if(NOT TARGET QGCGPSReceiver)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Receiver" "${CMAKE_CURRENT_BINARY_DIR}/Receiver")
endif()
endblock()
add_subdirectory(Contracts)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Geo/GeographicLib.cmake")

# QGCGPSProtocols is one library: Common/ holds the shared protocol base and each family folder
# (Ashtech, Femto, Quectel, SBF, UBX, Unicore) is a source subfolder of it, not a separate target.
set(_protocol_headers
    Common/GPSProtocol.h
    Common/GPSProtocolTime.h
    Common/GPSAsciiProtocol.h
    Common/GPSConfigurationSequence.h
    Common/GPSSurveyClock.h
    Common/GPSNMEAReport.h
    Ashtech/AshtechProtocol.h
    Femto/FemtoMessages.h
    Femto/FemtoProtocol.h
    Quectel/QuectelProtocol.h
    SBF/SBFMessages.h
    SBF/SBFProtocol.h
    UBX/UBXConfigurationValues.h
    UBX/UBXFrameDecoder.h
    UBX/UBXMessageCodec.h
    UBX/UBXMessageSchema.h
    UBX/UBXMessages.h
    UBX/UBXNavigationEpoch.h
    UBX/UBXProtocol.h
    UBX/UBXReceiverController.h
    UBX/UBXReceiverProfile.h
    Unicore/UnicoreProtocol.h
)
qt_add_library(
    QGCGPSProtocols
    STATIC
    ${_protocol_headers}
    Common/GPSProtocol.cc
    Common/GPSAsciiProtocol.cc
    Ashtech/AshtechConfiguration.cc
    Ashtech/AshtechDecoder.cc
    Femto/FemtoConfiguration.cc
    Femto/FemtoDecoder.cc
    Quectel/QuectelCodec_p.h
    Quectel/QuectelConfiguration.cc
    Quectel/QuectelDecoder.cc
    Quectel/QuectelProtocol.cc
    SBF/SBFConfiguration.cc
    SBF/SBFDecoder.cc
    UBX/UBXConfiguration.cc
    UBX/UBXConfiguration_p.h
    UBX/UBXDecoder.cc
    UBX/UBXLegacyConfiguration.cc
    UBX/UBXProtocol.cc
    Unicore/UnicoreConfiguration.cc
    Unicore/UnicoreDecoder.cc
    Unicore/UnicoreProtocol.cc
)
add_library(QGC::GPSProtocols ALIAS QGCGPSProtocols)
target_sources(QGCGPSProtocols PUBLIC FILE_SET HEADERS FILES ${_protocol_headers})
set_target_properties(
    QGCGPSProtocols
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_include_directories(QGCGPSProtocols PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Common
                                                  "${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Math"
)
target_link_libraries(
    QGCGPSProtocols
    PUBLIC QGCWire
           QGCClock
           QGCGPSCommon
           QGCGPSProtocolContracts
           QGCGPSNMEAProtocol
           QGCGPSReceiver
           QGCGPSRTCM
    PRIVATE GeographicLib::GeographicLib QGCLoggingCategory
)
