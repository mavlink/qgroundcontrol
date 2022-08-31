find_package(Qt5 COMPONENTS Core REQUIRED)

add_library(libevents_generated)
target_include_directories(libevents_generated INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libevents/libs/cpp/generated)

add_library(libevents_parser
        definitions.cpp
        libevents/libs/cpp/parse/parser.cpp
        libevents/libs/cpp/protocol/receive.cpp
        )
target_include_directories(libevents_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libevents_parser PUBLIC Qt5::Core)
target_link_libraries(libevents_parser PUBLIC comm)

add_library(libevents_health_and_arming_checks
    libevents/libs/cpp/parse/health_and_arming_checks.cpp)
target_link_libraries(libevents_health_and_arming_checks PRIVATE comm)
