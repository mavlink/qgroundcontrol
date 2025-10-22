# add_subdirectory(AppSettings)
# add_subdirectory(toolbar)

# qt_add_library(MainWindowModule STATIC)

# qt_add_qml_module(MainWindowModule
#     URI QGroundControl.MainWindow
#     VERSION 1.0
#     RESOURCE_PREFIX /qml
#     QML_FILES
#         MainWindow.qml
#     NO_PLUGIN
# )



add_subdirectory(AppSettings)
add_subdirectory(toolbar)

qt_add_library(MainWindowModule STATIC)

qt_add_qml_module(MainWindowModule
    URI QGroundControl.MainWindow
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        MainWindow.qml
    SOURCES
        mainwindow_prefs.h
        mainwindow_prefs.cc
    NO_PLUGIN
)
target_link_libraries(MainWindowModule PRIVATE Qt6::Core Qt6::Qml)


