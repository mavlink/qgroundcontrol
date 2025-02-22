find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(MAVLink
    PRIVATE
        EventHandler.cc
        EventHandler.h
        HealthAndArmingCheckReport.cc
        HealthAndArmingCheckReport.h
        logging.cpp
)

target_link_libraries(MAVLink
    PRIVATE
        QmlControls
        Utilities
    PUBLIC
        Qt6::Core
)

target_include_directories(MAVLink
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

#===========================================================================#

message(STATUS "Building libevents")

include(CPM)
CPMAddPackage(
    NAME libevents
    GITHUB_REPOSITORY mavlink/libevents
    GIT_TAG main
    SOURCE_SUBDIR libs/cpp
)

if(TARGET libevents)
    target_link_libraries(MAVLink PUBLIC libevents)
    target_include_directories(MAVLink PUBLIC ${libevents_SOURCE_DIR}/libs/cpp)
endif()
