find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(libevents_generated STATIC
    libevents/libs/cpp/generated/events_generated.h
)

target_include_directories(libevents_generated
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        libevents/libs/cpp/generated
)


qt_add_library(libevents_parser STATIC
    definitions.cpp
    libevents_definitions.h
    libevents/libs/cpp/parse/parser.cpp
    libevents/libs/cpp/parse/parser.h
    libevents/libs/cpp/protocol/receive.cpp
    libevents/libs/cpp/protocol/receive.h
)

add_subdirectory(libevents/libs/cpp/parse/nlohmann_json)

target_include_directories(libevents_parser
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        libevents/libs/cpp/parse
        libevents/libs/cpp/protocol
)

target_link_libraries(libevents_parser
    PRIVATE
        qgc
    PUBLIC
        Qt6::Core
        comm
)


qt_add_library(libevents_health_and_arming_checks STATIC
    libevents/libs/cpp/parse/health_and_arming_checks.cpp
    libevents/libs/cpp/parse/health_and_arming_checks.h
)

target_include_directories(libevents_health_and_arming_checks
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        libevents/libs/cpp/parse
)

target_link_libraries(libevents_health_and_arming_checks
    PUBLIC
        libevents_parser
)


qt_add_library(libevents STATIC)

target_link_libraries(libevents
    PUBLIC
        libevents_generated
        libevents_health_and_arming_checks
)
