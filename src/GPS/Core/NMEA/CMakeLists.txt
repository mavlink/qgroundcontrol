# ============================================================================
# GPS Core NMEA
# NMEA 0183 sentences: framing, field parsing, navigation and satellite epoch assembly, and sentence building; and the
# checked header of NovAtel-style ASCII logs.
# ============================================================================

target_sources(
    QGCGPSCore
    PRIVATE GPSASCIILog.h
            NMEAFramer.h
            NMEANavigationEpoch.cc
            NMEANavigationEpoch.h
            NMEASatellites.cc
            NMEASatellites.h
            NMEASentence.cc
            NMEASentence.h
)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
