# ============================================================================
# GPS Core RTCM
# RTCM 3 framing and CRC-24Q validation, receipt-stamped frame decoding, the base station position of messages 1005
# and 1006, the header of the MSM observation messages, and what a stream of frames carries for an RTK rover.
# ============================================================================

target_sources(
    QGCGPSCore
    PRIVATE
        RTCMBits.h
        RTCMFramer.cc
        RTCMFramer.h
        RTCMMsmHeader.cc
        RTCMMsmHeader.h
        RTCMStationPosition.cc
        RTCMStationPosition.h
        RTCMStreamContent.cc
        RTCMStreamContent.h
)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
