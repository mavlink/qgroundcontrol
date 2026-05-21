# ============================================================================
# QGroundControl USV (Unmanned Surface Vehicle) Custom Plugin
# 无人船专用定制插件
# ============================================================================

message(STATUS "QGC: Adding USV Custom Plugin")

# ============================================================================
# Custom Android Package Template (if needed)
# ============================================================================

if(ANDROID)
    set(CUSTOM_ANDROID_DIR "${CMAKE_SOURCE_DIR}/custom/android")
    if(EXISTS "${CUSTOM_ANDROID_DIR}")
        file(GLOB CUSTOM_ANDROID_FILES "${CUSTOM_ANDROID_DIR}/*")
        if(CUSTOM_ANDROID_FILES)
            message(STATUS "QGC: Custom Android package template found. Overlaying custom files...")
            set(DEFAULT_ANDROID_DIR "${CMAKE_SOURCE_DIR}/android")
            set(FINAL_ANDROID_DIR "${CMAKE_BINARY_DIR}/custom/android")
            file(COPY "${DEFAULT_ANDROID_DIR}/." DESTINATION "${FINAL_ANDROID_DIR}")
            file(COPY "${CUSTOM_ANDROID_DIR}/." DESTINATION "${FINAL_ANDROID_DIR}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                "${DEFAULT_ANDROID_DIR}/"
                "${CUSTOM_ANDROID_DIR}/"
            )
            set(QGC_ANDROID_PACKAGE_SOURCE_DIR "${FINAL_ANDROID_DIR}" CACHE PATH "Path to a custom Android package template" FORCE)
        endif()
    endif()
endif()

# ============================================================================
# Custom Resources and Import Paths
# ============================================================================

list(APPEND CUSTOM_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/custom.qrc
)
set(QGC_RESOURCES ${QGC_RESOURCES} ${CUSTOM_RESOURCES} CACHE STRING "Paths to .qrc Resources" FORCE)

set(QML_IMPORT_PATH ${QML_IMPORT_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/res" CACHE STRING "Extra qml import paths" FORCE)

# ============================================================================
# Custom QML Module
# ============================================================================

qt_add_library(USVModule STATIC)

qt_add_qml_module(USVModule
    URI USV
    VERSION 1.0
    RESOURCE_PREFIX /qml
    SOURCES
        src/USVPayloadFactGroup.cc
        src/USVPayloadFactGroup.h
    QML_FILES
        res/USVFlyViewLayout.js
        res/USVPayloadSummaryStrip.qml
        res/USVPayloadDetailPanel.qml
        res/USVSamplingDataView.qml
        res/USVDiagnosticsManualView.qml
        res/USVSamplingDataTokens.js
        res/USVActionBar.qml
        res/USVInstrumentPanel.qml
    NO_PLUGIN
)

target_include_directories(USVModule PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/MAVLink
    ${CMAKE_SOURCE_DIR}/src/Vehicle
    ${CMAKE_SOURCE_DIR}/src/Vehicle/FactGroups
)

# ============================================================================
# Custom Plugin Sources
# ============================================================================

set(CUSTOM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/USVPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/USVPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/USVOptions.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/USVOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/USVFirmwarePlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/USVFirmwarePlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/USVFirmwarePluginFactory.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/USVFirmwarePluginFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin/USVAutoPilotPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin/USVAutoPilotPlugin.h
    CACHE INTERNAL "" FORCE
)

# ============================================================================
# USV Translation Files
# ============================================================================

# 收集 USV 专用翻译文件
file(GLOB USV_TS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/translations/usv_*.ts")

if(USV_TS_SOURCES)
    message(STATUS "QGC: Adding USV translation files: ${USV_TS_SOURCES}")

    # 设置翻译文件输出位置到 i18n 目录
    set_source_files_properties(${USV_TS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_BINARY_DIR}/i18n")

    # 将 USV 翻译文件添加到主项目的翻译列表
    set(USV_TRANSLATIONS ${USV_TS_SOURCES} CACHE INTERNAL "USV Translation files" FORCE)

    message(STATUS "QGC: USV translations will be compiled to: ${CMAKE_BINARY_DIR}/i18n")
endif()

# ============================================================================
# Custom Build Configuration
# ============================================================================

set(CUSTOM_LIBRARIES
    USVModule
    CACHE INTERNAL "" FORCE
)

set(CUSTOM_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/MAVLink
    ${CMAKE_SOURCE_DIR}/src/Vehicle
    ${CMAKE_SOURCE_DIR}/src/Vehicle/FactGroups
    CACHE INTERNAL "" FORCE
)

set(CUSTOM_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="USVPlugin.h"
    CUSTOMCLASS=USVPlugin
    CACHE INTERNAL "" FORCE
)

set(CUSTOM_QT_COMPONENTS
    Core
    Charts
    CACHE INTERNAL "" FORCE
)
