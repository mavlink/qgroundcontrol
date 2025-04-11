set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ExifParserTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ExifParserTest.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/GeoTagControllerTest.cc
    # ${CMAKE_CURRENT_SOURCE_DIR}/GeoTagControllerTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogDownloadTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LogDownloadTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkLogTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkLogTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4LogParserTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PX4LogParserTest.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/ULogParserTest.cc
    # ${CMAKE_CURRENT_SOURCE_DIR}/ULogParserTest.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# https://github.com/ianare/exif-samples
qt_add_resources(${CMAKE_PROJECT_NAME} "AnalyzeViewTest_res"
    PREFIX "/"
    FILES
        DSCN0010.jpg
        # SampleULog.ulg
)
