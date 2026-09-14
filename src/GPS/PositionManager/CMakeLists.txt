# ============================================================================
# Position Manager Module
# Manages GPS/GNSS position sources and simulated position data
# ============================================================================

if(TARGET ${CMAKE_PROJECT_NAME})
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE PositionManager.cpp PositionManager.h SimulatedPosition.cc
                                                 SimulatedPosition.h
    )

    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_library(QGCGPSPositioning STATIC GPSPositionService.cc GPSPositionSourceAdapter.cc GPSPositionSourceRegistration.cc
                                     GPSPositionSourceSelector.cc
)
add_library(QGC::GPSPositioning ALIAS QGCGPSPositioning)
set_target_properties(QGCGPSPositioning PROPERTIES AUTOMOC ON)
qt_extract_metatypes(QGCGPSPositioning)
target_compile_features(QGCGPSPositioning PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSPositioning
    PUBLIC QGCGPSCore
    PRIVATE QGCLoggingCategory
)
target_sources(
    QGCGPSPositioning
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSPositionService.h
           GPSPositionSourceAdapter.h
           GPSPositionSourceRegistration.h
           GPSPositionSourceSelector.h
)
if(TARGET ${CMAKE_PROJECT_NAME})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSPositioning)
endif()
