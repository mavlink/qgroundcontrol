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
        "${gps_parser_source}/GPSObservation.cc"
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
        "${gps_parser_source}/RTCM/RTCMParser.cc"
        "${gps_parser_source}/RTCM/GPSCorrectionDiagnostics.cc"
        "${gps_parser_source}/RTCM/GPSCorrectionDiagnostics.h"
        "${gps_parser_source}/RTCM/GPSCorrectionRouter.cc"
        "${gps_parser_source}/RTCM/GPSCorrectionRouter.h"
    )
    set_target_properties(${target} PROPERTIES AUTOMOC ON)
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_include_directories(${target}
                               PUBLIC "${gps_parser_source}" "${gps_parser_source}/NMEA" "${gps_parser_source}/NTRIP"
                                      "${gps_parser_source}/RTCM" "${gps_repository}/src/Utilities/Logging"
    )
    target_link_libraries(${target} PUBLIC Qt6::Core Qt6::Positioning)
endfunction()
