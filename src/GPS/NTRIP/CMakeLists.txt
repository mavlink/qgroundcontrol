# ============================================================================
# NTRIP Module
# NTRIP client, RTCM parsing, and correction streaming
# ============================================================================

add_subdirectory(Http)

include("${CMAKE_CURRENT_LIST_DIR}/../GPSComponents.cmake")
qgc_gps_component_enabled(NTRIP _qgc_ntrip_enabled)
if(NOT _qgc_ntrip_enabled)
    return()
endif()

add_subdirectory(Client)

if(NOT TARGET ${CMAKE_PROJECT_NAME})
    return()
endif()

add_subdirectory(Service)
