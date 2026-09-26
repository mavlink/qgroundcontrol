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
set(QGC_GPS_COMPONENTS_ENABLED NMEA RTCM ReceiverConfig Transport)
if(NOT TARGET QGCGPSCommon)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Common" "${CMAKE_CURRENT_BINARY_DIR}/Common")
endif()
if(NOT TARGET QGCGPSTransport)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Transport" "${CMAKE_CURRENT_BINARY_DIR}/Transport")
endif()
if(NOT TARGET QGCGPSNMEA)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../NMEA" "${CMAKE_CURRENT_BINARY_DIR}/NMEA")
endif()
if(NOT TARGET QGCGPSRTCM)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../RTCM" "${CMAKE_CURRENT_BINARY_DIR}/RTCM")
endif()
if(NOT TARGET QGCGPSReceiverConfig)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../ReceiverConfig" "${CMAKE_CURRENT_BINARY_DIR}/ReceiverConfig")
endif()
endblock()
include("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Geo/GeographicLib.cmake")

# QGCGPSProtocols is one library. The root holds GPSDriver, its public API types and the family table; Runtime/ the
# coroutine protocol runtime and report assembly; Common/ shared family components; and each family folder (Ashtech,
# Femto, Passive, Quectel, SBF, UBX, Unicore) is a source subfolder of it, not a separate target.
set(_protocol_headers
    GPSConfigurationEvidence.h
    GPSDriver.h
    GPSProtocolError.h
    GPSReceiveUpdates.h
    GPSReceiverFamilies.h
    GPSReceiverSettingId.h
    Common/GPSASCIILog.h
    Common/GPSCommandText.h
    Common/GPSProtocolTime.h
    Common/GPSSurveyClock.h
    Common/GPSNMEAReport.h
    Common/GPSNMEAStream.h
    Ashtech/AshtechConfigurator.h
    Ashtech/AshtechDecoder.h
    Ashtech/AshtechFamily.h
    Femto/FemtoConfigurator.h
    Femto/FemtoDecoder.h
    Femto/FemtoFamily.h
    Passive/PassiveFamily.h
    Quectel/QuectelFamily.h
    SBF/Generated/SBFBlocks.h
    SBF/SBFFamily.h
    UBX/Generated/UBXConfigKeys.h
    UBX/Generated/UBXMessageIds.h
    UBX/UBXConfigurationValues.h
    UBX/UBXDecoder.h
    UBX/UBXFamily.h
    UBX/UBXFrameDecoder.h
    UBX/UBXMessageSchema.h
    UBX/UBXNavigationEpoch.h
    UBX/UBXPlan.h
    UBX/UBXReceiverController.h
    UBX/UBXReceiverProfile.h
    Unicore/UnicoreFamily.h
    Runtime/GPSCommandChannel.h
    Runtime/GPSCommandSequence.h
    Runtime/GPSCommandTransaction.h
    Runtime/GPSDeadline.h
    Runtime/GPSDecodedReports.h
    Runtime/GPSEventSink.h
    Runtime/GPSFamilyProtocol.h
    Runtime/GPSFrame.h
    Runtime/GPSProtocolEvent.h
    Runtime/GPSProtocolMath.h
    Runtime/GPSProtocolRuntime.h
    Runtime/GPSRawAckMatcher.h
    Runtime/GPSReceiverDetector.h
    Runtime/GPSReceiverFamily.h
    Runtime/GPSRuntimeIO.h
    Runtime/GPSSatelliteObservation.h
    Runtime/GPSSatelliteState.h
    Runtime/GPSStreamDemux.h
    Runtime/GPSTask.h
)
qt_add_library(
    QGCGPSProtocols
    STATIC
    ${_protocol_headers}
    GPSDriver.cc
    GPSReceiverFamilies.cc
    Common/GPSCommandText.cc
    Common/GPSNMEAStream.cc
    Ashtech/AshtechConfigurator.cc
    Ashtech/AshtechDecoder.cc
    Ashtech/AshtechFamily.cc
    Ashtech/AshtechPlan.cc
    Ashtech/AshtechPlan.h
    Femto/FemtoConfigurator.cc
    Femto/FemtoDecoder.cc
    Femto/FemtoFamily.cc
    Femto/FemtoPlan.cc
    Femto/FemtoPlan.h
    Passive/PassiveFamily.cc
    Quectel/QuectelCodec_p.h
    Quectel/QuectelConfigurator.cc
    Quectel/QuectelConfigurator.h
    Quectel/QuectelDecoder.cc
    Quectel/QuectelDecoder.h
    Quectel/QuectelFamily.cc
    Quectel/QuectelPlan.cc
    Quectel/QuectelPlan.h
    SBF/SBFConfigurator.cc
    SBF/SBFConfigurator.h
    SBF/SBFDecoder.cc
    SBF/SBFDecoder.h
    SBF/SBFFamily.cc
    SBF/SBFPlan.cc
    SBF/SBFPlan.h
    UBX/UBXConfigurator.cc
    UBX/UBXConfigurator.h
    UBX/UBXDecoder.cc
    UBX/UBXFamily.cc
    UBX/UBXPlan.cc
    Unicore/UnicoreConfigurator.cc
    Unicore/UnicoreConfigurator.h
    Unicore/UnicoreDecoder.cc
    Unicore/UnicoreDecoder.h
    Unicore/UnicoreFamily.cc
    Unicore/UnicorePlan.cc
    Unicore/UnicorePlan.h
    Runtime/GPSCommandChannel.cc
    Runtime/GPSDecodedData.cc
    Runtime/GPSDecodedData_p.h
    Runtime/GPSProtocolMath.cc
    Runtime/GPSProtocolRuntime.cc
    Runtime/GPSReceiverDetector.cc
    Runtime/GPSSatelliteObservation.cc
    Runtime/GPSStreamDemux.cc
)
add_library(QGC::GPSProtocols ALIAS QGCGPSProtocols)
target_sources(QGCGPSProtocols PUBLIC FILE_SET HEADERS FILES ${_protocol_headers})
set_target_properties(
    QGCGPSProtocols
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_include_directories(
    QGCGPSProtocols PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Common
                           ${CMAKE_CURRENT_SOURCE_DIR}/Runtime "${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Math"
)
target_link_libraries(
    QGCGPSProtocols
    PUBLIC QGCWire
           QGCClock
           QGCGPSCommon
           QGCGPSNMEA
           QGCGPSReceiverConfig
           QGCGPSRTCM
           QGCGPSTransport
    PRIVATE GeographicLib::GeographicLib QGCLoggingCategory
)
