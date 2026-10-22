target_sources(
    QGCUnitTests
    PRIVATE
        QGCSqlHelperTest.cc
        QGCSqlHelperTest.h
)

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities/Database
)

add_qgc_test(QGCSqlHelperTest LABELS Unit Utilities)
