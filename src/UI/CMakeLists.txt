project(UIplugin)

add_subdirectory(PlanView)
add_subdirectory(ScreenTools)
add_subdirectory(Toolbar)

qt_add_qml_module(${PROJECT_NAME}
    URI QGroundControl
    VERSION 1.0
    STATIC
    RESOURCE_PREFIX /qml
    QML_FILES
        AppSettings.qml
        MainRootWindow.qml
        SettingsPagesModel.qml
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Quick)
