find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_library(CompressionTest
    STATIC
        DecompressionTest.cc
        DecompressionTest.h
)

target_link_libraries(CompressionTest
    PRIVATE
        Qt6::Test
        Compression
    PUBLIC
        qgcunittest
)

target_include_directories(CompressionTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(CompressionTest "CompressionTest"
    PREFIX "/"
    FILES
        manifest.json.gz
        manifest.json.xz
)
