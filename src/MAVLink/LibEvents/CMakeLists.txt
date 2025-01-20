find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(LibEventsWrapper STATIC
    EventHandler.cc
    EventHandler.h
    HealthAndArmingCheckReport.cc
    HealthAndArmingCheckReport.h
    logging.cpp
)

target_link_libraries(LibEventsWrapper
    PRIVATE
        QmlControls
        Utilities
    PUBLIC
        Qt6::Core
        MAVLink
)

target_include_directories(LibEventsWrapper
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${libevents_SOURCE_DIR}/libs/cpp
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
    target_link_libraries(LibEventsWrapper PUBLIC libevents)
    target_include_directories(LibEventsWrapper PUBLIC ${libevents_SOURCE_DIR}/libs/cpp)
endif()
