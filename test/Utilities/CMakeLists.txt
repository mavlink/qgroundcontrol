find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(UtilitiesTest STATIC
    FileSystem/QGCFileDownloadTest.cc
    FileSystem/QGCFileDownloadTest.h
)

target_link_libraries(UtilitiesTest
    PRIVATE
        Qt6::Test
        Utilities
    PUBLIC
        qgcunittest
)

target_include_directories(UtilitiesTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} FileSystem)

qt_add_resources(UtilitiesTest "UtilitiesTest_res"
    PREFIX "/"
    FILES
        arducopter.apj
)

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
