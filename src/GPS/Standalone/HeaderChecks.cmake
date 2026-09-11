include_guard(GLOBAL)

# Separate translation units prevent one public header from supplying another's missing includes.
function(qgc_gps_check_headers target directory)
    if(NOT TARGET ${target} OR NOT IS_DIRECTORY "${directory}")
        message(FATAL_ERROR "Header checks need a production target and header directory")
    endif()
    cmake_parse_arguments(ARG "" "INCLUDE;EXCLUDE" "" ${ARGN})
    file(GLOB headers CONFIGURE_DEPENDS "${directory}/*.h")
    set(sources)
    foreach(header IN LISTS headers)
        get_filename_component(header_name "${header}" NAME_WE)
        if(header_name MATCHES "Private$|GPSDriverBackend"
           OR (ARG_INCLUDE AND NOT header_name MATCHES "${ARG_INCLUDE}")
           OR (ARG_EXCLUDE AND header_name MATCHES "${ARG_EXCLUDE}")
        )
            continue()
        endif()
        set(source "${CMAKE_CURRENT_BINARY_DIR}/headers/${target}/${header_name}.cc")
        file(
            GENERATE
            OUTPUT "${source}"
            CONTENT "#include \"${header}\"\n"
        )
        list(APPEND sources "${source}")
    endforeach()
    if(TARGET ${target}Headers)
        target_sources(${target}Headers PRIVATE ${sources})
        return()
    endif()
    add_library(${target}Headers OBJECT ${sources})
    set_target_properties(
        ${target}Headers
        PROPERTIES AUTOMOC OFF
                   AUTORCC OFF
                   AUTOUIC OFF
    )
    target_link_libraries(${target}Headers PRIVATE ${target})
endfunction()

set(_gps_root "${CMAKE_CURRENT_LIST_DIR}/..")
qgc_gps_check_headers(QGCGPSNativeContracts "${_gps_root}/Driver/Protocols/Contracts")
qgc_gps_check_headers(QGCGPSNative "${_gps_root}/Driver/Protocols")
qgc_gps_check_headers(QGCGPSContracts "${_gps_root}/Contracts")
qgc_gps_check_headers(QGCGPSCore "${_gps_root}/Core")
qgc_gps_check_headers(QGCGPSNMEA "${_gps_root}/NMEA" EXCLUDE "NMEASourceManager|NMEAConnectionAttempt")
qgc_gps_check_headers(QGCGPSConnections "${_gps_root}/NMEA" INCLUDE "NMEASourceManager|NMEAConnectionAttempt")
qgc_gps_check_headers(QGCGPSNTRIPNetwork "${_gps_root}/NTRIP")
qgc_gps_check_headers(QGCGPSPositioning "${_gps_root}/PositionManager")
qgc_gps_check_headers(QGCGPSConnections "${_gps_root}/Receiver" INCLUDE "GPSReceiverAutoConnect|GPSSerialDiscovery")
qgc_gps_check_headers(QGCGPSReceiver "${_gps_root}/Receiver" EXCLUDE "GPSReceiverAutoConnect|GPSSerialDiscovery")
qgc_gps_check_headers(QGCGPSDriver "${_gps_root}/Driver")
qgc_gps_check_headers(QGCGPSModels "${_gps_root}/Models")
qgc_gps_check_headers(QGCGPSCorrections "${_gps_root}/Corrections")
foreach(family UBX Ashtech SBF Femto)
    if(TARGET QGCGPS${family})
        qgc_gps_check_headers(QGCGPS${family} "${_gps_root}/Driver/Protocols/${family}")
    endif()
endforeach()
qgc_gps_check_headers(QGCGPSRecordingController "${_gps_root}/Recording")
qgc_gps_check_headers(QGCIO "${_gps_root}/../Utilities/IO")
qgc_gps_check_headers(QGCTiming "${_gps_root}/../Utilities/Timing")
qgc_gps_check_headers(QGCNetworkIO "${_gps_root}/../Utilities/Network/IO")
