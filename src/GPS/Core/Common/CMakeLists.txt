# ============================================================================
# GPS Core Common
# Lowest-level GPS vocabulary: receiver types, the receiver configuration request with its base-station settings and
# validation, the driver reports with fix quality and constellations, the event a receiver session reports, the
# receiver families' descriptors and the hint for receiver detection. Also the size-limited file that the receiver
# recording and the correction log are written to.
# ============================================================================

target_sources(
    QGCGPSCore
    PRIVATE
        GPSBoundedFile.cc
        GPSBoundedFile.h
        GPSClock.h
        GPSConnectionErrors.h
        GPSDetectionHint.h
        GPSProtocolMath.cc
        GPSProtocolMath.h
        GPSReceiverConfig.cc
        GPSReceiverConfig.h
        GPSReceiverDescriptor.cc
        GPSReceiverDescriptor.h
        GPSReceiverEvent.h
        GPSReceiverReports.h
        GPSTime.h
        GPSType.h
)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
