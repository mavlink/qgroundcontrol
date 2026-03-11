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

# Dedicated out-of-process Qt Quick Test executable.
# This runs tst_*.qml files through quick_test_main(), which cannot run
# in-process inside the standard UnitTest harness.
add_executable(QGCQmlQuickTests
    ${CMAKE_CURRENT_SOURCE_DIR}/QmlQuickTestMain.cc
)

target_link_libraries(QGCQmlQuickTests
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Gui
        Qt6::Quick
)

target_compile_definitions(QGCQmlQuickTests
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests"
)

add_test(
    NAME QmlQuickTests
    COMMAND $<TARGET_FILE:QGCQmlQuickTests> -platform offscreen
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(QmlQuickTests PROPERTIES
    LABELS "Unit;QML"
    TIMEOUT ${QGC_TEST_TIMEOUT_UNIT}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_LOGGING_RULES=*.debug=false;QML2_IMPORT_PATH=${CMAKE_BINARY_DIR}/qml"
    FAIL_REGULAR_EXPRESSION "FAIL!;QFATAL;Segmentation fault;ASSERT"
)

foreach(_check_target check check-unit check-fast check-ci)
    if(TARGET ${_check_target})
        add_dependencies(${_check_target} QGCQmlQuickTests)
    endif()
endforeach()

# Copy QML test files to build directory
file(GLOB QML_TEST_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.qml")
file(COPY ${QML_TEST_FILES} DESTINATION ${CMAKE_BINARY_DIR}/qmltests)
