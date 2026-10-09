# ============================================================================
# GPS Core Common
# Lowest-level GPS vocabulary: receiver types, the receiver configuration request with its base-station settings and
# validation, the driver reports with fix quality and constellations, and the receiver families' descriptors.
# ============================================================================

target_sources(
    QGCGPSCore
    PRIVATE GPSClock.h
            GPSConnectionErrors.h
            GPSProtocolMath.cc
            GPSProtocolMath.h
            GPSReceiverConfig.cc
            GPSReceiverConfig.h
            GPSReceiverDescriptor.cc
            GPSReceiverDescriptor.h
            GPSReceiverReports.h
            GPSTime.h
            GPSType.h
)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
