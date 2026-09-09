# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support
# ============================================================================

# The GPS/RTK core and network correction sources do not require serial support.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Core)
add_subdirectory(RTCM)
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

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSCorrectionManager.cc GPSCorrectionManager.h)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSDriver QGCGPSCore QGCGPSReceiver)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSPositionFactGroup.cc GPSPositionFactGroup.h)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_position
    PREFIX "/json/Vehicle"
    FILES GPSFact.json
)
