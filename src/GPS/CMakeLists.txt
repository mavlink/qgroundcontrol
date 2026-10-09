# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support. Core/ (QGCGPSCore) and NTRIP/HTTP/ (QGCNTRIPHttp) are libraries that build
# without the application; the services in the other folders and the glue here that wires them to settings and
# vehicles are application sources.
# ============================================================================

add_subdirectory(Core)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSManager.cc GPSManager.h GPSSettingsBindings.cc GPSSettingsBindings.h)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSCore GPSModule)

add_subdirectory(Corrections)
add_subdirectory(NTRIP)
add_subdirectory(Positioning)
add_subdirectory(Receiver)
add_subdirectory(Qml)
