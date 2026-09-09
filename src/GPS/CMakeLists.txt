# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support
# ============================================================================

# The GPS/RTK core and network correction sources do not require serial support.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Core)
add_subdirectory(Corrections)
add_subdirectory(Models)
add_subdirectory(NMEA)
add_subdirectory(NTRIP)
add_subdirectory(PositionManager)

add_subdirectory(Driver)
add_subdirectory(Recording)
add_subdirectory(Receiver)
add_subdirectory(BaseStation)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSManager.cc GPSManager.h GPSQmlTypes.h GPSConnectionSettings.cc
                                             GPSConnectionSettings.h
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSDriver QGCGPSCore QGCGPSReceiver)
