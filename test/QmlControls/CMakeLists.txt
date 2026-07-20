# ============================================================================
# QmlControls Unit Tests
# ============================================================================

qt_add_library(QGCStyleTesting STATIC)

qt_add_qml_module(QGCStyleTesting
    URI QGCStyle.Testing
    VERSION 1.0
    RESOURCE_PREFIX /qml
    DEPENDENCIES
        QGCStyle
    QML_FILES
        QGCStyle/QGCStyleGallery.qml
        QGCStyle/QGCStyleKitPreview.qml
    NO_PLUGIN
)

target_link_libraries(QGCStyleTesting PRIVATE QGCStyle Qt6::LabsStyleKit)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCStyleTesting)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ObjectItemModelBaseTest.cc
        ObjectItemModelBaseTest.h
        ObjectListModelBaseTest.cc
        ObjectListModelBaseTest.h
        QGCPaletteTest.cc
        QGCPaletteTest.h
        QGCStyleTest.cc
        QGCStyleTest.h
        ScreenToolsTest.cc
        ScreenToolsTest.h
        QmlObjectListModelTest.cc
        QmlObjectListModelTest.h
        QmlObjectTreeModelTest.cc
        QmlObjectTreeModelTest.h
        TestDirtyObject.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(ObjectItemModelBaseTest LABELS Unit)
add_qgc_test(ObjectListModelBaseTest LABELS Unit)
add_qgc_test(QGCPaletteTest LABELS Unit)
add_qgc_test(QGCStyleTest LABELS Unit)
add_qgc_test(ScreenToolsTest LABELS Unit)
add_qgc_test(QmlObjectListModelTest LABELS Unit)
add_qgc_test(QmlObjectTreeModelTest LABELS Unit)

add_executable(QGCLabelBasicStyleTestRunner
    QGCLabelBasicStyleTestMain.cc
)

target_link_libraries(QGCLabelBasicStyleTestRunner
    PRIVATE
        Qt6::Gui
        Qt6::Qml
        Qt6::QuickControls2
)

add_test(
    NAME QGCLabelBasicStyleTest
    COMMAND
        $<TARGET_FILE:QGCLabelBasicStyleTestRunner>
        ${CMAKE_SOURCE_DIR}/src/QmlControls/QGCLabel.qml
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(QGCLabelBasicStyleTest PROPERTIES
    LABELS "Unit;QML"
    TIMEOUT ${QGC_TEST_TIMEOUT_UNIT}
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;LIBGL_ALWAYS_SOFTWARE=1;QT_LOGGING_RULES=*.debug=false"
    FAIL_REGULAR_EXPRESSION "QML load failed;Missing Label property;Unexpected Qt Quick Controls style;Segmentation fault"
)
