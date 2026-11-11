# Correction routing: stream selection, MAVLink packing for vehicles, UDP input and output, the stream's log and its
# per-message counts.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSCorrectionManager.cc
        GPSCorrectionManager.h
        GPSCorrectionSelector.cc
        GPSCorrectionSelector.h
        RTCMLogFile.cc
        RTCMLogFile.h
        RTCMMAVLink.cc
        RTCMMAVLink.h
        RTCMMAVLinkPacket.cc
        RTCMMAVLinkPacket.h
        RTCMMessageCountModel.cc
        RTCMMessageCountModel.h
        RTCMUdpInput.cc
        RTCMUdpInput.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
