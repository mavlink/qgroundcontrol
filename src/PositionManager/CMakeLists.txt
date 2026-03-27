# ============================================================================
# Position Manager Module
# Manages GPS/GNSS position sources and simulated position data
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        NmeaStreamSplitter.cc
        NmeaStreamSplitter.h
        PositionHealth.h
        PositionManager.cc
        PositionManager.h
        SimulatedPosition.cc
        SimulatedPosition.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
