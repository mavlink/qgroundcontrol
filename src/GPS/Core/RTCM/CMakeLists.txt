# ============================================================================
# GPS Core RTCM
# RTCM 3 framing and CRC-24Q validation, receipt-stamped frame decoding and per-message counts.
# ============================================================================

target_sources(QGCGPSCore PRIVATE RTCMFramer.cc RTCMFramer.h RTCMMessageCount.h)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
