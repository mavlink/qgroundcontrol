find_package(Qt6 REQUIRED COMPONENTS Core Test)

target_sources(qgctest
    PRIVATE
        DecompressionTest.cc
        DecompressionTest.h
)

target_link_libraries(qgctest PRIVATE Qt6::Test)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(qgctest "UtilitiesTest_CompressionTest_res"
    PREFIX "/"
    FILES
        manifest.json.gz
        manifest.json.xz
        manifest.json.zip
)
