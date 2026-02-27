target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        EventHandler.cc
        EventHandler.h
        HealthAndArmingCheckReport.cc
        HealthAndArmingCheckReport.h
        logging.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

if(NOT USE_SYSTEM_LIBEVENTS)
CPMAddPackage(
    NAME libevents
    GITHUB_REPOSITORY mavlink/libevents
    GIT_TAG main
    SOURCE_SUBDIR libs/cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE libevents)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${libevents_SOURCE_DIR}/libs/cpp)
else()
find_package(libevents REQUIRED)
set(LIBEVENTS_INCLUDE_DIRS /usr/include/libevents)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${LIBEVENTS_INCLUDE_DIRS})
find_library(EVENTS_HEALTH_LIB events_health_and_arming_checks)
find_library(EVENTS_PARSER_LIB events_parser)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${EVENTS_HEALTH_LIB})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${EVENTS_PARSER_LIB})
find_package(PkgConfig)

if(TARGET libevents)
    if(TARGET events_health_and_arming_checks)
        if(TARGET events_parser)
            target_link_libraries(events_health_and_arming_checks PRIVATE events_parser)
            message(STATUS "Fixed: events_health_and_arming_checks -> events_parser")
        endif()
    endif()

    if(TARGET events::health_and_arming_checks AND TARGET events::parser)
        target_link_libraries(events::health_and_arming_checks PRIVATE events::parser)
        message(STATUS "Fixed: events::health_and_arming_checks -> events::parser")
    endif()
endif()
endif()
