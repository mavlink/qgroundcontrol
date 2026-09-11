include_guard(GLOBAL)

# The manifest belongs to the target, so validation and consumers use the same public API.
function(qgc_gps_public_headers target directory)
    if(NOT TARGET ${target})
        return()
    endif()
    if(NOT ARGN)
        message(FATAL_ERROR "Public header manifest for ${target} cannot be empty")
    endif()
    get_target_property(target_type ${target} TYPE)
    set(scope PUBLIC)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(scope INTERFACE)
    endif()
    set(headers)
    foreach(header IN LISTS ARGN)
        if(NOT EXISTS "${directory}/${header}")
            message(FATAL_ERROR "Missing public header for ${target}: ${directory}/${header}")
        endif()
        list(APPEND headers "${directory}/${header}")
    endforeach()
    target_sources(
        ${target}
        ${scope}
        FILE_SET
        HEADERS
        BASE_DIRS
        "${directory}"
        FILES
        ${headers}
    )
    set_property(
        TARGET ${target}
        APPEND
        PROPERTY QGC_PUBLIC_HEADER_NAMES ${ARGN}
    )
    set_property(GLOBAL APPEND PROPERTY QGC_GPS_PUBLIC_API_TARGETS ${target})
endfunction()

# Keep each target's API declaration independent of neighboring implementation files.
function(qgc_gps_declare_public_headers)
    set(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    if(TARGET QGCGPSNativeContracts)
        get_target_property(contract_binary_dir QGCGPSNativeContracts BINARY_DIR)
        qgc_gps_public_headers(QGCGPSNativeContracts "${contract_binary_dir}" GPSProtocolFeatures.h)
    endif()
    qgc_gps_public_headers(
        QGCGPSNativeContracts
        "${root}/Driver/Protocols/Contracts"
        GPSBaseStationConfig.h
        GPSCommandTransaction.h
        GPSConstellation.h
        GPSDeadline.h
        GPSDecodedBatch.h
        GPSIntegrityReport.h
        GPSDriverRevision.h
        GPSExecutionContext.h
        GPSIOStatus.h
        GPSPositionReport.h
        GPSProtocolIO.h
        GPSReceiverSettingId.h
        GPSRelativeReport.h
        GPSSatelliteData.h
        GPSSatelliteReport.h
        GPSSurveyReport.h
    )
    qgc_gps_public_headers(
        QGCGPSNMEAProtocol
        "${root}/Driver/Protocols/NMEA"
        GPSNMEAReport.h
        NMEAConstellation.h
        NMEAFields.h
        NMEASatelliteEpoch.h
        NMEASentence.h
    )
    qgc_gps_public_headers(QGCGPSNativeCommon "${root}/Driver/Protocols" GPSBaseProtocol.h GPSProtocol.h
                           GPSProtocolTime.h
    )
    qgc_gps_public_headers(
        QGCGPSContracts
        "${root}/Contracts"
        GPSConfigurationReport.h
        GPSConnectionError.h
        GPSReceiverCapabilities.h
        GPSReceiverConfig.h
        GPSReceiverFamily.h
        GPSReceiverProfile.h
        GPSReceiverSetting.h
        GPSTransportResult.h
        GPSType.h
    )
    qgc_gps_public_headers(
        QGCGPSCore
        "${root}/Core"
        GPSBaseReference.h
        GPSConnectionControl.h
        GPSConnectionState.h
        GPSIntegrityObservation.h
        GPSIntegrityStore.h
        GPSObservation.h
        GPSRelativePositionStore.h
        GPSSatelliteStore.h
        GPSSourceHealth.h
        GPSSurveyInStatus.h
    )
    qgc_gps_public_headers(
        QGCGPSCorrections
        "${root}/Corrections"
        GPSCorrectionDiagnostics.h
        GPSCorrectionEventModel.h
        GPSCorrectionFrame.h
        GPSCorrectionLedger.h
        GPSCorrectionRouter.h
        GPSCorrectionSelector.h
        GPSCorrectionSourceRegistration.h
        RTCMFrame.h
        RTCMFrameDecoder.h
        RTCMFramer.h
        RTCMMavlinkPacket.h
    )
    qgc_gps_public_headers(QGCGPSModels "${root}/Models" GPSRelativePositionModel.h GPSSatelliteModel.h)
    qgc_gps_public_headers(
        QGCGPSNMEA
        "${root}/NMEA"
        NMEADecoderSession.h
        NMEAPositionSource.h
        NMEASatelliteAdapter.h
        NMEASentenceEnvelope.h
        NMEAStreamSplitter.h
        NMEAUtils.h
    )
    qgc_gps_public_headers(
        QGCGPSNTRIPSession
        "${root}/NTRIP"
        NTRIPError.h
        NTRIPHttpDecoder.h
        NTRIPRequest.h
        NTRIPSession.h
        NTRIPStream.h
        NTRIPTransportConfig.h
    )
    qgc_gps_public_headers(
        QGCGPSNTRIPNetwork
        "${root}/NTRIP"
        NTRIPConnectionStats.h
        NTRIPGgaProvider.h
        NTRIPHttpResponse.h
        NTRIPHttpTransport.h
        NTRIPSourceTable.h
        NTRIPSourceTableController.h
        NTRIPTlsPolicy.h
    )
    qgc_gps_public_headers(QGCGPSPositioning "${root}/PositionManager" GPSPositionService.h GPSPositionSourceAdapter.h
                           GPSPositionSourceRegistration.h GPSPositionSourceSelector.h
    )
    qgc_gps_public_headers(QGCGPSDriver "${root}/Driver" GPSDriver.h GPSDriverData.h)
    qgc_gps_public_headers(QGCGPSTransport "${root}/Driver/Transport" GPSTransport.h)
    qgc_gps_public_headers(QGCGPSReceiverTransports "${root}/Driver/Transport" TcpGPSTransport.h UdpGPSTransport.h)
    qgc_gps_public_headers(
        QGCGPSRecording
        "${root}/Recording"
        GPSRecordingBuffer.h
        GPSRecordingDevice.h
        GPSRecordingFormat.h
        GPSRecordingEvent.h
        GPSRecordingValidator.h
        GPSRecordingTransport.h
    )
    qgc_gps_public_headers(QGCGPSRecordingController "${root}/Recording" GPSRecordingController.h)
    qgc_gps_public_headers(
        QGCGPSReceiver
        "${root}/Receiver"
        GPSByteStream.h
        GPSProvider.h
        GPSReceiverAttempt.h
        GPSReceiverAttemptReducer.h
        GPSReceiverFailure.h
        GPSReceiverMailbox.h
        GPSReceiverSession.h
        GPSReceiverState.h
        GPSReceiverTransportFactory.h
    )
    qgc_gps_public_headers(QGCGPSConnections "${root}/Receiver" GPSReceiverAutoConnect.h GPSSerialDiscovery.h)
    qgc_gps_public_headers(
        QGCGPSUBX
        "${root}/Driver/Protocols"
        UBX/GPSDriverUBX.h
        UBX/UBXFrameDecoder.h
        UBX/UBXReceiverController.h
        UBX/UBXMessageSchema.h
        UBX/UBXMessageCodec.h
        UBX/UBXMessages.h
        UBX/UBXNavigationEpoch.h
        UBX/UBXReceiverProfile.h
        UBX/UBXWire.h
    )
    qgc_gps_public_headers(QGCGPSAshtech "${root}/Driver/Protocols" Ashtech/GPSDriverAshtech.h)
    qgc_gps_public_headers(QGCGPSSBF "${root}/Driver/Protocols" SBF/GPSDriverSBF.h SBF/SBFMessages.h)
    qgc_gps_public_headers(QGCGPSFemto "${root}/Driver/Protocols" Femto/FemtoMessages.h Femto/GPSDriverFemto.h)
    qgc_gps_public_headers(QGCIO "${root}/../Utilities/IO" DataRateTracker.h ReadTimestamp.h TimestampedByteBuffer.h)
    qgc_gps_public_headers(QGCTiming "${root}/../Utilities/Timing" ManualScheduler.h QtRuntimeScheduler.h
                           RuntimeScheduler.h ScheduledTask.h
    )
    qgc_gps_public_headers(QGCNetworkIO "${root}/../Utilities/Network/IO" UdpForwarder.h UdpIODevice.h UdpPeer.h)
    qgc_gps_public_headers(QGCLoggingCategory "${root}/../Utilities/Logging/Category" QGCLoggingCategory.h)
    qgc_gps_public_headers(QGCJsonValidation "${root}/../Utilities/Parsing/Json" JsonParsing.h JsonSchemaValidator.h)
    qgc_gps_public_headers(QGCWire "${root}/../Utilities/Parsing/Wire" LittleEndian.h)
    qgc_gps_public_headers(QGCGPSConnections "${root}/Receiver" NMEASourceManager.h NMEAConnectionAttempt.h)
    if(NOT QGC_NO_SERIAL_LINK)
        qgc_gps_public_headers(QGCGPSReceiverTransports "${root}/Driver/Transport" SerialGPSTransport.h)
    endif()
endfunction()
