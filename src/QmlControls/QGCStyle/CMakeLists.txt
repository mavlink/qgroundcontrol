set(_qgc_style_config "${CMAKE_CURRENT_SOURCE_DIR}/qtquickcontrols2.conf")
qgc_set_qt_resource_alias("${_qgc_style_config}")

qt_add_resources(${CMAKE_PROJECT_NAME} "qgcstyle_configuration"
    PREFIX "/"
    FILES "${_qgc_style_config}"
)

qt_add_library(QGCStyle STATIC)

set_source_files_properties(
    StyleMetrics.qml
    StylePreferences.qml
    StyleTypography.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(QGCStyle
    URI QGCStyle
    VERSION 1.0
    RESOURCE_PREFIX /qml
    DEPENDENCIES
        QtQuick
    QML_FILES
        LayoutProfile.qml
        StyleKit/QGCStyleKit.qml
        StyleMetrics.qml
        StylePreferences.qml
        StyleTypography.qml
        Templates/ApplicationWindow.qml
        Templates/Label.qml
    SOURCES
        impl/InputCapabilities.cc
        impl/InputCapabilities.h
        impl/SystemFonts.cc
        impl/SystemFonts.h
)

target_link_libraries(QGCStyle PRIVATE Qt6::Gui Qt6::LabsStyleKit)
target_include_directories(QGCStyle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/impl")
