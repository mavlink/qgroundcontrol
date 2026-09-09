# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support
# ============================================================================

# The GPS/RTK core and network correction sources do not require serial support.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(RTCM)
add_subdirectory(NMEA)
add_subdirectory(NTRIP)
add_subdirectory(PositionManager)

add_subdirectory(Driver)
add_subdirectory(RTK)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSManager.cc GPSManager.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSConnectionState.cc GPSConnectionState.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSourceHealth.cc GPSSourceHealth.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSCorrectionManager.cc GPSCorrectionManager.h)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSDriver)
