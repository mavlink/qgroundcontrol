find_package(Qt6 REQUIRED COMPONENTS Core Test)

target_sources(UtilitiesTest
    PRIVATE
        DecompressionTest.cc
        DecompressionTest.h
)

target_link_libraries(UtilitiesTest
    PRIVATE
        Qt6::Test
        Utilities
    PUBLIC
        qgcunittest
)

target_include_directories(UtilitiesTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(UtilitiesTest "UtilitiesTest_CompressionTest_res"
    PREFIX "/"
    FILES
        manifest.json.gz
        manifest.json.xz
        manifest.json.zip
)
