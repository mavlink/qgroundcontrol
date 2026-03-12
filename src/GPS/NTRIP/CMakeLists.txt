# ============================================================================
# NTRIP Module
# NTRIP client, RTCM parsing, and correction streaming
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        NTRIPHttpTransport.cc
        NTRIPHttpTransport.h
        NTRIPManager.cc
        NTRIPManager.h
        NTRIPSourceTable.cc
        NTRIPSourceTable.h
        RTCMParser.cc
        RTCMParser.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
