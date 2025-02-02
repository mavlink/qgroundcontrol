PROJECT(ScreenToolsplugin)

list(APPEND MODULE_QML_FILES
    ScreenTools.qml
)

set_source_files_properties(${MODULE_QML_FILES} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${PROJECT_NAME}
    URI QGroundControl.ScreenTools
    VERSION 1.0
    STATIC
    RESOURCE_PREFIX /qml
    QML_FILES ${MODULE_QML_FILES}
)
