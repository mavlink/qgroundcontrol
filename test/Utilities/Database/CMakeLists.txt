target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCSqlHelperTest.cc
        QGCSqlHelperTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities/Database
)
