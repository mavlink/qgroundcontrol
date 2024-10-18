find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test)

qt_add_library(GpsTest
    STATIC
        GpsTest.cc
        GpsTest.h
)

target_link_libraries(GpsTest
    PRIVATE
        Qt6::Test
        GPS
    PUBLIC
        qgcunittest
)

target_include_directories(GpsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
