# Correction routing: stream selection, MAVLink packing for vehicles, and UDP input and output.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSCorrectionManager.cc
            GPSCorrectionManager.h
            GPSCorrectionSelector.cc
            GPSCorrectionSelector.h
            RTCMMAVLink.cc
            RTCMMAVLink.h
            RTCMMAVLinkPacket.cc
            RTCMMAVLinkPacket.h
            RTCMUdpInput.cc
            RTCMUdpInput.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
