find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

qt_add_qml_module(qgcgraphicaleffectsplugin
    URI "QGroundControl.GraphicalEffects"
    VERSION 1.0
    STATIC
    CLASS_NAME QGCGraphicalEffectsPlugin
    PLUGIN_TARGET qgcgraphicaleffectsplugin
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES "ColorOverlay.qml"
    RESOURCES "coloroverlay.frag.qsb"
    SOURCES
        plugin.cpp
        qgfxsourceproxy.cpp
        qgfxsourceproxy_p.h
)

target_include_directories(qgcgraphicaleffectsplugin PUBLIC ${Qt6Quick_PRIVATE_INCLUDE_DIRS})

target_link_libraries(qgcgraphicaleffectsplugin
    PUBLIC
        Qt::Core
        Qt::Qml
        Qt::Quick
)
