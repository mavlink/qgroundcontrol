# ============================================================================
# NTRIP Module
# NTRIP client, RTCM parsing, and correction streaming
# ============================================================================

add_subdirectory(Http)

if(DEFINED QGC_GPS_COMPONENTS_ENABLED AND NOT "NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    return()
endif()

add_subdirectory(Client)

if(NOT TARGET ${CMAKE_PROJECT_NAME})
    return()
endif()

add_subdirectory(Service)
