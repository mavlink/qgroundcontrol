# ----------------------------------------------------------------------------
# Qt Quick Test (QML Testing)
# ----------------------------------------------------------------------------
if(TARGET Qt6::QuickTest)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::QuickTest)
else()
    message(WARNING "Qt6::QuickTest not found - QML testing disabled")
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlTesting.h
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlTestRunner.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlTestRunner.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Copy QML test files to build directory
file(GLOB QML_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.qml")
file(COPY ${QML_TEST_FILES} DESTINATION ${CMAKE_BINARY_DIR}/qmltests)
