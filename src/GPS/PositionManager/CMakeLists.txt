# ============================================================================
# Position Manager Module
# Manages GPS/GNSS position sources and simulated position data
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE PositionManager.cpp
            PositionManager.h
            GPSPositionSourceRegistration.cc
            GPSPositionSourceRegistration.h
            GPSPositionSourceAdapter.cc
            GPSPositionSourceAdapter.h
            GPSPositionSourceSelector.cc
            GPSPositionSourceSelector.h
            SimulatedPosition.cc
            SimulatedPosition.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
