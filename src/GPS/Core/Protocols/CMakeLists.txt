# ============================================================================
# GPS Core Protocols
# GPSDriver and the protocol framework it hosts the receiver families on: the family table, the runtime with its
# command channel, stream demultiplexer and receiver detection, and the components the families share. Each family
# folder (Ashtech, Femto, Passive, Quectel, SBF, UBX, Unicore) holds its protocol and plan: <F>Plan holds the wire data
# (commands, rates, timeouts, limits) and stateless reply matchers, <F>Protocol the receiver state, the matchers that
# read it, and decoding. The headers are private to QGCGPSCore; the protocol test support exports them to the tests,
# the hardware runner and the fuzzer.
# ============================================================================

target_sources(
    QGCGPSCore
    PRIVATE GPSCommand.h
            GPSCommandChannel.cc
            GPSCommandChannel.h
            GPSDeadline.h
            GPSDecodedData.cc
            GPSDecodedData.h
            GPSDriver.cc
            GPSDriver.h
            GPSFamilyProtocol.h
            GPSNMEAFamilyProtocol.h
            GPSNMEAStream.cc
            GPSNMEAStream.h
            GPSProtocolEvent.h
            GPSProtocolRuntime.cc
            GPSProtocolRuntime.h
            GPSReceiverDetector.cc
            GPSReceiverDetector.h
            GPSReceiverFamilies.cc
            GPSReceiverFamilies.h
            GPSStreamDemux.cc
            GPSStreamDemux.h
            GPSSurveyClock.h
            Ashtech/AshtechPlan.cc
            Ashtech/AshtechPlan.h
            Ashtech/AshtechProtocol.cc
            Femto/FemtoPlan.cc
            Femto/FemtoPlan.h
            Femto/FemtoProtocol.cc
            Passive/PassiveProtocol.cc
            Quectel/QuectelCodec.h
            Quectel/QuectelPlan.cc
            Quectel/QuectelPlan.h
            Quectel/QuectelProtocol.cc
            SBF/SBFBlocks.h
            SBF/SBFPlan.cc
            SBF/SBFPlan.h
            SBF/SBFProtocol.cc
            UBX/UBXConfigKeys.h
            UBX/UBXConfigurator.cc
            UBX/UBXDecoder.cc
            UBX/UBXEpochAssembler.cc
            UBX/UBXEpochAssembler.h
            UBX/UBXFrame.h
            UBX/UBXMessageSchema.h
            UBX/UBXPlan.cc
            UBX/UBXPlan.h
            UBX/UBXProtocol.h
            UBX/UBXReceiverController.h
            Unicore/UnicorePlan.cc
            Unicore/UnicorePlan.h
            Unicore/UnicoreProtocol.cc
)
# Family headers are included by folder, such as "UBX/UBXPlan.h".
target_include_directories(QGCGPSCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
