find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

target_sources(QGC
    PRIVATE
        FollowMe.cc
        FollowMe.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::Positioning
    PUBLIC
        Qt6::Core
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
