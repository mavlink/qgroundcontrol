message(STATUS "ATLAS: personalizacao carregada")

# ============================================================================
# BRANDING
# ============================================================================

set(ATLAS_BRAND_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/res/atlas_logo_white.svg"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/res/atlas_logo_white.svg"
    PROPERTIES
    QT_RESOURCE_ALIAS "atlas_logo_white.svg"
)

qt_add_resources(
    ${CMAKE_PROJECT_NAME}
    atlas_brand
    PREFIX "/atlas"
    FILES ${ATLAS_BRAND_RESOURCES}
)

# ============================================================================
# QML OVERRIDES
# ============================================================================

set(ATLAS_QML_OVERRIDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewCustomLayer.qml"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewCustomLayer.qml"
    PROPERTIES
    QT_RESOURCE_ALIAS
    "QGroundControl/FlyView/FlyViewCustomLayer.qml"
)

qt_add_resources(
    ${CMAKE_PROJECT_NAME}
    atlas_qml_overrides
    PREFIX "/Custom/qml"
    FILES ${ATLAS_QML_OVERRIDES}
)

# ============================================================================
# ATLAS C++ SOURCES
# ============================================================================

set(CUSTOM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasCustomPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasCustomPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasJetsonClient.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AtlasJetsonClient.h
    PARENT_SCOPE
)

set(CUSTOM_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    PARENT_SCOPE
)

# ============================================================================
# QGC CUSTOM PLUGIN
# ============================================================================

set(CUSTOM_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="AtlasCustomPlugin.h"
    CUSTOMCLASS=AtlasCustomPlugin
    PARENT_SCOPE
)
