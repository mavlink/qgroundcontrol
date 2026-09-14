# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support
# ============================================================================

# The GPS/RTK core and network correction sources do not require serial support.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(RTCM)
add_subdirectory(NTRIP)
add_subdirectory(PositionManager)

add_subdirectory(RTK)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NmeaSourceManager.cc NmeaSourceManager.h GPSManager.cc GPSManager.h)
