include_guard(GLOBAL)

# Explicit component dispatch keeps the production source and dependency manifests together.
# cmake-lint: disable=R0912,R0915

# This manifest is shared by application, standalone replay, and instrumented fuzz builds.
function(qgc_gps_component_sources output component)
    set(gps_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    if(component STREQUAL "Contracts")
        set(sources
            "${gps_root}/Contracts/GPSType.h"
            "${gps_root}/Contracts/GPSReceiverFamily.h"
            "${gps_root}/Contracts/GPSReceiverFamily.cc"
            "${gps_root}/Contracts/GPSReceiverConfig.h"
            "${gps_root}/Contracts/GPSBaseStationConfig.h"
            "${gps_root}/Contracts/GPSIOStatus.h"
            "${gps_root}/Contracts/GPSReceiverConfig.cc"
            "${gps_root}/Contracts/GPSReceiverSetting.h"
            "${gps_root}/Contracts/GPSReceiverCapabilities.h"
            "${gps_root}/Contracts/GPSReceiverCapabilities.cc"
            "${gps_root}/Contracts/GPSConfigurationReport.h"
            "${gps_root}/Contracts/GPSReceiverProfile.h"
            "${gps_root}/Contracts/GPSReceiverProfile.cc"
            "${gps_root}/Contracts/GPSConnectionError.h"
            "${gps_root}/Contracts/GPSTransportResult.h"
        )
    elseif(component STREQUAL "Scheduler")
        set(sources "${gps_root}/Core/GPSRuntimeScheduler.cc" "${gps_root}/Core/GPSRuntimeScheduler.h"
                    "${gps_root}/Core/GPSQtRuntimeScheduler.cc" "${gps_root}/Core/GPSQtRuntimeScheduler.h"
        )
    elseif(component STREQUAL "Core")
        set(sources
            "${gps_root}/Core/GPSConnectionState.cc"
            "${gps_root}/Core/GPSConnectionState.h"
            "${gps_root}/Core/GPSObservation.cc"
            "${gps_root}/Core/GPSObservation.h"
            "${gps_root}/Core/GPSConstellation.h"
            "${gps_root}/Core/GPSBaseReference.h"
            "${gps_root}/Core/GPSIntegrityObservation.h"
            "${gps_root}/Core/GPSReadTimestamp.h"
            "${gps_root}/Core/GPSSurveyInStatus.h"
            "${gps_root}/Core/GPSSatelliteStore.cc"
            "${gps_root}/Core/GPSSatelliteStore.h"
            "${gps_root}/Core/GPSSourceHealth.cc"
            "${gps_root}/Core/GPSSourceHealth.h"
        )
    elseif(component STREQUAL "NMEA")
        set(sources
            "${gps_root}/NMEA/NMEADecoderSession.cc"
            "${gps_root}/NMEA/NMEADecoderSession.h"
            "${gps_root}/NMEA/NMEAPositionSource.cc"
            "${gps_root}/NMEA/NMEAPositionSource.h"
            "${gps_root}/NMEA/NMEASatelliteAdapter.cc"
            "${gps_root}/NMEA/NMEASatelliteAdapter.h"
            "${gps_root}/NMEA/NMEAStreamSplitter.cc"
            "${gps_root}/NMEA/NMEAStreamSplitter.h"
            "${gps_root}/NMEA/NMEAUtils.cc"
            "${gps_root}/NMEA/NMEAUtils.h"
            "${gps_root}/NMEA/NMEAFields.h"
        )
    elseif(component STREQUAL "NTRIPSession")
        set(sources
            "${gps_root}/NTRIP/NTRIPError.h"
            "${gps_root}/NTRIP/NTRIPHttpDecoder.cc"
            "${gps_root}/NTRIP/NTRIPHttpDecoder.h"
            "${gps_root}/NTRIP/NTRIPSession.cc"
            "${gps_root}/NTRIP/NTRIPSession.h"
            "${gps_root}/NTRIP/NTRIPStream.cc"
            "${gps_root}/NTRIP/NTRIPStream.h"
            "${gps_root}/NTRIP/NTRIPTransportConfig.cc"
            "${gps_root}/NTRIP/NTRIPTransportConfig.h"
            "${gps_root}/NTRIP/NTRIPRequest.cc"
            "${gps_root}/NTRIP/NTRIPRequest.h"
        )
    elseif(component STREQUAL "Corrections")
        set(sources
            "${gps_root}/Corrections/RTCMMavlinkPacket.cc"
            "${gps_root}/Corrections/RTCMMavlinkPacket.h"
            "${gps_root}/Corrections/GPSCorrectionRouter.cc"
            "${gps_root}/Corrections/GPSCorrectionRouter.h"
            "${gps_root}/Corrections/GPSCorrectionFrame.h"
            "${gps_root}/Corrections/GPSCorrectionDiagnostics.cc"
            "${gps_root}/Corrections/GPSCorrectionDiagnostics.h"
            "${gps_root}/Corrections/GPSCorrectionEventModel.cc"
            "${gps_root}/Corrections/GPSCorrectionEventModel.h"
            "${gps_root}/Corrections/RTCMParser.cc"
            "${gps_root}/Corrections/RTCMParser.h"
            "${gps_root}/Corrections/RTCMFramer.h"
            "${gps_root}/Corrections/RTCMFrameDecoder.cc"
            "${gps_root}/Corrections/RTCMFrameDecoder.h"
            "${gps_root}/Corrections/GPSCorrectionSourceRegistration.cc"
            "${gps_root}/Corrections/GPSCorrectionSourceRegistration.h"
        )
    elseif(component STREQUAL "Transport")
        set(sources "${gps_root}/Driver/Transport/GPSTransport.cc" "${gps_root}/Driver/Transport/GPSTransport.h")
    elseif(component STREQUAL "ReceiverTransports")
        set(sources
            "${gps_root}/Driver/Transport/GPSSocketWait.cc" "${gps_root}/Driver/Transport/GPSSocketWait.h"
            "${gps_root}/Driver/Transport/TcpGPSTransport.cc" "${gps_root}/Driver/Transport/TcpGPSTransport.h"
            "${gps_root}/Driver/Transport/UdpGPSTransport.cc" "${gps_root}/Driver/Transport/UdpGPSTransport.h"
        )
    elseif(component STREQUAL "Driver")
        set(sources
            "${gps_root}/Driver/GPSDriver.cc"
            "${gps_root}/Driver/GPSDriver.h"
            "${gps_root}/Driver/GPSDriverClock.h"
            "${gps_root}/Driver/GPSProtocolIO.h"
            "${gps_root}/Driver/GPSDriverBackend.cc"
            "${gps_root}/Driver/GPSDriverBackend.h"
            "${gps_root}/Driver/GPSDriverData.cc"
            "${gps_root}/Driver/GPSDriverData.h"
            "${gps_root}/Driver/GPSDriverPlatform.h"
            "${gps_root}/Driver/GPSSatelliteReport.h"
            "${gps_root}/Driver/GPSRelativeReport.h"
            "${gps_root}/Driver/GPSPositionReport.h"
        )
    elseif(component STREQUAL "Recording")
        set(sources
            "${gps_root}/Recording/GPSRecordingBuffer.cc"
            "${gps_root}/Recording/GPSRecordingBuffer.h"
            "${gps_root}/Recording/GPSRecordingFormat.cc"
            "${gps_root}/Recording/GPSRecordingFormat.h"
            "${gps_root}/Recording/GPSRecordingTransport.cc"
            "${gps_root}/Recording/GPSRecordingTransport.h"
            "${gps_root}/Recording/GPSRecordingDevice.cc"
            "${gps_root}/Recording/GPSRecordingDevice.h"
        )
    elseif(component STREQUAL "Receiver")
        set(sources
            "${gps_root}/Receiver/GPSReceiverTransportFactory.cc"
            "${gps_root}/Receiver/GPSReceiverTransportFactory.h"
            "${gps_root}/Receiver/GPSByteStream.cc"
            "${gps_root}/Receiver/GPSByteStream.h"
            "${gps_root}/Receiver/GPSProvider.cc"
            "${gps_root}/Receiver/GPSProvider.h"
            "${gps_root}/Receiver/GPSReceiverMailbox.cc"
            "${gps_root}/Receiver/GPSReceiverMailbox.h"
            "${gps_root}/Receiver/GPSReceiverSession.cc"
            "${gps_root}/Receiver/GPSReceiverSession.h"
            "${gps_root}/Receiver/GPSReceiverAttempt.h"
        )
    elseif(component STREQUAL "RecordingController")
        set(sources "${gps_root}/Recording/GPSRecordingController.cc" "${gps_root}/Recording/GPSRecordingController.h")
    elseif(component STREQUAL "Native")
        include("${gps_root}/Driver/Protocols/GPSProtocolSources.cmake")
        set(sources ${GPS_PROTOCOL_SOURCES})

    else()
        message(FATAL_ERROR "Unknown GPS component: ${component}")
    endif()
    set(${output}
        ${sources}
        PARENT_SCOPE
    )
endfunction()

# PREFIX binds dependencies to the corresponding runtime; only native code requires a clock definitions header.
function(qgc_add_gps_component target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "COMPONENT;PREFIX;LOGGING_TARGET;NATIVE_DEFINITIONS_HEADER;SERIAL_TARGET"
                          ""
    )
    if(arg_UNPARSED_ARGUMENTS
       OR arg_KEYWORDS_MISSING_VALUES
       OR NOT arg_COMPONENT
    )
        message(FATAL_ERROR "qgc_add_gps_component requires COMPONENT and known arguments with values")
    endif()
    if(NOT arg_PREFIX)
        set(arg_PREFIX QGCGPS)
    endif()
    if(NOT arg_COMPONENT STREQUAL "Contracts"
       AND NOT arg_COMPONENT STREQUAL "Native"
       AND NOT arg_LOGGING_TARGET
    )
        message(FATAL_ERROR "GPS ${arg_COMPONENT} requires an explicit LOGGING_TARGET")
    endif()
    set(gps_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    qgc_gps_component_sources(sources "${arg_COMPONENT}")
    add_library(${target} STATIC ${sources})
    set_target_properties(${target} PROPERTIES AUTOMOC ON)
    target_compile_features(${target} PUBLIC cxx_std_20)
    if(arg_LOGGING_TARGET
       AND NOT arg_COMPONENT STREQUAL "Contracts"
       AND NOT arg_COMPONENT STREQUAL "Native"
    )
        target_link_libraries(${target} PRIVATE ${arg_LOGGING_TARGET})
    endif()
    if(arg_COMPONENT STREQUAL "Contracts")
        target_include_directories(${target} PUBLIC "${gps_root}/Contracts")
        target_link_libraries(${target} PUBLIC Qt6::Core)
    elseif(arg_COMPONENT STREQUAL "Scheduler")
        target_include_directories(${target} PUBLIC "${gps_root}/Core")
        target_link_libraries(${target} PUBLIC Qt6::Core)
    elseif(arg_COMPONENT STREQUAL "Core")
        target_include_directories(${target} PUBLIC "${gps_root}/Core")
        target_link_libraries(${target} PUBLIC ${arg_PREFIX}Contracts ${arg_PREFIX}Scheduler Qt6::Core Qt6::Positioning)
    elseif(arg_COMPONENT STREQUAL "NMEA")
        target_include_directories(${target} PUBLIC "${gps_root}/NMEA")
        target_link_libraries(${target} PUBLIC ${arg_PREFIX}Core)
    elseif(arg_COMPONENT STREQUAL "NTRIPSession")
        target_include_directories(${target} PUBLIC "${gps_root}/NTRIP")
        target_link_libraries(${target} PUBLIC ${arg_PREFIX}Scheduler Qt6::Core)
    elseif(arg_COMPONENT STREQUAL "Corrections")
        target_include_directories(${target} PUBLIC "${gps_root}/Corrections" "${gps_root}/../Utilities/Math")
        target_link_libraries(${target} PUBLIC Qt6::Core)
    elseif(arg_COMPONENT STREQUAL "Transport")
        target_include_directories(${target} PUBLIC "${gps_root}/Driver/Transport")
        target_link_libraries(${target} PUBLIC ${arg_PREFIX}Contracts Qt6::Core)
    elseif(arg_COMPONENT STREQUAL "ReceiverTransports")
        target_include_directories(${target} PUBLIC "${gps_root}/Driver/Transport")
        target_link_libraries(
            ${target}
            PUBLIC ${arg_PREFIX}Transport
            PRIVATE Qt6::Network
        )
        if(arg_SERIAL_TARGET)
            target_sources(${target} PRIVATE "${gps_root}/Driver/Transport/SerialGPSTransport.cc"
                                             "${gps_root}/Driver/Transport/SerialGPSTransport.h"
            )
            target_link_libraries(${target} PRIVATE ${arg_SERIAL_TARGET})
        else()
            target_compile_definitions(${target} PUBLIC QGC_NO_SERIAL_LINK)
        endif()
    elseif(arg_COMPONENT STREQUAL "Native" OR arg_COMPONENT STREQUAL "Driver")
        if(NOT arg_NATIVE_DEFINITIONS_HEADER)
            message(FATAL_ERROR "GPS ${arg_COMPONENT} requires NATIVE_DEFINITIONS_HEADER")
        endif()
        target_compile_definitions(${target} PRIVATE GPS_PLATFORM_HEADER="${arg_NATIVE_DEFINITIONS_HEADER}")
        target_include_directories(${target} PRIVATE "${gps_root}/Driver/Protocols" "${gps_root}/Corrections"
                                                     "${gps_root}/NMEA"
        )
        if(arg_COMPONENT STREQUAL "Native")
            include("${gps_root}/../Utilities/Geo/GeographicLib.cmake")
            target_link_libraries(${target} PRIVATE GeographicLib::GeographicLib)
            target_include_directories(${target}
                                       PRIVATE "${gps_root}/Driver" "${gps_root}/Contracts" "${gps_root}/Core"
                                               "${gps_root}/Corrections" "${gps_root}/../Utilities/Math"
            )
            target_link_libraries(${target} PRIVATE Qt6::Core)
            set_target_properties(${target} PROPERTIES AUTOMOC OFF)

        else()
            target_include_directories(${target} PUBLIC "${gps_root}/Driver")
            target_link_libraries(
                ${target}
                PUBLIC ${arg_PREFIX}Contracts ${arg_PREFIX}Core ${arg_PREFIX}Transport
                PRIVATE ${arg_PREFIX}Native
            )
        endif()
    elseif(arg_COMPONENT STREQUAL "Recording")
        target_include_directories(${target} PUBLIC "${gps_root}/Recording")
        target_link_libraries(
            ${target}
            PUBLIC ${arg_PREFIX}Contracts ${arg_PREFIX}Transport ${arg_PREFIX}Core
            PRIVATE QGCJsonValidation
        )
    elseif(arg_COMPONENT STREQUAL "Receiver")
        target_include_directories(${target} PUBLIC "${gps_root}/Receiver")
        target_link_libraries(${target} PUBLIC ${arg_PREFIX}Driver ${arg_PREFIX}Core ${arg_PREFIX}Corrections
                                               ${arg_PREFIX}Recording ${arg_PREFIX}ReceiverTransports
        )
    elseif(arg_COMPONENT STREQUAL "RecordingController")
        target_include_directories(${target} PUBLIC "${gps_root}/Recording")
        target_link_libraries(${target} PUBLIC ${arg_PREFIX}Recording Qt6::QmlIntegration)
    endif()
endfunction()
