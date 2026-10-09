# ============================================================================
# Positioning Module
# GPS observations, source health, GCS position source selection policy, and the platform source
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSObservation.cc GPSObservation.h GPSSourceHealth.cc GPSSourceHealth.h
                                             PositionManager.cc PositionManager.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
