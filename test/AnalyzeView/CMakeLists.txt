find_package(Qt6 REQUIRED COMPONENTS Core Test)

target_sources(qgctest
    PRIVATE
        ExifParserTest.cc
        ExifParserTest.h
        GeoTagControllerTest.cc
        GeoTagControllerTest.h
        LogDownloadTest.cc
        LogDownloadTest.h
        MavlinkLogTest.cc
        MavlinkLogTest.h
        PX4LogParserTest.cc
        PX4LogParserTest.h
        ULogParserTest.cc
        ULogParserTest.h
)

target_link_libraries(qgctest
    PRIVATE
        Qt6::Core
        Qt6::Test
)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# https://github.com/ianare/exif-samples
qt_add_resources(qgctest "AnalyzeViewTest_res"
    PREFIX "/"
    FILES
        DSCN0010.jpg
        SampleULog.ulg
)
