include_guard(GLOBAL)

get_filename_component(QGC_GPS_REPOSITORY "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(QGC_GPS_PARSER_SOURCE "${QGC_GPS_REPOSITORY}/src/GPS")

# Build the actual GPS parser sources without application or QML dependencies.
function(qgc_add_gps_parser_runtime target)
    get_filename_component(gps_repository "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../.." ABSOLUTE)
    set(gps_parser_source "${gps_repository}/src/GPS")
    add_library(
        ${target} STATIC
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GPSParserLogging.cc"
        "${gps_parser_source}/Core/GPSObservation.cc"
        "${gps_parser_source}/NMEA/NMEAUtils.cc"
        "${gps_parser_source}/NMEA/NMEAStreamSplitter.cc"
        "${gps_parser_source}/NMEA/NMEAStreamSplitter.h"
        "${gps_parser_source}/NTRIP/NTRIPError.h"
        "${gps_parser_source}/NTRIP/NTRIPHttpDecoder.cc"
        "${gps_parser_source}/NTRIP/NTRIPSession.cc"
        "${gps_parser_source}/NTRIP/NTRIPSession.h"
        "${gps_parser_source}/NTRIP/NTRIPStream.cc"
        "${gps_parser_source}/NTRIP/NTRIPStream.h"
        "${gps_parser_source}/NTRIP/NTRIPTransportConfig.cc"
        "${gps_parser_source}/NTRIP/NTRIPRequest.cc"
        "${gps_parser_source}/NTRIP/NTRIPRequest.h"
        "${gps_parser_source}/Corrections/RTCMParser.cc"
        "${gps_parser_source}/Corrections/RTCMFrameDecoder.cc"
        "${gps_parser_source}/Corrections/RTCMFrameDecoder.h"
        "${gps_parser_source}/Corrections/GPSCorrectionDiagnostics.cc"
        "${gps_parser_source}/Corrections/GPSCorrectionDiagnostics.h"
        "${gps_parser_source}/Corrections/GPSCorrectionRouter.cc"
        "${gps_parser_source}/Corrections/GPSCorrectionRouter.h"
    )
    set_target_properties(${target} PROPERTIES AUTOMOC ON)
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_include_directories(
        ${target} PUBLIC "${gps_parser_source}/Core" "${gps_parser_source}/NMEA" "${gps_parser_source}/NTRIP"
                         "${gps_parser_source}/Corrections" "${gps_repository}/src/Utilities/Logging"
    )
    target_link_libraries(${target} PUBLIC Qt6::Core Qt6::Positioning)
endfunction()

# Exercise the production facade and native implementations with deterministic test time.
function(qgc_add_gps_replay_driver target parser_target)
    set(driver_source "${QGC_GPS_PARSER_SOURCE}/Driver")
    file(GLOB native_sources CONFIGURE_DEPENDS "${driver_source}/PX4/*.cpp")
    add_library(
        ${target} STATIC
        "${driver_source}/GPSDriver.cc"
        "${driver_source}/GPSDriverBackend.cc"
        "${driver_source}/GPSDriverData.cc"
        "${driver_source}/GPSReceiverConfig.cc"
        "${driver_source}/GPSReceiverCapabilities.cc"
        "${driver_source}/GPSTransport.cc"
        ${native_sources}
    )
    target_include_directories(${target} PUBLIC "${driver_source}" "${driver_source}/PX4")
    target_compile_definitions(${target}
                               PUBLIC GPS_DEFINITIONS_HEADER="${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GPSReplayDefinitions.h"
    )
    target_link_libraries(${target} PUBLIC ${parser_target})
endfunction()
