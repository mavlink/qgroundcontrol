add_subdirectory(AppSettings)
add_subdirectory(toolbar)

qt_add_library(MainWindowModule STATIC)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE MainWindowModuleplugin)

qt_add_qml_module(MainWindowModule
    URI QGroundControl.MainWindow
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        MainWindow.qml
)
