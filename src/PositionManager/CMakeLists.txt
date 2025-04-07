target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PositionManager.cpp
        PositionManager.h
        SimulatedPosition.cc
        SimulatedPosition.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
