# ============================================================================
# GPS Core RTCM
# RTCM 3 framing and CRC-24Q validation, receipt-stamped frame decoding, per-message counts and the base station
# position of messages 1005 and 1006.
# ============================================================================

target_sources(
    QGCGPSCore
    PRIVATE
        RTCMFramer.cc
        RTCMFramer.h
        RTCMMessageCountModel.cc
        RTCMMessageCountModel.h
        RTCMStationPosition.cc
        RTCMStationPosition.h
)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
