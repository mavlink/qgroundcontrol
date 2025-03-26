find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test)

target_sources(qgctest
    PRIVATE
        GpsTest.cc
        GpsTest.h
)

target_link_libraries(qgctest PRIVATE Qt6::Test)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
